# Lesson 12: Embedded Data Structures & Universal Patterns

## 学习目标

- 掌握环形缓冲区（Ring Buffer）—— 所有流式数据处理的基础
- 理解两种状态机实现（枚举 + switch vs 表驱动）
- 精通跨架构位操作（M0 无 BFI/UBFX/REV，全部手动实现）
- 了解 COBS 数据帧编码（确定性开销，跨平台通信协议）

## 为什么这些是"举一反三"的核心？

| 模式 | 适用场景 | 跨平台性 |
|------|----------|----------|
| **Ring Buffer** | UART/ADC/DMA 缓冲、音频流、传感器数据 | ARM / RISC-V / x86 完全一致 |
| **State Machine** | 协议解析、UI 控制、电源管理、电机控制 | C / C++ / Rust / Python 均可实现 |
| **Bit Manipulation** | 寄存器操作、协议编解码、数据压缩 | 所有 CPU 架构通用 |
| **COBS Framing** | 串口协议、数据包分隔、自定义通信 | 算法与 CPU 无关 |

## 文件结构

```
lesson_12_patterns/
├── CMakeLists.txt
├── linker/microbit.ld
└── src/
    ├── startup.S
    ├── main.c              # 6 个演示模块
    ├── ring_buffer.h / .c  # 环形缓冲区
    ├── state_machine.h / .c # 状态机 (enum + table)
    ├── bit_utils.h / .c     # 位操作工具箱
    └── cobs_framer.h / .c   # COBS 帧编解码
```

## 演示内容

| 模块 | 内容 |
|------|------|
| 1 | 环形缓冲区 — 基本读写 / 批量操作 / 环绕演示 / ISR 安全模式 |
| 2 | 枚举状态机 — UART 包解析器 (IDLE→HEADER→PAYLOAD→CHECKSUM) |
| 3 | 表驱动状态机 — 交通灯示例, 数据与逻辑分离 |
| 4 | 位操作 — 单比特/位域/端序检测/结构体打包/M0 限制 |
| 5 | COBS 帧编码 — 编码/解码/往返验证/对比 SLIP |
| 6 | 综合 — Ring Buffer + FSM + COBS 协议栈架构 |

## 关键知识点

### 为什么 Ring Buffer 大小必须是 2 的幂？

```c
/* 取模运算: 需要除法器 (M0 没有, ~100 周期软件除法) */
index = (index + 1) % size;

/* 位掩码: 单周期 AND 指令 */
index = (index + 1) & (size - 1);  /* size = 256 → mask = 0xFF */
```

### 枚举 FSM vs 表驱动 FSM

| | 枚举 + switch | 表驱动 |
|------|---------------|--------|
| 代码量 | 少 (< 50 行) | 中 (~100 行) |
| 可读性 | 高 | 中 |
| 扩展性 | 需修改 switch | 只加表行 |
| 内存 | 0 额外 | 转换表 (N×M 字节) |
| 适用 | < 8 状态 | > 8 状态 |

### M0 位操作限制

M0 没有 BFI/UBFX/SBFX/REV/RBIT 指令 (ARMv7-M 新增)。
所有位操作依赖 AND/ORR/LSL/LSR 组合 — 这些"手动"方法在所有架构上通用。

### COBS vs SLIP

| | COBS | SLIP |
|------|------|------|
| 最坏开销 | +0.4% | +100% |
| 确定性 | 强 | 弱 |
| 复杂度 | 中 | 低 |
| 适用场景 | 高速/大数据 | 简单调试 |

## 相关文档

- [Lesson 11: 错误处理](../lesson_11_error_handling/) — 栈哨兵、断言、CRC
- [Lesson 2: 汇编语言](../lesson_02_assembly/) — M0 指令集限制
- [ARM Cortex-M0 汇编指南](../docs/02_assembly.md)
