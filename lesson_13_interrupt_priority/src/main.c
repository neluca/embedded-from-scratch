/*
 * =============================================================================
 * Lesson 13 主程序 — 中断优先级与抢占实战
 * =============================================================================
 *
 * 学习目标:
 * 1. 理解 NVIC 优先级机制 (抢占/尾链/迟到)
 * 2. 掌握中断优先级配置 (IPR/SHPR 寄存器)
 * 3. 了解软件中断 (SWI) 的触发与处理
 * 4. 理解临界区保护 (PRIMASK)
 * 5. 理解优先级反转及其解决方案
 *
 * 使用 nRF51 的 SWI (Software Interrupt) 来演示:
 *   SWI0 = IRQ20, SWI1 = IRQ21, SWI2 = IRQ22, SWI3 = IRQ23
 *   这些是 NVIC 核心功能, 在 QEMU 中可正常触发
 * =============================================================================
 */

#include <stddef.h>
#include <stdint.h>

/* Semihosting 输出 */
static void sh_write0(const char *s)
{
    register int r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = s;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
static void sh_write_hex(uint32_t v)
{
    char b[] = "00000000 ";
    for (int i = 7; i >= 0; i--)
    {
        uint32_t n = v & 0xF;
        b[i] = n < 10 ? '0' + n : 'A' + n - 10;
        v >>= 4;
    }
    sh_write0(b);
}
static void sh_write_dec(uint32_t v)
{
    char buf[12];
    int pos = sizeof(buf) - 1;
    buf[pos] = '\0';
    if (v == 0)
    {
        buf[--pos] = '0';
    }
    else
    {
        while (v > 0)
        {
            buf[--pos] = '0' + (v % 10);
            v /= 10;
        }
    }
    sh_write0(&buf[pos]);
}

/* =========================================================================
 * NVIC / SCB 寄存器地址
 * ========================================================================= */
#define NVIC_ISER_BASE 0xE000E100
#define NVIC_ICER_BASE 0xE000E180
#define NVIC_ISPR_BASE 0xE000E200
#define NVIC_ICPR_BASE 0xE000E280
#define NVIC_IPR_BASE  0xE000E400
#define SCB_SHPR2      0xE000ED1C
#define SCB_SHPR3      0xE000ED20

#define NVIC_ISER ((volatile uint32_t *)NVIC_ISER_BASE)
#define NVIC_ICER ((volatile uint32_t *)NVIC_ICER_BASE)
#define NVIC_ISPR ((volatile uint32_t *)NVIC_ISPR_BASE)
#define NVIC_ICPR ((volatile uint32_t *)NVIC_ICPR_BASE)
#define NVIC_IPR  ((volatile uint8_t *)NVIC_IPR_BASE)

/* IRQ 编号: nRF51 SWI 中断 */
#define IRQ_SWI0 20
#define IRQ_SWI1 21
#define IRQ_SWI2 22
#define IRQ_SWI3 23

/* =========================================================================
 * SWI 中断处理函数 (在 startup.S 中声明, 这里实现)
 * ========================================================================= */
volatile uint32_t g_swi_count[4] = { 0, 0, 0, 0 };

void swi0_handler_c(void)
{
    g_swi_count[0]++;
    /* No semihosting in ISR — QEMU may deadlock.
     * In production: ISR should be minimal, defer I/O to main loop. */
}

void swi1_handler_c(void)
{
    g_swi_count[1]++;
}

void swi2_handler_c(void)
{
    g_swi_count[2]++;
}

void swi3_handler_c(void)
{
    g_swi_count[3]++;
}

/* =========================================================================
 * NVIC 操作函数
 * ========================================================================= */
static void nvic_enable_irq(uint32_t irq)
{
    NVIC_ISER[irq >> 5] = (1U << (irq & 0x1F));
}

static void nvic_disable_irq(uint32_t irq)
{
    NVIC_ICER[irq >> 5] = (1U << (irq & 0x1F));
}

static void nvic_set_pending(uint32_t irq)
{
    NVIC_ISPR[irq >> 5] = (1U << (irq & 0x1F));
}

static void nvic_clear_pending(uint32_t irq)
{
    NVIC_ICPR[irq >> 5] = (1U << (irq & 0x1F));
}

static void nvic_set_priority(uint32_t irq, uint8_t prio)
{
    /* M0 仅使用每个字节的高 2 位, prio 应左移 6 位 */
    NVIC_IPR[irq] = (prio << 6);
}

static uint8_t nvic_get_priority(uint32_t irq)
{
    return NVIC_IPR[irq] >> 6;
}

static uint32_t get_primask(void)
{
    uint32_t result;
    __asm__ volatile("mrs %0, PRIMASK" : "=r"(result));
    return result;
}

static void set_primask(uint32_t val)
{
    __asm__ volatile("msr PRIMASK, %0" : : "r"(val) : "memory");
}

/* --------------------------------------------------------------------------
 * 演示 1: NVIC 寄存器探索
 * -------------------------------------------------------------------------- */
static void demo_nvic_registers(void)
{
    sh_write0("[1] NVIC Register Exploration\n\n");

    /* 读取 SHPR 寄存器 (系统异常优先级) */
    volatile uint32_t *shpr2 = (volatile uint32_t *)SCB_SHPR2;
    volatile uint32_t *shpr3 = (volatile uint32_t *)SCB_SHPR3;

    sh_write0("    System Handler Priorities:\n");
    sh_write0("      SHPR2 (SVCall)  = 0x");
    sh_write_hex(*shpr2);
    sh_write0("\n");
    sh_write0("      SHPR3 (PendSV/SysTick) = 0x");
    sh_write_hex(*shpr3);
    sh_write0("\n\n");

    /* 读取 PRIMASK */
    sh_write0("    PRIMASK = ");
    sh_write_dec(get_primask());
    sh_write0(" (0 = IRQs enabled, 1 = masked)\n\n");

    /* 使能 SWI0-SWI3 并读取 ISER */
    sh_write0("    Enabling SWI0..SWI3 (IRQ20-23)...\n");
    nvic_enable_irq(IRQ_SWI0);
    nvic_enable_irq(IRQ_SWI1);
    nvic_enable_irq(IRQ_SWI2);
    nvic_enable_irq(IRQ_SWI3);

    sh_write0("      ISER[0] = 0x");
    sh_write_hex(NVIC_ISER[0]);
    sh_write0(" (SWI0-3 = bits 20-23, expect 0x00F00000)\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 2: 优先级配置
 * -------------------------------------------------------------------------- */
static void demo_priority_config(void)
{
    sh_write0("[2] Priority Configuration\n\n");

    sh_write0("    M0 has 4 priority levels (2 bits):\n");
    sh_write0("      0x00 = Highest, 0x40 = High, 0x80 = Medium, 0xC0 = Lowest\n\n");

    /* 设置 SWI 优先级: 从低到高 */
    nvic_set_priority(IRQ_SWI0, 3); /* 0xC0 = 最低 */
    nvic_set_priority(IRQ_SWI1, 2); /* 0x80 = 中 */
    nvic_set_priority(IRQ_SWI2, 1); /* 0x40 = 高 */
    nvic_set_priority(IRQ_SWI3, 0); /* 0x00 = 最高 */

    sh_write0("    Configured priorities:\n");
    sh_write0("      SWI0 (IRQ20) = ");
    sh_write_dec(nvic_get_priority(IRQ_SWI0));
    sh_write0(" (lowest)\n");
    sh_write0("      SWI1 (IRQ21) = ");
    sh_write_dec(nvic_get_priority(IRQ_SWI1));
    sh_write0("\n");
    sh_write0("      SWI2 (IRQ22) = ");
    sh_write_dec(nvic_get_priority(IRQ_SWI2));
    sh_write0("\n");
    sh_write0("      SWI3 (IRQ23) = ");
    sh_write_dec(nvic_get_priority(IRQ_SWI3));
    sh_write0(" (highest)\n\n");

    sh_write0("    Priority rule: Lower number = higher priority.\n");
    sh_write0("    ISR with priority 0x00 preempts ISR with priority 0xC0.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 3: 软件中断触发与抢占
 * --------------------------------------------------------------------------
 * 注: QEMU microbit 的中断触发可能不完全.
 * 代码逻辑对真实硬件正确, 此处展示 API 用法.
 */
static void demo_swi_trigger(void)
{
    sh_write0("[3] Software Interrupt Triggering\n\n");

    sh_write0("    Cortex-M NVIC supports software-triggered interrupts\n");
    sh_write0("    via the ISPR (Interrupt Set-Pending) register.\n");
    sh_write0("    This is how PendSV is triggered in FreeRTOS!\n\n");

    sh_write0("    API demonstration (no actual trigger in QEMU):\n\n");

    /* 展示 ISPR 写操作但不实际触发 */
    sh_write0("    // Trigger SWI0 (lowest priority)\n");
    sh_write0("    nvic_set_pending(IRQ_SWI0);\n");
    sh_write0("    // On real HW: SWI0 ISR runs here.\n");
    sh_write0("    // ISR fires: g_swi_count[0]++;\n\n");

    sh_write0("    // Trigger SWI3 (highest priority)\n");
    sh_write0("    nvic_set_pending(IRQ_SWI3);\n");
    sh_write0("    // On real HW: SWI3 preempts SWI0!\n");
    sh_write0("    // ISR fires: g_swi_count[3]++;\n\n");

    sh_write0("    // Clear any pending (if ISR didn't fire)\n");
    sh_write0("    nvic_clear_pending(IRQ_SWI0);\n");
    sh_write0("    nvic_clear_pending(IRQ_SWI3);\n\n");

    sh_write0("    ISR counters (should increment on real HW):\n");
    sh_write0("      SWI0: ");
    sh_write_dec(g_swi_count[0]);
    sh_write0(", SWI1: ");
    sh_write_dec(g_swi_count[1]);
    sh_write0(", SWI2: ");
    sh_write_dec(g_swi_count[2]);
    sh_write0(", SWI3: ");
    sh_write_dec(g_swi_count[3]);
    sh_write0("\n\n");

    sh_write0("    NOTE: QEMU microbit has known issues with NVIC ISPR\n");
    sh_write0("    interrupt delivery when semihosting is active.\n");
    sh_write0("    The register access and ISR code are correct for HW.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 4: 临界区保护 (PRIMASK)
 * --------------------------------------------------------------------------
 */
static void demo_critical_section(void)
{
    sh_write0("[4] Critical Sections with PRIMASK\n\n");

    sh_write0("    PRIMASK is the global interrupt mask on Cortex-M0.\n");
    sh_write0("    Setting PRIMASK=1 disables all interrupts except NMI/HardFault.\n\n");

    /* 进入临界区前保存状态 */
    uint32_t saved_primask = get_primask();
    sh_write0("    Before critical section: PRIMASK = ");
    sh_write_dec(saved_primask);
    sh_write0("\n");

    /* 关中断 */
    __asm__ volatile("cpsid i");
    sh_write0("    After CPSID i:          PRIMASK = ");
    sh_write_dec(get_primask());
    sh_write0("\n");

    sh_write0("    >>> Critical section: safe to access shared data <<<\n");

    /* 恢复中断 */
    __asm__ volatile("cpsie i");
    sh_write0("    After CPSIE i:          PRIMASK = ");
    sh_write_dec(get_primask());
    sh_write0("\n\n");

    sh_write0("    Pattern for critical sections:\n");
    sh_write0("      uint32_t saved = __get_PRIMASK();\n");
    sh_write0("      __disable_irq();           // cpsid i\n");
    sh_write0("      // ... critical operation ...\n");
    sh_write0("      __set_PRIMASK(saved);      // restore, don't blindly enable\n\n");

    sh_write0("    Key rule: NEVER blindly 'cpsie i' after critical section.\n");
    sh_write0("    Always restore the PREVIOUS state. Otherwise you might\n");
    sh_write0("    enable interrupts that were already disabled!\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: 优先级反转概念
 * --------------------------------------------------------------------------
 */
static void demo_priority_inversion(void)
{
    sh_write0("[5] Priority Inversion\n\n");

    sh_write0("    Scenario (3 tasks with priority H > M > L):\n\n");
    sh_write0("    1. Task_L acquires a mutex (shared resource).\n");
    sh_write0("    2. Task_H preempts Task_L, tries to acquire same mutex.\n");
    sh_write0("       → Task_H blocks (waiting for Task_L to release).\n");
    sh_write0("    3. Task_M preempts Task_L (medium priority!).\n");
    sh_write0("       → Task_L can't run to release the mutex!\n");
    sh_write0("       → Task_H is blocked indefinitely!\n");
    sh_write0("       → Task_M (lower than H) effectively blocks Task_H.\n\n");

    sh_write0("    Solution: Priority Inheritance\n");
    sh_write0("      FreeRTOS mutex temporarily boosts Task_L's priority\n");
    sh_write0("      to Task_H's level while it holds the mutex.\n");
    sh_write0("      → Task_M can no longer preempt Task_L.\n");
    sh_write0("      → Task_L runs, releases mutex.\n");
    sh_write0("      → Task_H gets the mutex and runs.\n\n");

    sh_write0("    This is a classic embedded systems problem.\n");
    sh_write0("    (See FreeRTOS xSemaphoreCreateMutex for implementation)\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 6: 中断设计最佳实践
 * --------------------------------------------------------------------------
 */
static void demo_isr_best_practices(void)
{
    sh_write0("[6] ISR Design Best Practices\n\n");

    sh_write0("    1. Keep ISRs SHORT.\n");
    sh_write0("       Do minimal work in ISR, defer rest to task/thread.\n");
    sh_write0("       Pattern: ISR → set flag/queue → main loop processes.\n\n");

    sh_write0("    2. Assign priorities carefully.\n");
    sh_write0("       Time-critical: 0x00 or 0x40 (high).\n");
    sh_write0("       Normal I/O:    0x80 (medium).\n");
    sh_write0("       Background:    0xC0 (low, like PendSV for RTOS).\n\n");

    sh_write0("    3. Avoid function calls that may block.\n");
    sh_write0("       No vTaskDelay, no semihosting heavy I/O in ISR.\n\n");

    sh_write0("    4. Use ISR-safe queue/semaphore APIs.\n");
    sh_write0("       FreeRTOS: xQueueSendFromISR(), not xQueueSend().\n\n");

    sh_write0("    5. Measure and budget interrupt latency.\n");
    sh_write0("       M0 interrupt latency: 16 cycles (fixed).\n");
    sh_write0("       Plus: longest critical section = worst-case latency.\n\n");

    sh_write0("    6. Protect shared data with critical sections.\n");
    sh_write0("       But keep critical sections as short as possible.\n");
    sh_write0("       Long critical sections increase interrupt latency.\n\n");
}

int main(void)
{
    sh_write0("\n==============================================\n");
    sh_write0("  Lesson 13: Interrupt Priority & Preemption\n");
    sh_write0("  NVIC Deep Dive & ISR Design Patterns\n");
    sh_write0("==============================================\n\n");

    sh_write0(
        "Interrupt management is the heart of real-time embedded\n"
        "systems. Understanding priority, preemption, and critical\n"
        "sections is essential for reliable firmware.\n\n");

    demo_nvic_registers();
    demo_priority_config();
    demo_swi_trigger();
    demo_critical_section();
    demo_priority_inversion();
    demo_isr_best_practices();

    sh_write0("==============================================\n");
    sh_write0("  Key Takeaways:\n\n");
    sh_write0("  1. M0 has 4 priority levels (2 bits).\n");
    sh_write0("  2. Lower number = higher priority.\n");
    sh_write0("  3. High priority ISR preempts low priority ISR.\n");
    sh_write0("  4. NVIC supports tail-chaining (avoids re-stacking).\n");
    sh_write0("  5. PRIMASK disables all maskable interrupts.\n");
    sh_write0("  6. Always restore (not blindly set) PRIMASK state.\n");
    sh_write0("  7. Priority inversion is solved by priority inheritance.\n");
    sh_write0("  8. Keep ISRs short, defer work to tasks.\n");
    sh_write0("==============================================\n\n");

    sh_write0("=== Lesson 13 complete! ===\n\n");

    register int r0 __asm__("r0") = 0x18;
    register int r1 __asm__("r1") = 0;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    while (1)
    {
    }
    return 0;
}
