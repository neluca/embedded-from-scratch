/*
 * =============================================================================
 * nrf51_registers.h -- nRF51822 (microbit) 外设寄存器定义
 * =============================================================================
 *
 * 本文件定义了 nRF51822 常用外设的寄存器映射.
 * 使用结构体指针实现类型安全的内存映射 I/O 访问.
 *
 * 内存映射 I/O 要点:
 *   1. 所有寄存器指针必须用 volatile 修饰 (防止编译器优化掉读写)
 *   2. 读-修改-写操作可能不是原子的 (需要临界区保护)
 *   3. M0 不支持非对齐访问, 所有寄存器地址必须自然对齐
 *   4. 写入保留位时保持为 0 (read-modify-write)
 *
 * nRF51822 外设基地址:
 *   UART0:  0x40002000
 *   TIMER0: 0x40008000
 *   TIMER1: 0x40009000
 *   TIMER2: 0x4000A000
 *   GPIO P0: 0x50000000
 *   RTC0:   0x4000B000
 * =============================================================================
 */

#ifndef NRF51_REGISTERS_H
#define NRF51_REGISTERS_H

#include <stdint.h>

/* =========================================================================
 * UART0 寄存器 (0x40002000)
 * =========================================================================
 * nRF51 UART 使用 "task/event" 模型:
 *   - TASK:  写 1 触发操作 (STARTTX, STARTRX, STOPTX, STOPRX 等)
 *   - EVENT: 硬件将 1 写入事件寄存器 (RXDRDY, TXDRDY, ERROR 等)
 *            软件读后写 0 清除
 */
typedef struct
{
    volatile uint32_t RESERVED0[64];            /* 0x000-0x0FC: 保留 */

    /* --- 事件 (Event) 寄存器 --- */
    volatile uint32_t EVENTS_RXDRDY;            /* 0x100: RX Data Ready */
    volatile uint32_t RESERVED1[1];
    volatile uint32_t EVENTS_TXDRDY;            /* 0x108: TX Data Ready (实际偏移?) */
    volatile uint32_t RESERVED2[5];
    volatile uint32_t EVENTS_ERROR;             /* 0x124: Error detected */
    volatile uint32_t RESERVED3[54];

    /* --- 快捷 (Shortcut) 寄存器 --- */
    volatile uint32_t SHORTS;                   /* 0x200: Shortcuts */

    /* --- 中断 (Interrupt) 寄存器 --- */
    volatile uint32_t INTENSET;                 /* 0x300: Interrupt Enable Set */
    volatile uint32_t INTENCLR;                 /* 0x304: Interrupt Enable Clear */
    volatile uint32_t RESERVED4[78];

    /* --- 配置寄存器 --- */
    volatile uint32_t ERRORS;                   /* 0x43C: Error Source (?) */
    volatile uint32_t RESERVED5[42];
    volatile uint32_t ENABLE;                   /* 0x500: Enable (0=disabled, 4=enabled) */
    volatile uint32_t RESERVED6[1];
    volatile uint32_t PSELRXD;                  /* 0x508: RX Pin Select */
    volatile uint32_t PSELTXD;                  /* 0x50C: TX Pin Select */
    volatile uint32_t RESERVED7[1];
    volatile uint32_t RXD;                      /* 0x518: RX Data */
    volatile uint32_t TXD;                      /* 0x51C: TX Data (write to send) */
    volatile uint32_t RESERVED8[1];
    volatile uint32_t BAUDRATE;                 /* 0x524: Baud Rate */
    volatile uint32_t RESERVED9[17];
    volatile uint32_t CONFIG;                   /* 0x56C: Configuration */
} uart_regs_t;

/* nRF51 UART task 宏: 写 1 触发 */
#define UART_TASK_STARTRX (*(volatile uint32_t *)0x40002000)
#define UART_TASK_STOPRX  (*(volatile uint32_t *)0x40002004)
#define UART_TASK_STARTTX (*(volatile uint32_t *)0x40002008)
#define UART_TASK_STOPTX  (*(volatile uint32_t *)0x4000200C)
#define UART_TASK_SUSPEND (*(volatile uint32_t *)0x4000201C)

