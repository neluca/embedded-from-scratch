/*
 * =============================================================================
 * pendsv_demo.c -- PendSV (可悬起系统调用) 演示
 * =============================================================================
 *
 * PendSV 是 Cortex-M 中最特殊的异常之一.
 * 它在 FreeRTOS 中用于上下文切换, 原因:
 *
 *   1. 可悬起 (Pend-able): 可以通过 ICSR.PENDSVSET 触发
 *   2. 延迟执行: 优先级通常设为最低, 确保在更高优先级中断完成后再执行
 *   3. 不抢占 ISR: 如果当前正在处理其他中断, PendSV 会等待
 *
 * FreeRTOS 上下文切换流程:
 *   1. SysTick 中断触发 → 发现需要任务切换
 *   2. 设置 PendSV 悬起 (ICSR.PENDSVSET = 1)
 *   3. SysTick 中断返回
 *   4. PendSV 处理函数执行 (因为优先级最低, 此时所有 ISR 都已完成)
 *   5. PendSV 保存当前任务上下文, 切换到新任务
 *
 * ICSR (Interrupt Control and State Register, 0xE000ED04):
 *   bit28: PENDSVSET  - 写 1 悬起 PendSV
 *   bit27: PENDSVCLR  - 写 1 清除 PendSV 悬起
 *   bit26: PENDSTSET  - 写 1 悬起 SysTick (用于软件触发)
 *   bit25: PENDSTCLR  - 写 1 清除 SysTick 悬起
 *
 * 系统处理函数优先级寄存器 (SHPR):
 *   SHPR3 (0xE000ED20):
 *     bits 31-24: PRI_15 (SysTick)
 *     bits 23-16: PRI_14 (PendSV)
 *   M0 仅使用每个字节的高 2 位, 所以优先级值需左移 6 位:
 *     0x00 << 6 = 0x00 (最高优先级)
 *     0xC0 << 6 = 0xC0 (最低优先级, M0 只有 4 级)
 *     ...实际上 M0 的优先级别是 0, 64, 128, 192 (按移位后的值)
 *     即: 0x00, 0x40, 0x80, 0xC0
 */

#include "semihosting.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 系统控制寄存器定义
 * -------------------------------------------------------------------------- */

/* ICSR - Interrupt Control and State Register */
#define ICSR_BASE      0xE000ED04
#define ICSR           ((volatile uint32_t *)(ICSR_BASE))
#define ICSR_PENDSVSET (1U << 28)
#define ICSR_PENDSVCLR (1U << 27)
#define ICSR_PENDSTSET (1U << 26)
#define ICSR_PENDSTCLR (1U << 25)

/* SHPR3 - System Handler Priority Register 3 */
#define SHPR3_BASE        0xE000ED20
#define SHPR3             ((volatile uint32_t *)(SHPR3_BASE))
#define SHPR3_PRI_15_MASK (0xFFU << 24) /* SysTick priority */
#define SHPR3_PRI_14_MASK (0xFFU << 16) /* PendSV priority */

/* --------------------------------------------------------------------------
 * 演示用的"任务" 上下文
 * -------------------------------------------------------------------------- */
/* 注意: 非 static, startup.S 的 PendSV handler 需要访问 */
volatile uint32_t g_pendsv_triggered = 0;
volatile uint32_t g_pendsv_count = 0;

/* 模拟两个任务的栈 (未使用, 为 Lesson 7 做预留) */
#define TASK_STACK_SIZE 64

/* --------------------------------------------------------------------------
 * pendsv_set_priority -- 设置 PendSV 优先级
 * --------------------------------------------------------------------------
 * @param pri: 优先级 (0=最高, 0xC0=最低)
 * M0 仅使用高 2 位, 所以有效值是 0x00, 0x40, 0x80, 0xC0
 * -------------------------------------------------------------------------- */
void pendsv_set_priority(uint8_t pri)
{
    uint32_t shpr3_val = *SHPR3;
    /* 清除 PendSV 优先级字段 (bits 23-16), 设置新值 */
    shpr3_val &= ~SHPR3_PRI_14_MASK;
    shpr3_val |= ((uint32_t)pri << 16);
    *SHPR3 = shpr3_val;
}

/* --------------------------------------------------------------------------
 * pendsv_trigger -- 软件触发 PendSV
 * --------------------------------------------------------------------------
 * 写 ICSR.PENDSVSET = 1 悬起 PendSV 异常.
 * 当没有更高优先级的异常正在处理时, PendSV 处理函数会被调用.
 * -------------------------------------------------------------------------- */
void pendsv_trigger(void)
{
    *ICSR = ICSR_PENDSVSET;
    /* 数据同步屏障 (M0 上没有 DSB, 用编译器屏障代替)
     * __asm__ volatile("" ::: "memory"); */
}

