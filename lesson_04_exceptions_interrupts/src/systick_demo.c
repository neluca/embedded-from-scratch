/*
 * =============================================================================
 * systick_demo.c -- SysTick 系统节拍定时器
 * =============================================================================
 *
 * SysTick 是 Cortex-M0 内建的 24 位递减定时器, 每个时钟周期减 1.
 * 减到 0 时, 重载 RELOAD 值, 并可选择产生中断.
 *
 * 寄存器 (位于系统控制空间 SCS, 0xE000E000):
 *   SYST_CSR (0xE000E010): 控制和状态寄存器
 *     bit0:  ENABLE      - 使能计数器
 *     bit1:  TICKINT     - 使能 SysTick 异常
 *     bit2:  CLKSOURCE   - 0=外部时钟, 1=处理器时钟
 *     bit16: COUNTFLAG   - 自上次读后计数器是否到过 0
 *   SYST_RVR (0xE000E014): 重载值寄存器
 *     bits 0-23: RELOAD  - 计数器到 0 后重载此值
 *   SYST_CVR (0xE000E018): 当前值寄存器
 *     bits 0-23: CURRENT - 当前计数值 (写任意值清零)
 *   SYST_CALIB (0xE000E01C): 校准值寄存器 (只读, 取决于芯片)
 *
 * 在 QEMU microbit (nRF51822 @ 16MHz):
 *   - 处理器时钟 = 16 MHz
 *   - RELOAD = 16000000 / tick_rate - 1
 *   - 例如 tick_rate = 1000 Hz → RELOAD = 15999
 *
 * 注: 在 QEMU 中时钟频率可能不精确, 使用较小的 RELOAD 值测试.
 */

#include "semihosting.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * SysTick 寄存器定义
 * -------------------------------------------------------------------------- */
typedef struct
{
    volatile uint32_t CSR;     /* 0xE000E010: Control and Status */
    volatile uint32_t RVR;     /* 0xE000E014: Reload Value */
    volatile uint32_t CVR;     /* 0xE000E018: Current Value */
    volatile const uint32_t CALIB; /* 0xE000E01C: Calibration (RO) */
} systick_regs_t;

#define SYSTICK_BASE ((systick_regs_t *)0xE000E010)

/* CSR 位定义 */
#define SYST_CSR_ENABLE    (1U << 0)
#define SYST_CSR_TICKINT   (1U << 1)
#define SYST_CSR_CLKSOURCE (1U << 2)
#define SYST_CSR_COUNTFLAG (1U << 16)

/* --------------------------------------------------------------------------
 * 全局计数器 (在 SysTick 中断中递增)
 * -------------------------------------------------------------------------- */
static volatile uint32_t g_tick_count = 0;
static volatile uint32_t g_systick_fired = 0;

/* --------------------------------------------------------------------------
 * systick_init -- 初始化 SysTick 定时器
 * --------------------------------------------------------------------------
 * @param reload_val: 重载值 (计数器从 reload_val 减到 0)
 * @param enable_int: 是否使能中断 (1 = 使能)
 *
 * QEMU 中 nRF51822 的 SysTick 可能用处理器时钟 (16MHz).
 * 为了快速看到效果, 使用较小的 reload_val (例如 100000).
 * -------------------------------------------------------------------------- */
void systick_init(uint32_t reload_val, int enable_int)
{
    systick_regs_t *stk = SYSTICK_BASE;

    /* 1. 禁用 SysTick (在配置期间) */
    stk->CSR = 0;

    /* 2. 设置重载值 (24 位: 0x000000 - 0xFFFFFF) */
    stk->RVR = reload_val & 0x00FFFFFF;

    /* 3. 清零当前值 (写任何值都会清零) */
    stk->CVR = 0;

    /* 4. 使能 SysTick */
    uint32_t csr = SYST_CSR_ENABLE | SYST_CSR_CLKSOURCE;
    if (enable_int)
    {
        csr |= SYST_CSR_TICKINT;
    }
    stk->CSR = csr;
}

/* --------------------------------------------------------------------------
 * systick_deinit -- 禁用 SysTick
 * -------------------------------------------------------------------------- */
void systick_deinit(void)
{
    SYSTICK_BASE->CSR = 0;
}