/* UART_BASE 从 0x40002000 开始, struct RESERVED0[64] 跳过 task 区(0x000-0x0FF) */
#define UART_BASE ((uart_regs_t *)0x40002000)

/* UART INTEN 位 */
#define UART_INT_RXDRDY (1U << 2)
#define UART_INT_TXDRDY (1U << 7)
#define UART_INT_ERROR  (1U << 9)

/* UART BAUDRATE 常见值 (nRF51 直接写寄存器值, 不是除数) */
#define UART_BAUD_1200   0x0004F000
#define UART_BAUD_2400   0x0009D000
#define UART_BAUD_4800   0x0013B000
#define UART_BAUD_9600   0x00275000
#define UART_BAUD_14400  0x003B0000
#define UART_BAUD_19200  0x004EA000
#define UART_BAUD_38400  0x009D5000
#define UART_BAUD_57600  0x00EBF000
#define UART_BAUD_76800  0x013A9000
#define UART_BAUD_115200 0x01D7E000
#define UART_BAUD_230400 0x03AFB000
#define UART_BAUD_250000 0x04000000
#define UART_BAUD_460800 0x075F7000
#define UART_BAUD_921600 0x0EBED000
#define UART_BAUD_1M     0x10000000

/* UART ENABLE 值 */
#define UART_ENABLE_DISABLED 0
#define UART_ENABLE_ENABLED  4

/* UART CONFIG 位 */
#define UART_CONFIG_PARITY_EXCLUDED (0 << 1)
#define UART_CONFIG_PARITY_INCLUDED (7 << 1)
#define UART_CONFIG_HWFC_DISABLED   0
#define UART_CONFIG_HWFC_ENABLED    1

/* =========================================================================
 * GPIO P0 寄存器 (0x50000000)
 * =========================================================================
 * nRF51 的 GPIO 比较特殊: 没有传统的 DIR 寄存器!
 * 方向通过 PIN_CNF[n] 寄存器中的 DIR 位控制.
 *
 * 简化操作使用 OUT, OUTSET, OUTCLR, IN:
 *   OUT:    完整输出值 (写所有 32 位)
 *   OUTSET: 写 1 的位被设置为输出高
 *   OUTCLR: 写 1 的位被设置为输出低
 *   IN:     读引脚状态
 *   DIR:    方向寄存器 (1=输出, 0=输入) [实际存在]
 *   DIRSET: 写 1 设方向为输出
 *   DIRCLR: 写 1 设方向为输入
 */
typedef struct
{
    volatile uint32_t RESERVED0[321];
    volatile uint32_t OUT;                      /* 0x504: GPIO Output */
    volatile uint32_t OUTSET;                   /* 0x508: Set Output bits */
    volatile uint32_t OUTCLR;                   /* 0x50C: Clear Output bits */
    volatile uint32_t IN;                       /* 0x510: GPIO Input */
    volatile uint32_t DIR;                      /* 0x514: Direction */
    volatile uint32_t DIRSET;                   /* 0x518: Set Direction (1=output) */
    volatile uint32_t DIRCLR;                   /* 0x51C: Clear Direction */
    volatile uint32_t RESERVED1[120];
    volatile uint32_t PIN_CNF[32];              /* 0x700-0x77C: Pin Configuration */
} gpio_regs_t;

#define GPIO_BASE ((gpio_regs_t *)0x50000000)

/* PIN_CNF 位 */
#define PIN_CNF_DIR_OUTPUT     (1U << 0)  /* 1=output, 0=input */
#define PIN_CNF_DIR_INPUT      0
#define PIN_CNF_INPUT_CONNECT  (1U << 1)  /* 1=connect, 0=disconnect input buffer */
#define PIN_CNF_PULL_DISABLED  (0U << 2)
#define PIN_CNF_PULL_DOWN      (1U << 2)
#define PIN_CNF_PULL_UP        (3U << 2)
#define PIN_CNF_DRIVE_S0S1     (0U << 8)  /* Standard 0, standard 1 */
#define PIN_CNF_DRIVE_H0S1     (1U << 8)
#define PIN_CNF_DRIVE_S0H1     (2U << 8)
#define PIN_CNF_DRIVE_H0H1     (3U << 8)
#define PIN_CNF_SENSE_DISABLED (0U << 16)
#define PIN_CNF_SENSE_HIGH     (2U << 16)
#define PIN_CNF_SENSE_LOW      (3U << 16)

