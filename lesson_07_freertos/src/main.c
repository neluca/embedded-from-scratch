/*
 * =============================================================================
 * Lesson 7 主程序 -- FreeRTOS on Cortex-M0 (microbit)
 * =============================================================================
 *
 * 演示 3 个 FreeRTOS 任务:
 *
 *   Task 1 "Blinky":  周期性输出心跳 (模拟 LED)
 *   Task 2 "Sensor":  模拟传感器读数, 通过 Queue 发送数据
 *   Task 3 "Monitor": 接收传感器数据, 显示统计信息
 *
 * FreeRTOS 启动流程:
 *   1. main() 初始化硬件
 *   2. 创建任务和同步对象
 *   3. vTaskStartScheduler() → 永远不返回
 *   4. FreeRTOS 通过 SVC 指令启动第一个任务
 *   5. SysTick → PendSV → 任务调度循环
 *
 * 内存配置 (microbit, 16KB RAM):
 *   - FreeRTOS heap:    4096 字节 (FreeRTOSConfig.h)
 *   - Blinky task stack: 128 字节 (32 字)
 *   - Sensor task stack: 256 字节 (64 字)
 *   - Monitor task stack: 256 字节 (64 字)
 *   - System stack (MSP): 512 字节
 *   - .data + .bss:      ~500 字节
 *   总计:                ~6KB / 16KB
 * =============================================================================
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "semihosting.h"
#include "task.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 任务函数声明
 * -------------------------------------------------------------------------- */
extern void task_blinky(void *pvParameters);
extern void task_sensor(void *pvParameters);
extern void task_monitor(void *pvParameters);

/* --------------------------------------------------------------------------
 * 队列句柄 (全局, 供任务间通信)
 * -------------------------------------------------------------------------- */
QueueHandle_t g_sensor_queue = NULL;

/* --------------------------------------------------------------------------
 * 任务栈大小 (字为单位, 在 microbit 上每个字 = 4 字节)
 * -------------------------------------------------------------------------- */
#define BLINKY_STACK_SIZE  128
#define SENSOR_STACK_SIZE  256
#define MONITOR_STACK_SIZE 256

/* --------------------------------------------------------------------------
 * 任务优先级 (0=最低, 3=最高)
 * -------------------------------------------------------------------------- */
#define BLINKY_PRIORITY  1
#define SENSOR_PRIORITY  2
#define MONITOR_PRIORITY 1

/* --------------------------------------------------------------------------
 * vApplicationMallocFailedHook -- 堆分配失败时的钩子
 * -------------------------------------------------------------------------- */
void vApplicationMallocFailedHook(void)
{
    /* 进入临界区, 输出错误信息 */
    sh_write0("\n!!! FATAL: Malloc failed! Heap exhausted.\n");
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

/* --------------------------------------------------------------------------
 * vApplicationStackOverflowHook -- 栈溢出时的钩子
 * -------------------------------------------------------------------------- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    sh_write0("\n!!! FATAL: Stack overflow in task: ");
    sh_write0(pcTaskName);
    sh_write0("\n");
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

/* --------------------------------------------------------------------------
 * vApplicationIdleHook -- 空闲钩子 (系统空闲时调用)
 * -------------------------------------------------------------------------- */
void vApplicationIdleHook(void)
{
    /* 在真实硬件上, 这里可以:
     *   - 进入低功耗模式 (WFI)
     *   - 执行后台诊断
     *   - 更新看门狗
     * 目前在 QEMU 中什么都不做 */
}

/* --------------------------------------------------------------------------
 * main -- FreeRTOS 应用程序入口
 * -------------------------------------------------------------------------- */
int main(void)
{
    sh_write0("\n=== Lesson 7: FreeRTOS on Cortex-M0 ===\n\n");

    sh_write0("FreeRTOS Kernel: V11.1.0\n");
    sh_write0("Platform: BBC micro:bit (nRF51822, Cortex-M0)\n");
    sh_write0("Config: 4KB heap, 3 tasks, 1 queue\n\n");

    /* --- 创建队列 (用于 Sensor → Monitor 通信) ---
     * 队列长度: 5 个元素
     * 每个元素: sizeof(uint32_t) = 4 字节
     */
    g_sensor_queue = xQueueCreate(5, sizeof(uint32_t));
    if (g_sensor_queue == NULL)
    {
        sh_write0("ERROR: Failed to create queue!\n");
        for (;;)
        {
        }
    }
    sh_write0("Queue created: 5 elements of 4 bytes each\n");

    /* --- 创建任务 ---
     * xTaskCreate(函数, 名称, 栈大小(字), 参数, 优先级, 句柄)
     */
    sh_write0("Creating tasks...\n");

    BaseType_t ret;
    ret = xTaskCreate(task_blinky, "Blinky",
                      BLINKY_STACK_SIZE, NULL, BLINKY_PRIORITY, NULL);
    if (ret != pdPASS)
    {
        sh_write0("ERROR: Failed to create Blinky task!\n");
        for (;;)
        {
        }
    }

    ret = xTaskCreate(task_sensor, "Sensor",
                      SENSOR_STACK_SIZE, NULL, SENSOR_PRIORITY, NULL);
    if (ret != pdPASS)
    {
        sh_write0("ERROR: Failed to create Sensor task!\n");
        for (;;)
        {
        }
    }

    ret = xTaskCreate(task_monitor, "Monitor",
                      MONITOR_STACK_SIZE, NULL, MONITOR_PRIORITY, NULL);
    if (ret != pdPASS)
    {
        sh_write0("ERROR: Failed to create Monitor task!\n");
        for (;;)
        {
        }
    }

    sh_write0("All 3 tasks created. Starting scheduler...\n\n");
    sh_write0("--- FreeRTOS Scheduler Running ---\n\n");

    /* --- 启动调度器 ---
     * vTaskStartScheduler() 永远不返回!
     * 它通过 SVC 指令启动最高优先级的就绪任务,
     * 然后 SysTick + PendSV 持续进行任务调度.
     */
    vTaskStartScheduler();

    /* 如果调度器返回 (不应该发生), 进入死循环 */
    sh_write0("ERROR: Scheduler returned! (should never happen)\n");
    for (;;)
    {
    }
    return 0;
}
