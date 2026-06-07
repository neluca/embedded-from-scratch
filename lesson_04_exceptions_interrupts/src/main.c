/*
 * =============================================================================
 * Lesson 4 主程序 -- 异常与中断处理
 * =============================================================================
 *
 * 演示顺序:
 *   1. SysTick 定时器 (轮询模式 + 中断模式)
 *   2. PendSV 触发和上下文切换模拟
 *   3. HardFault 分析器 (理论讲解, 不实际触发)
 *
 * Cortex-M0 异常模型要点:
 *   - 异常/中断入口: 硬件自动压栈 8 字 (xPSR,PC,LR,R12,R3,R2,R1,R0)
 *   - 异常/中断退出: 硬件自动出栈 8 字 (通过 BX LR, LR=EXC_RETURN)
 *   - NVIC: 最多 32 个外部中断, 4 级优先级
 *   - SysTick: 24 位递减定时器, 内建异常
 *   - PendSV: 可悬起异常, FreeRTOS 上下文切换的核心
 * =============================================================================
 */

#include "semihosting.h"
#include <stdint.h>

/* 从其他模块导入的演示函数 */
void demo_systick(void);
void demo_pendsv(void);
void demo_hardfault(void);

/* 从 startup.S 导入的 C 处理函数 (在各自模块中实现) */
void systick_handler_c(void);
void pendsv_handler_c(void);
void svcall_handler_c(void);
void hardfault_handler_c(uint32_t *frame);

/* SVCall 处理函数 (Lesson 7 FreeRTOS 将使用它) */
void svcall_handler_c(void)
{
    sh_write0("[SVCall] SVCall handler called (normally used by FreeRTOS)\n");
}

/* --------------------------------------------------------------------------
 * 简易忙等待延时 (用于演示)
 * -------------------------------------------------------------------------- */
static void busy_delay(uint32_t count)
{
    for (volatile uint32_t i = 0; i < count; i++)
    {
        __asm__ volatile("nop");
    }
}

/* --------------------------------------------------------------------------
 * main -- 主程序入口
 * -------------------------------------------------------------------------- */
int main(void)
{
    sh_write0("\n=== Lesson 4: Exception & Interrupt Handling ===\n\n");

    sh_write0(
        "Cortex-M0 Exception Model:\n"
        "  - Exception entry: hardware pushes 8 registers to stack\n"
        "  - Exception exit:  hardware pops 8 registers from stack\n"
        "  - NVIC: up to 32 external interrupts, 4 priority levels\n"
        "  - SysTick: built-in 24-bit down-counter timer\n"
        "  - PendSV: pendable exception for OS context switching\n\n");

    /* -------------------------------------------------------------------
     * 演示 1: SysTick 定时器
     * ------------------------------------------------------------------- */
    demo_systick();

    sh_write0("----------------------------------------------\n\n");

    /* -------------------------------------------------------------------
     * 演示 2: PendSV 触发
     * ------------------------------------------------------------------- */
    demo_pendsv();

    sh_write0("----------------------------------------------\n\n");

    /* -------------------------------------------------------------------
     * 演示 3: HardFault 分析器
     * ------------------------------------------------------------------- */
    demo_hardfault();

    /* -------------------------------------------------------------------
     * 如果想实际测试 HardFault 分析器, 取消下面的注释:
     * ⚠  HardFault 会停止程序, 此后的代码不会执行!
     * ------------------------------------------------------------------- */
    /* trigger_hardfault(1); */

    sh_write0("\n=== Lesson 4 complete! ===\n\n");
    sh_exit(0);

    while (1)
    {
    }
    return 0;
}
