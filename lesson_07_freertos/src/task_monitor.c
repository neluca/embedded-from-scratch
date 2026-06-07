/*
 * =============================================================================
 * task_monitor.c -- 数据监控任务 (演示 Queue 接收)
 * =============================================================================
 *
 * 从队列接收传感器数据并显示统计信息.
 *
 * 演示:
 *   1. xQueueReceive: 从队列接收数据
 *   2. 阻塞接收: 队列空时任务进入 Blocked 状态
 *   3. 数据处理: 计算平均值
 *   4. 栈检查: 验证任务栈使用情况
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "semihosting.h"
#include "task.h"
#include <stdint.h>

extern QueueHandle_t g_sensor_queue;

void task_monitor(void *pvParameters)
{
    (void)pvParameters;

    sh_write0("[Monitor] Task started. Waiting for sensor data...\n");

    uint32_t sample_count = 0;
    uint32_t sample_sum = 0;

    for (;;)
    {
        uint32_t reading;

        /*
         * 从队列接收数据.
         *
         * xQueueReceive(g_sensor_queue, &reading, portMAX_DELAY):
         *   - 如果队列有数据: 立即返回 pdPASS, reading 包含数据
         *   - 如果队列为空: 任务进入 Blocked 状态
         *   - portMAX_DELAY: 无限等待 (直到有新数据)
         *
         * 当 Sensor 任务调用 xQueueSend 时:
         *   1. 数据被复制到队列
         *   2. FreeRTOS 检查是否有任务在等待此队列
         *   3. 发现 Monitor 在 Blocked 状态等待
         *   4. Monitor 被移回 Ready 状态
         *   5. 如果 Monitor 优先级 > 当前运行任务, 触发 PendSV 切换
         */
        if (xQueueReceive(g_sensor_queue, &reading, portMAX_DELAY) == pdPASS)
        {
            sample_count++;
            sample_sum += reading;

            uint32_t average = sample_sum / sample_count;

            sh_write0("[Monitor] Sample #");
            sh_write_dec(sample_count);
            sh_write0(": reading=");
            sh_write_dec(reading);
            sh_write0(", avg=");
            sh_write_dec(average);

            /* 每 5 个样本输出一次统计 */
            if (sample_count % 5 == 0)
            {
                uint32_t stack_free =
                    uxTaskGetStackHighWaterMark(NULL);
                sh_write0("\n  [Stats] samples=");
                sh_write_dec(sample_count);
                sh_write0(", avg=");
                sh_write_dec(average);
                sh_write0(", stack_free=");
                sh_write_dec(stack_free * 4);
                sh_write0(" bytes");
            }
            sh_write0("\n");
        }
    }
}