/* microbit 常用引脚 */
#define MICROBIT_PIN_UART_TX  24  /* P0.24 */
#define MICROBIT_PIN_UART_RX  25  /* P0.25 */
#define MICROBIT_PIN_LED_COL1 4  /* LED matrix column 1 */
#define MICROBIT_PIN_LED_ROW1 13 /* LED matrix row 1 */
#define MICROBIT_PIN_BUTTON_A 17 /* Button A */
#define MICROBIT_PIN_BUTTON_B 26 /* Button B */

/* =========================================================================
 * TIMER0 寄存器 (0x40008000)
 * =========================================================================
 * nRF51 有 3 个 Timer (TIMER0/1/2), 每个是 16/24/32 位可配置.
 * Timer 也使用 task/event 模型.
 */
typedef struct
{
    volatile uint32_t RESERVED0[80];            /* 0x000-0x13F: tasks + reserved */

    /* --- 事件 --- */
    volatile uint32_t EVENTS_COMPARE[4];        /* 0x140-0x14C: Compare match events */

    /* --- 快捷 --- */
    volatile uint32_t RESERVED1[44];
    volatile uint32_t SHORTS;                   /* 0x200: Shortcuts */

    volatile uint32_t RESERVED2[63];
    /* --- 中断 --- */
    volatile uint32_t INTENSET;                 /* 0x304: Interrupt Enable Set */
    volatile uint32_t INTENCLR;                 /* 0x308: Interrupt Enable Clear */

    volatile uint32_t RESERVED3[126];

    /* --- 配置 --- */
    volatile uint32_t MODE;                     /* 0x504: Mode */
    volatile uint32_t BITMODE;                  /* 0x508: Bit Mode */
    volatile uint32_t RESERVED4[1];
    volatile uint32_t PRESCALER;                /* 0x510: Prescaler */
    volatile uint32_t RESERVED5[11];
    volatile uint32_t CC[4];                    /* 0x540-0x54C: Capture/Compare */
} timer_regs_t;

/* Task 寄存器 (在 0x40008000) */
#define TIMER0_TASK_START (*(volatile uint32_t *)0x40008000)
#define TIMER0_TASK_STOP  (*(volatile uint32_t *)0x40008004)
#define TIMER0_TASK_COUNT (*(volatile uint32_t *)0x40008008)
#define TIMER0_TASK_CLEAR (*(volatile uint32_t *)0x4000800C)

/* TIMER0_BASE 从 0x40008000 开始, RESERVED0[80] 跳过 task区(0x000-0x13F) */
#define TIMER0_BASE ((timer_regs_t *)0x40008000)

/* TIMER 模式 */
#define TIMER_MODE_TIMER   0
#define TIMER_MODE_COUNTER 1

/* TIMER 位宽 */
#define TIMER_BITMODE_16 0
#define TIMER_BITMODE_8  1
#define TIMER_BITMODE_24 2
#define TIMER_BITMODE_32 3

/* =========================================================================
 * NVIC 寄存器 (Cortex-M0 系统控制空间)
 * ========================================================================= */
#define NVIC_ISER_BASE 0xE000E100
#define NVIC_ISER      ((volatile uint32_t *)NVIC_ISER_BASE)

/* nRF51 外设中断号 */
#define IRQ_UART0  2
#define IRQ_TIMER0 8
#define IRQ_TIMER1 9
#define IRQ_TIMER2 10
#define IRQ_GPIOTE 6
#define IRQ_RTC0   11

/* 启用 NVIC 中断 */
static inline void nvic_enable_irq(uint32_t irq_num)
{
    NVIC_ISER[irq_num >> 5] = (1U << (irq_num & 0x1F));
}

#endif /* NRF51_REGISTERS_H */