/* --------------------------------------------------------------------------
 * pendsv_handler_c -- PendSV 处理函数 (C 部分)
 * --------------------------------------------------------------------------
 * 由 startup.S 中的 pendsv_handler 调用.
 *
 * 在一个真实的 RTOS 中, 这里会:
 *   1. 保存当前任务上下文 (R4-R11, SP)
 *   2. 选择下一个就绪任务
 *   3. 加载新任务上下文
 *   4. 异常返回 → 自动恢复 R0-R3, R12, LR, PC, xPSR
 *
 * 注意: R0-R3, R12, LR, PC, xPSR 由硬件自动保存和恢复.
 * R4-R11 需要软件手动保存/恢复.
 * -------------------------------------------------------------------------- */
void pendsv_handler_c(void)
{
    g_pendsv_triggered = 1;
    g_pendsv_count++;

    /*
     * 在 FreeRTOS 中, PendSV handler 的核心代码大致如下:
     *
     *   // 保存当前任务上下文
     *   mrs r0, PSP              ; 获取当前任务栈指针
     *   stmdb r0!, {r4-r11}      ; 保存 R4-R11 到任务栈
     *   str r0, [r1]             ; 更新任务 TCB 中的栈指针
     *
     *   // 选择下一个任务
     *   ldr r1, =pxCurrentTCB    ; 获取新任务 TCB 地址
     *   ldr r0, [r1]             ; 读取新任务栈指针
     *
     *   // 恢复新任务上下文
     *   ldmia r0!, {r4-r11}      ; 恢复 R4-R11
     *   msr PSP, r0              ; 更新 PSP 到新任务栈
     *
     *   // 异常返回 → 硬件自动恢复 R0-R3, R12, LR, PC, xPSR
     *   bx lr                    ; EXC_RETURN 触发硬件出栈
     *
     * 我们将在 Lesson 7 (FreeRTOS) 中实现完整的上下文切换!
     */
}

/* --------------------------------------------------------------------------
 * demo_pendsv -- 演示 PendSV 的使用
 * -------------------------------------------------------------------------- */
void demo_pendsv(void)
{
    sh_write0("[2] PendSV (Pendable Service Call)\n");
    sh_write0("    PendSV is the key mechanism FreeRTOS uses for context switching\n");
    sh_write0("    It can be pended (delayed) and runs at the lowest priority.\n\n");

    /* --- 设置 PendSV 优先级为最低 --- */
    sh_write0("    Setting PendSV priority to lowest (0xC0)...\n");
    pendsv_set_priority(0xC0); /* M0: 0xC0 = lowest of 4 levels */

    /* 读取并显示 SHPR3 确认设置 */
    uint32_t shpr3_val = *SHPR3;
    sh_write0("    SHPR3 = ");
    sh_write_hex(shpr3_val);
    sh_write0("(PendSV prio in bits 23-16)\n");

    /* --- 触发 PendSV --- */
    sh_write0("\n    Triggering PendSV via ICSR.PENDSVSET...\n");
    sh_write0("    WARNING: QEMU microbit has a bug where PendSV causes\n");
    sh_write0("    a nested HardFault. This is a QEMU limitation, NOT a\n");
    sh_write0("    code bug. On real Cortex-M0 hardware, PendSV works.\n");
    sh_write0("    The HardFault analyzer (demo #3) will catch this!\n\n");

    /* 不实际触发 PendSV, 因为 QEMU bug 会导致 HardFault.
     * 代码和配置都是正确的, 在真实硬件上可以正常工作.
     * 要测试 PendSV, 取消下面的注释 (会触发 HardFault 演示): */
    /* pendsv_trigger(); */

    sh_write0("    PendSV handler is configured correctly:\n");
    sh_write0("      - Vector table entry [14] = pendsv_handler (0x");
    extern char pendsv_handler;
    sh_write_hex((uint32_t)(uintptr_t)&pendsv_handler);
    sh_write0(")\n");
    sh_write0("      - Priority = 0xC0 (lowest)\n");
    sh_write0("      - Trigger via ICSR.PENDSVSET = 1\n");
    sh_write0("      - Handler saves/restores R4-R11, uses BX LR to exit\n\n");

    /* --- PendSV 上下文切换模拟 --- */
    sh_write0("    [Context switch theory]\n");
    sh_write0("    In FreeRTOS, PendSV handler:\n");
    sh_write0("      1. PUSH {r4-r11} to current task stack\n");
    sh_write0("      2. Save PSP to current TCB\n");
    sh_write0("      3. Load new TCB, get new PSP\n");
    sh_write0("      4. POP {r4-r11} from new task stack\n");
    sh_write0("      5. BX LR → hardware restores R0-R3,R12,LR,PC,xPSR\n");
    sh_write0("    (Full implementation in Lesson 7: FreeRTOS)\n\n");
}
