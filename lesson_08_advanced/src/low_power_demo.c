/*
 * =============================================================================
 * low_power_demo.c -- Cortex-M0 低功耗模式实战
 * =============================================================================
 *
 * Cortex-M0 支持两种睡眠模式, 通过 WFI/WFE 指令触发:
 *   - 普通睡眠 (Sleep): CPU 时钟停止, 外设继续运行
 *   - 深度睡眠 (Deep Sleep): CPU + 部分外设时钟停止 (取决于 SoC 实现)
 *
 * SCR (System Control Register) @ 0xE000ED10:
 *   bit 1: SLEEPONEXIT — ISR 返回后自动睡眠
 *   bit 2: SLEEPDEEP   — 1=深度睡眠, 0=普通睡眠
 *
 * QEMU 重要说明:
 *   QEMU 不模拟实际的功耗状态. WFI/WFE 在 QEMU 中立即返回 (不睡眠).
 *   但代码逻辑对真实硬件完全正确.
 *   在真实的 nRF51822 上:
 *     - WFI 使 CPU 进入睡眠, 功耗从 ~2.4mA 降到 ~2μA
 *     - 任何中断都可以唤醒 CPU
 *     - 唤醒延迟约 16 cycles
 * =============================================================================
 */

#include "semihosting.h"
#include <stdint.h>

/* SCB 寄存器地址 */
#define SCB_SCR     (*(volatile uint32_t *)0xE000ED10)

/* SysTick 寄存器 (用于 QEMU 兼容的唤醒演示) */
#define SYST_CSR    (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR    (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR    (*(volatile uint32_t *)0xE000E018)

/* --------------------------------------------------------------------------
 * 辅助: 读取 SCR 寄存器当前值
 * -------------------------------------------------------------------------- */
static uint32_t scr_read(void)
{
    return SCB_SCR;
}

static void scr_write_demo(void)
{
    /* 演示如何写 SCR 寄存器 (不实际修改, 避免意外进入深度睡眠) */
    volatile uint32_t current = SCB_SCR;
    sh_write0("    SCR register write demo: current value = ");
    sh_write_hex(current);
    sh_write0("\n");
    /* 实际写入: SCB_SCR = value; */
}

/* --------------------------------------------------------------------------
 * 演示 1: WFI (Wait For Interrupt) 基础
 * --------------------------------------------------------------------------
 *
 * WFI 指令使 CPU 立即进入睡眠模式.
 * CPU 停止执行, 直到以下事件发生:
 *   1. 任何中断 (即使被 PRIMASK 屏蔽, 但如果中断 pending 会唤醒)
 *   2. 调试事件 (调试器发起的唤醒)
 *   3. 复位
 *
 * 唤醒后, CPU 从 WFI 之后的第一条指令继续执行.
 *
 * QEMU 行为: WFI 在 QEMU 中立即返回 (NOP 等效).
 *           用 SysTick COUNTFLAG 演示唤醒检测逻辑.
 * -------------------------------------------------------------------------- */
