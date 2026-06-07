/*
 * =============================================================================
 * task_sensor.c -- 模拟传感器任务 (演示 Queue 发送)
 * =============================================================================
 *
 * 每 300ms 生成模拟传感器数据, 通过队列发送给 Monitor 任务.
 *
 * 演示:
 *   1. xQueueSend: 任务间队列通信
 *   2. 任务优先级: Sensor (优先级 2) > Monitor (优先级 1)
 *      → Sensor 生成数据后立即唤醒 Monitor
 *   3. 生产-消费模式: Sensor 生产数据, Monitor 消费数据
 *
 * FreeRTOS 队列特性:
 *   - 线程安全 (自动保护, 不需要额外的互斥锁)
 *   - 阻塞发送: 队列满时发送者进入阻塞态
 *   - 阻塞接收: 队列空时接收者进入阻塞态
 *   - 复制语义: 数据按值复制到队列 (不是指针)
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "semihosting.h"
#include "task.h"

/* 外部队列 (在 main.c 中创建) */
extern QueueHandle_t g_sensor_queue;

/* 模拟传感器数据生成 */
static uint32_t simulate_sensor_reading(void)
{
    static uint32_t seed = 0x12345678;
    /* 简易 LFSR (线性反馈移位寄存器) 生成伪随机数 */
    seed = (seed >> 1) ^ ((seed & 1) ? 0x80000062 : 0);
    /* 限制在 0-1023 (10 位 ADC 范围) */
    return seed & 0x3FF;
}

void task_sensor(void *pvParameters)
{
    (void)pvParameters;

    sh_write0("[Sensor] Task started. Generating readings...\n");

    uint32_t reading_count = 0;
    uint32_t stack_high    = 0;

    for (;;)
    {
        reading_count++;

        /* 生成模拟读数 */
        uint32_t reading = simulate_sensor_reading();

        /*
         * 发送数据到队列.
         *
         * xQueueSend(g_sensor_queue, &reading, portMAX_DELAY):
         *   - 参数 1: 队列句柄
         *   - 参数 2: 数据指针 (按值复制)
         *   - 参数 3: 超时时间 (portMAX_DELAY = 永远等待)
         *
         * 如果队列满:
         *   - Sensor 任务进入 Blocked 状态
         *   - 当 Monitor 从队列中取出数据后, Sensor 被唤醒
         *   - portMAX_DELAY 表示无限等待
         *
         * 在这个 demo 中队列不会满 (Monitor 比 Sensor 优先级低,
         * 但 Sensor 会阻塞 300ms, Monitor 有足够时间处理)
         */
        if (xQueueSend(g_sensor_queue, &reading, pdMS_TO_TICKS(100))
            == pdPASS)
        {
            sh_write0("[Sensor] Reading #");
            sh_write_dec(reading_count);
            sh_write0(": value=");
            sh_write_dec(reading);
            sh_write0(" (sent via queue)\n");
        }
        else
        {
            sh_write0("[Sensor] ERROR: Queue full!\n");
        }

        /* 栈使用检查 */
        stack_high = uxTaskGetStackHighWaterMark(NULL);
        if (stack_high < 16)
        {
            sh_write0("[Sensor] WARNING: Stack running low!\n");
        }

        /* 300ms 后再次读数 */
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