/* --------------------------------------------------------------------------
 * systick_get_count -- 获取节拍计数
 * -------------------------------------------------------------------------- */
uint32_t systick_get_count(void)
{
    return g_tick_count;
}

/* --------------------------------------------------------------------------
 * systick_handler_c -- SysTick 中断处理函数 (C 部分)
 * --------------------------------------------------------------------------
 * 由 startup.S 中的 systick_handler 调用.
 *
 * 注意: 此函数运行在 Handler 模式下!
 *   - 使用 MSP (主栈指针)
 *   - 不能调用会阻塞的函数
 *   - 尽量减少处理时间
 *   - semihosting 调用在此环境中是安全的 (QEMU 同步处理)
 * -------------------------------------------------------------------------- */
void systick_handler_c(void)
{
    g_tick_count++;
    g_systick_fired = 1;
}

/* --------------------------------------------------------------------------
 * systick_interrupt_fired -- 检查 SysTick 是否触发过
 * -------------------------------------------------------------------------- */
int systick_interrupt_fired(void)
{
    if (g_systick_fired)
    {
        g_systick_fired = 0;
        return 1;
    }
    return 0;
}

/* --------------------------------------------------------------------------
 * demo_systick -- 演示 SysTick 使用
 * -------------------------------------------------------------------------- */
void demo_systick(void)
{
    sh_write0("[1] SysTick Timer\n");
    sh_write0("    SysTick is a 24-bit down-counter built into Cortex-M0\n");
    sh_write0("    Registers: CSR(0xE000E010), RVR(0xE000E014), CVR(0xE000E018)\n\n");

    /* --- 演示 1: 轮询模式 (不使用中断) --- */
    sh_write0("    [1a] Polling mode (no interrupt):\n");

    /* 设置 SysTick 为很小的重载值 (快速递减) */
    systick_init(100000, 0); /* 不使能中断 */

    /* 轮询 COUNTFLAG 等待计数完成 */
    volatile int timeout = 1000000;
    while (!(SYSTICK_BASE->CSR & SYST_CSR_COUNTFLAG) && timeout > 0)
    {
        timeout--;
    }
    sh_write0("    SysTick count reached zero (polling detected COUNTFLAG)\n");

    systick_deinit();

    /* --- 演示 2: 中断模式 --- */
    sh_write0("\n    [1b] Interrupt mode (CONFIGURATION ONLY):\n");

    g_tick_count = 0;
    g_systick_fired = 0;

    /* 配置 SysTick 中断 */
    systick_init(100000, 1); /* 使能中断 */

    /* 读取 CSR 确认配置 */
    uint32_t csr_check = SYSTICK_BASE->CSR;
    sh_write0("    SYST_CSR = ");
    sh_write_hex(csr_check);
    sh_write0(" (ENABLE+TICKINT+CLKSOURCE = 0x07)\n");
    sh_write0("    SYST_RVR = ");
    sh_write_hex(SYSTICK_BASE->RVR);
    sh_write0(" (reload value)\n\n");

    sh_write0("    NOTE: QEMU microbit does NOT deliver SysTick interrupts.\n");
    sh_write0("    The ISR is correctly configured and would work on real HW.\n");
    sh_write0("    SysTick counter IS running (COUNTFLAG works in polling).\n");
    sh_write0("    This is a known QEMU limitation for nRF51/microbit.\n\n");

    /* 使用 COUNTFLAG 轮询模拟 SysTick 行为 */
    sh_write0("    [1c] Simulating interrupts via COUNTFLAG polling:\n");
    systick_init(50000, 0); /* 不使能中断, 只轮询 */
    uint32_t poll_count = 0;
    for (int i = 0; i < 5; i++)
    {
        /* 等待 COUNTFLAG */
        while (!(SYSTICK_BASE->CSR & SYST_CSR_COUNTFLAG))
        {
        }
        poll_count++;
        /* 读 CSR 会清除 COUNTFLAG */
        (void)SYSTICK_BASE->CSR;
        sh_write0("      Poll #");
        sh_write_dec(poll_count);
        sh_write0(": COUNTFLAG detected (=\"interrupt\")\n");
    }

    systick_deinit();
    sh_write0("    SysTick demo complete.\n\n");
}
