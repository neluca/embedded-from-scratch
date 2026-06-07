# I2C & SPI Communication Guide

## 为什么需要通信协议？

嵌入式系统几乎从不独立工作。传感器、显示屏、存储器、其他 MCU——都需要数据交换。I2C 和 SPI 是最常用的两种**板级串行通信协议**。

```
距离      协议              速率            线数
芯片间    I2C              100k-3.4Mbps     2 线 (SDA+SCL)
芯片间    SPI              1-100Mbps        4 线 (MOSI+MISO+SCK+CS)
板间      UART             9600-1Mbps       2 线 (TX+RX)
```

> UART 已在 Lesson 5 覆盖。本文档聚焦 I2C 和 SPI。

---

## I2C (Inter-Integrated Circuit)

### 物理层

```
       VCC
        |
       [Rp]          [Rp]         上拉电阻 (典型 4.7kΩ)
        |             |
SCL ----+-------------+------  串行时钟 (Master 驱动)
SDA ----+-------------+------  串行数据 (双向, 开漏)
        |             |
       [Master]      [Slave 1]  [Slave 2] ...
```

- **2 线**: SCL (时钟) + SDA (数据)
- **开漏输出**: 需要外部上拉电阻
- **多主多从**: 仲裁机制支持多主, 7/10 位地址支持多从
- **双向**: 同一根 SDA 线上 Master 和 Slave 都能发送数据

### 协议帧

```
START  | 7-bit Addr | R/W | ACK | 8-bit Data | ACK | ... | STOP

START:  SCL=HIGH, SDA 下降沿
STOP:   SCL=HIGH, SDA 上升沿
ACK:    第 9 个时钟周期, 接收方拉低 SDA
NACK:   第 9 个时钟周期, 接收方释放 SDA (保持高)
```

### 典型写操作

```
S | ADDR+W | A | REG_ADDR | A | DATA | A | P
```

### 典型读操作

```
S | ADDR+W | A | REG_ADDR | A | Sr | ADDR+R | A | DATA | N | P
                                            ^^^^
                                    Sr = Repeated START
```

### I2C 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| SDA 卡在低电平 | Slave 拉低 SDA 后复位/死机 | 在 SCL 上发 9 个脉冲恢复 |
| 地址冲突 | 两个 Slave 使用相同地址 | 换用不同地址或使用地址转换器 |
| 时钟拉伸过长 | Slave 处理不过来, 拉低 SCL | 设置超时, 或换用更快的 Slave |
| 上拉电阻过大 | 波形上升沿太慢 | 减小 Rp (但功耗增加) |

### nRF51 TWI 外设

nRF51822 有 2 个 TWI (I2C 兼容) 外设。TWI 是 nRF51 对 I2C Master 的实现:

```
TWI0: 0x40003000
TWI1: 0x40004000

TWI 使用 task/event 模型:
  TASK_STARTRX / TASK_STARTTX  →  启动读/写
  EVENT_RXDREADY / EVENT_TXDSENT → 数据就绪/发送完成
```

---

## SPI (Serial Peripheral Interface)

### 物理层

```
Master                          Slave
  SCK  ─────────────────────────> SCK   时钟
  MOSI ─────────────────────────> MOSI  Master→Slave 数据
  MISO <───────────────────────── MISO  Slave→Master 数据
  CS0  ─────────────────────────> CS     片选 (低有效)
```

- **4 线**: SCK + MOSI + MISO + CS
- **全双工**: 同时收发
- **无固定帧格式**: 数据长度可自定义 (通常 8/16/32 位)
- **多从**: 每个 Slave 需要独立的 CS 线
- **推挽输出**: 不需要上拉电阻, 速度更快

### 时钟模式 (CPOL + CPHA)

| 模式 | CPOL | CPHA | 空闲时钟 | 采样沿 |
|------|------|------|----------|--------|
| 0 | 0 | 0 | 低 | 上升沿 |
| 1 | 0 | 1 | 低 | 下降沿 |
| 2 | 1 | 0 | 高 | 下降沿 |
| 3 | 1 | 1 | 高 | 上升沿 |

### 典型传输

```
CS  ──┐                                ┌──
       └────────────────────────────────┘
SCK     │‾│_│‾│_│‾│_│‾│_│‾│_│‾│_│‾│_│
MOSI    X D7 D6 D5 D4 D3 D2 D1 D0 X
MISO    X D7 D6 D5 D4 D3 D2 D1 D0 X
```

1. Master 拉低 CS
2. Master 在 SCK 上产生时钟
3. 每个时钟周期 MOSI 和 MISO 各传输 1 bit
4. Master 拉高 CS 结束传输

### SPI 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 数据错位 | CPOL/CPHA 不匹配 | 确认 Slave 数据手册要求的模式 |
| 速率不够 | SCK 频率太低 | 提高 SCK (注意 Slave 最大速率限制) |
| CS 竞争 | 多从时 CS 时序错误 | 确保一次只拉低一个 CS |

### nRF51 SPI 外设

nRF51822 的 SPI 外设与 TWI 共享基地址 (同一时刻只能用一种):

```
SPI0: 0x40003000  (与 TWI0 共享)
SPI1: 0x40004000  (与 TWI1 共享)
```

---

## I2C vs SPI — 如何选择？

| | I2C | SPI |
|------|-----|-----|
| 线数 | 2 (+ GND) | 4 + N×CS (+ GND) |
| 速度 | 100k-3.4M | 1-100M |
| 多从 | 地址选择 (同总线) | 片选线 (每从一根) |
| 全双工 | 否 (半双工) | 是 |
| 功耗 | 较低 (开漏 + 上拉) | 较高 (推挽) |
| 复杂度 | 协议层复杂 | 物理层复杂 |
| 典型用途 | 传感器、RTC、EEPROM | Flash、LCD、高速 ADC |

> **经验法则**: 低速传感器 → I2C, 高速/大数据 → SPI。

---

## 驱动实现要点

### I2C 驱动框架

```c
// 初始化
void i2c_init(uint32_t frequency);     // 100kHz / 400kHz

// 基本操作
int i2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint32_t len);
int i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint32_t len);

// 典型用法: 读传感器寄存器
uint8_t whoami;
i2c_read(0x68, 0x75, &whoami, 1);  // 读 MPU6050 WHO_AM_I
```

### SPI 驱动框架

```c
// 初始化
void spi_init(uint32_t frequency, uint8_t mode);  // mode = 0/1/2/3

// 传输 (全双工, 同时收发)
void spi_transfer(uint8_t *tx, uint8_t *rx, uint32_t len);

// 典型用法: 写 Flash 命令
uint8_t cmd[4] = {0x9F, 0, 0, 0};  // JEDEC ID 命令
uint8_t rx[4];
cs_low();
spi_transfer(cmd, rx, 4);
cs_high();
// rx[1..3] = Manufacturer ID + Memory Type + Capacity
```

---

## 相关课程

| 课程 | 内容 |
|------|------|
| Lesson 5 | UART 驱动 (另一个核心串行协议) |
| Lesson 12 | 环形缓冲区 (接收数据缓冲) |
| Lesson 13 | 中断优先级 (SPI/I2C 中断 vs 其他中断) |

## 外部参考

- [I2C-bus Specification (NXP)](https://www.nxp.com/docs/en/user-guide/UM10204.pdf)
- [nRF51822 Product Specification — TWI/SPI chapters](https://infocenter.nordicsemi.com/pdf/nRF51822_PS_v3.3.pdf)
- [Serial Peripheral Interface (Wikipedia)](https://en.wikipedia.org/wiki/Serial_Peripheral_Interface)
