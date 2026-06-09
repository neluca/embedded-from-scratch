/*
 * =============================================================================
 * task_blinky.c -- 周期性心跳任务 (演示 vTaskDelay)
 * =============================================================================
 *
 * 每 500ms 输出一次 "心跳" 信息.
 * 使用 vTaskDelay() 进入阻塞态, 释放 CPU 给其他任务.
 *
 * FreeRTOS 任务状态转换:
 *   Ready → Running (被调度器选中)
 *   Running → Blocked (vTaskDelay)
 *   Blocked → Ready (延时到期)
 *   Running → Ready (被更高优先级任务抢占)
 *
 * 关键要点:
 *   1. 任务函数永远不返回 (无限循环)
 *   2. vTaskDelay() 让任务进入阻塞态, 允许其他任务运行
 *   3. 任务可以随时被更高优先级的任务抢占
 *   4. 使用 taskYIELD() 主动让出 CPU
 */

#include "FreeRTOS.h"
#include "semihosting.h"
#include "task.h"

/* 控制"心跳"输出次数 */
#define MAX_BLINKS 10

void task_blinky(void *pvParameters)
{
    (void)pvParameters; /* 未使用的参数 */

    uint32_t blink_count = 0;
    uint32_t stack_high = 0;

    sh_write0("[Blinky] Task started.\n");

    for (;;)
    {
        blink_count++;

        sh_write0("[Blinky] Heartbeat #");
        sh_write_dec(blink_count);
        sh_write0("\n");

        /* 检查栈使用情况 */
        stack_high = uxTaskGetStackHighWaterMark(NULL);
        sh_write0("  Stack free: ");
        sh_write_dec(stack_high * 4);
        sh_write0(" bytes\n");

        if (blink_count >= MAX_BLINKS)
        {
            sh_write0("[Blinky] Demo complete. Exiting.\n");
            /* 通过 semihosting 干净退出 QEMU, 防止 CI 超时 */
            sh_write0("\n=== Lesson 7 Complete ===\n");
            sh_exit(0);
            /* 如果 sh_exit 返回 (不应发生), 挂起自己 */
            vTaskSuspend(NULL);
        }

        /* 阻塞 500ms (500 个系统节拍)
         *
         * 在此期间:
         *   - Blinky 任务进入 Blocked 状态
         *   - 调度器选择下一个 Ready 任务运行
         *   - 500ms 后 Blinky 回到 Ready 状态
         *
         * vTaskDelay 的内部实现:
         *   1. 将当前任务从 Ready List 移除
         *   2. 将任务插入 Delayed List (按唤醒时间排序)
         *   3. 触发 PendSV 进行上下文切换
         *   4. SysTick 每 1ms 检查 Delayed List, 到期任务移回 Ready List
         */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