static void demo_wfi_basic(void)
{
    sh_write0("  [3.1] WFI (Wait For Interrupt) Basics\n\n");

    sh_write0("    WFI Instruction:\n");
    sh_write0("      asm volatile(\"wfi\");\n\n");

    sh_write0("    Effect on real nRF51822 hardware:\n");
    sh_write0("      - CPU clock stops (gated off)\n");
    sh_write0("      - Current drops from ~2.4mA → ~2μA (1000x reduction!)\n");
    sh_write0("      - Peripherals continue running (UART, Timer, RTC...)\n");
    sh_write0("      - Any interrupt wakes the CPU\n");
    sh_write0("      - Wake-up latency: ~16 cycles\n\n");

    sh_write0("    Typical FreeRTOS idle task:\n");
    sh_write0("      void vPortSuppressTicksAndSleep(...) {\n");
    sh_write0("          __WFI();  // sleep until next SysTick interrupt\n");
    sh_write0("      }\n\n");

    /* 演示: 用 COUNTFLAG 模拟中断唤醒 (QEMU 兼容) */
    sh_write0("    Testing SysTick COUNTFLAG wake-up pattern:\n");

    /* 配置 SysTick: CLKSOURCE=CPU clock, 但不启用中断 */
    SYST_CSR = 0x00; /* 停止 */
    SYST_RVR = 0x000000FF; /* 短超时 (~16μs @ 16MHz, fast for QEMU testing) */
    SYST_CVR = 0x00; /* 清零 */
    SYST_CSR = 0x05; /* ENABLE | CLKSOURCE (CPU clock) -- no interrupt */

    sh_write0("      SysTick configured. CSR=");
    sh_write_hex(SYST_CSR);
    sh_write0("\n");

    /* 轮询 COUNTFLAG -- 模拟 "等待中断" */
    int wake_count = 0;
    for (int i = 0; i < 3; i++)
    {
        /* 在实际代码中, 这里会是 __WFI() */
        sh_write0("      [simulated WFI] zzz...\n");

        /* 模拟等待: 轮询 SysTick COUNTFLAG */
        while (!(SYST_CSR & (1U << 16)))
        {
            /* 在真实硬件上 CPU 此时在睡眠, 不消耗功耗 */
            /* 在 QEMU 中必须轮询 */
        }

        wake_count++;
        sh_write0("      Wake-up #");
        sh_write_dec(wake_count);
        sh_write0(" (COUNTFLAG=1)\n");

        /* 清除 COUNTFLAG: 读 CSR 写回 */
        uint32_t csr_val = SYST_CSR;
        SYST_CSR = csr_val; /* 写回清除 COUNTFLAG */
    }

    SYST_CSR = 0x00; /* 停止 SysTick */
    sh_write0("\n");
}

/* --------------------------------------------------------------------------
 * 演示 2: WFE (Wait For Event)
 * --------------------------------------------------------------------------
 *
 * WFE 与 WFI 类似, 但唤醒条件不同:
 *   WFI: 被中断唤醒
 *   WFE: 被"事件"唤醒 (包括中断和 SEV 指令)
 *
 * M0 的 WFE 与 M0+ 的区别很重要:
 *   - M0:  有 WFE/SEV 指令, 但没有 TXEV/RXEV 外部事件接口
 *           WFE 主要由中断唤醒, SEV 只能唤醒同一 CPU 的 WFE
 *   - M0+:  增加了外部事件信号 (TXEV/RXEV), 可用于外设事件唤醒
 *
 * WFE 的独特行为: 如果事件寄存器已置位, WFE 不会睡眠 (立即返回).
 * 这避免了 "missed wake-up" 问题.
 * -------------------------------------------------------------------------- */
static void demo_wfe_basic(void)
{
    sh_write0("  [3.2] WFE (Wait For Event)\n\n");

    sh_write0("    WFE Instruction:\n");
    sh_write0("      asm volatile(\"wfe\");\n\n");

    sh_write0("    WFE vs WFI Comparison:\n");
    sh_write0("    ┌──────────┬─────────────────────┬──────────────────────┐\n");
    sh_write0("    │ Feature  │ WFI                 │ WFE                  │\n");
    sh_write0("    ├──────────┼─────────────────────┼──────────────────────┤\n");
    sh_write0("    │ Wake by  │ Any interrupt       │ Interrupt OR event   │\n");
    sh_write0("    │ Register │ N/A                 │ Event register       │\n");
    sh_write0("    │ SEV      │ No effect           │ Wakes WFE            │\n");
    sh_write0("    │ TXEV/RXEV│ N/A (both M0/M0+)   │ M0: No, M0+: Yes     │\n");
    sh_write0("    │ Use case │ General idle        │ Spinlock / multi-core│\n");
    sh_write0("    └──────────┴─────────────────────┴──────────────────────┘\n\n");

    sh_write0("    Example: WFE used in a polling loop with SEV:\n");
    sh_write0("      // CPU A: waiting for data\n");
    sh_write0("      while (!data_ready) { __WFE(); }\n");
    sh_write0("      // CPU B: data producer\n");
    sh_write0("      data_ready = 1; __SEV(); // wake CPU A\n\n");

    sh_write0("    On single-core M0: WFI is usually the right choice.\n");
    sh_write0("    WFE is mainly for inter-CPU synchronization (rare on M0).\n\n");

    sh_write0("    WFE behavior in QEMU microbit:\n");
    sh_write0("      - WFE may hang in some QEMU versions (no event source)\n");
    sh_write0("      - WFI returns immediately (works in QEMU)\n");
    sh_write0("      - On real nRF51822: both WFI and WFE work correctly\n\n");

    /* 不在 QEMU 中实际执行 WFE -- 某些 QEMU 版本会永久挂起
     * 如需测试 WFE, 在真实硬件上取消下面的注释:
     *   __asm__ volatile("wfe" ::: "memory");
     *   sh_write0("    WFE returned.\n\n");
     */
}

/* --------------------------------------------------------------------------
 * 演示 3: Sleep-on-Exit 模式
 * --------------------------------------------------------------------------
 *
 * Sleep-on-Exit 是一种极低功耗模式, 适用于纯中断驱动的系统:
 *   1. 系统初始化后, 执行 WFI 进入睡眠
 *   2. 中断到来 → CPU 唤醒, 执行 ISR
 *   3. ISR 返回时 → CPU 自动回到睡眠 (无需返回 main!)
 *   4. 下一个中断到来 → 重复
 *
 * 启用: SCR.SLEEPONEXIT = 1
 *
 * 这种模式下, main() 的 while(1) 永远不会执行 (除了第一次 WFI).
 * 系统完全由中断驱动.
 * -------------------------------------------------------------------------- */
static void demo_sleep_on_exit(void)
{
    sh_write0("  [3.3] Sleep-on-Exit Mode\n\n");

    uint32_t scr = scr_read();
    sh_write0("    Current SCR = ");
    sh_write_hex(scr);
    sh_write0("\n\n");

    sh_write0("    Sleep-on-Exit workflow:\n");
    sh_write0("      main() → WFI → [sleep] → interrupt → ISR → [auto-sleep]\n");
    sh_write0("                                      ↑                    │\n");
    sh_write0("                                      └────────────────────┘\n\n");

    sh_write0("    Configuration:\n");
    sh_write0("      SCR.SLEEPONEXIT (bit 1) = 1\n");
    sh_write0("      SCR.SLEEPDEEP   (bit 2) = 0  (normal sleep, not deep)\n\n");

    /* 展示如何配置 (不真正启用, 因为需要中断才能唤醒) */
    sh_write0("    To enable (uncomment in real code):\n");
    sh_write0("      uint32_t scr = SCB->SCR;\n");
    sh_write0("      scr |= (1 << 1);  // set SLEEPONEXIT\n");
    sh_write0("      SCB->SCR = scr;\n");
    sh_write0("      __WFI();  // now CPU sleeps; wakes only for interrupts\n\n");

    scr_write_demo();

    sh_write0("    Use case: battery-powered sensor node.\n");
    sh_write0("      - CPU sleeps 99.9% of the time\n");
    sh_write0("      - RTC wakes CPU every N seconds to read sensor\n");
    sh_write0("      - ISR reads sensor, queues data, returns → auto-sleep\n");
    sh_write0("      - Average current: ~3μA (years on a coin cell!)\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 4: 低功耗设计检查清单
 * -------------------------------------------------------------------------- */
static void demo_low_power_checklist(void)
{
    sh_write0("  [3.4] Low-Power Design Checklist\n\n");

    sh_write0("    ┌─────────────────────────────────────────────────────────┐\n");
    sh_write0("    │ Low-Power Checklist for Cortex-M0                       │\n");
    sh_write0("    ├─────────────────────────────────────────────────────────┤\n");
    sh_write0("    │ [ ] Use WFI in idle loop (not busy-wait)               │\n");
    sh_write0("    │ [ ] Clock peripherals only when needed (clock gating)   │\n");
    sh_write0("    │ [ ] Use lowest acceptable CPU clock frequency           │\n");
    sh_write0("    │ [ ] Use interrupt-driven I/O (not polling)              │\n");
    sh_write0("    │ [ ] Configure unused pins as analog input (lowest I)    │\n");
    sh_write0("    │ [ ] Enable Sleep-on-Exit for ISR-only systems           │\n");
    sh_write0("    │ [ ] Use RTC instead of SysTick for long idle periods    │\n");
    sh_write0("    │ [ ] Batch radio transmissions (TX is expensive!)        │\n");
    sh_write0("    │ [ ] Use FreeRTOS tickless idle (configUSE_TICKLESS_IDLE)│\n");
    sh_write0("    │ [ ] Disable pull-up/down resistors on unused pins       │\n");
    sh_write0("    │ [ ] Use fixed-point math instead of floating point      │\n");
    sh_write0("    │ [ ] Profile: measure actual current with a multimeter   │\n");
    sh_write0("    └─────────────────────────────────────────────────────────┘\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: nRF51 特定低功耗信息
 * -------------------------------------------------------------------------- */
static void demo_nrf51_power_modes(void)
{
    sh_write0("  [3.5] nRF51822 Power Modes\n\n");

    sh_write0("    nRF51 power consumption (approximate):\n\n");
    sh_write0("    ┌──────────────────────┬──────────┬─────────────────────┐\n");
    sh_write0("    │ Mode                 │ Current  │ Notes               │\n");
    sh_write0("    ├──────────────────────┼──────────┼─────────────────────┤\n");
    sh_write0("    │ CPU running @ 16MHz  │ ~2.4 mA  │ Normal operation    │\n");
    sh_write0("    │ CPU running @ 4MHz   │ ~1.0 mA  │ Lower clock         │\n");
    sh_write0("    │ WFI sleep (RAM on)   │ ~2.0 μA  │ RAM retained        │\n");
    sh_write0("    │ WFI deep sleep       │ ~0.6 μA  │ RAM lost (use .noinit)│\n");
    sh_write0("    │ System OFF           │ ~0.3 μA  │ Wake by reset/GPIO  │\n");
    sh_write0("    │ TX @ 0dBm (BLE)      │ ~5.3 mA  │ Radio is expensive! │\n");
    sh_write0("    │ TX @ +4dBm (BLE)     │ ~10.5 mA │ Max TX power        │\n");
    sh_write0("    └──────────────────────┴──────────┴─────────────────────┘\n\n");

    sh_write0("    A CR2032 coin cell has ~225 mAh capacity.\n");
    sh_write0("    - Running continuously:  225/2.4  ≈ 94 hours   (~4 days)\n");
    sh_write0("    - WFI sleep (RTC wake):  225/0.003 ≈ 75,000 hrs (~8.5 years!)\n\n");
}

/* ==========================================================================
 * 入口
 * ========================================================================== */
void low_power_demo_entry(void)
{
    sh_write0("\n");
    sh_write0("========================================================\n");
    sh_write0("  [Module 3] Low-Power Modes on Cortex-M0\n");
    sh_write0("========================================================\n\n");

    sh_write0("  IMPORTANT: QEMU does NOT simulate actual power states.\n");
    sh_write0("  WFI/WFE return immediately in QEMU (no sleep/wake).\n");
    sh_write0("  The code shown is correct for real nRF51822 hardware.\n\n");

    demo_wfi_basic();
    demo_wfe_basic();
    demo_sleep_on_exit();
    demo_nrf51_power_modes();
    demo_low_power_checklist();

    sh_write0("  Low-power demo complete.\n\n");
}
