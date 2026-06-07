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

/* FreeRTOS port tick handler (defined in port.c, in vector table) */
extern void SysTick_Handler(void);

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
/*
 * =========================================================================
 * QEMU SysTick 解决方案 — COUNTFLAG 轮询
 * =========================================================================
 *
 * QEMU microbit 模型的 SysTick 中断不触发 (NVIC 时钟树配置缺陷).
 * 但 SysTick 计数器本身是运行的, COUNTFLAG 在计数器到 0 时会置位.
 *
 * 解决方案: 在空闲钩子中轮询 COUNTFLAG, 模拟 SysTick ISR 的行为.
 * FreeRTOS 的 xPortSysTickHandler() 会递增 tick 计数并检查是否需要
 * 任务切换. 当所有任务都阻塞时 (vTaskDelay / xQueueReceive timeout),
 * 空闲任务运行 → 轮询 COUNTFLAG → tick 递增 → 任务到期被唤醒.
 *
 * 副作用: 只有在 CPU 空闲时 tick 才更新. 对演示任务 (全部使用
 * vTaskDelay 阻塞) 来说这不是问题. 对真实硬件, 应使用真正的
 * SysTick 中断 (去掉下方的 TICKINT 清除操作).
 */
void vApplicationIdleHook(void)
{
    /* SysTick CSR = 0xE000E010
     * bit0:  ENABLE     — 使能计数器
     * bit1:  TICKINT    — 中断使能 (QEMU bug: 不触发)
     * bit2:  CLKSOURCE  — 时钟源 (1=处理器时钟)
     * bit16: COUNTFLAG  — 计数器到过 0 (读 CSR 自动清零)
     *
     * 首次进入时: 清除 TICKINT (改用轮询)
     */
    static int first_call = 1;
    if (first_call)
    {
        first_call = 0;
        volatile uint32_t *syst_csr = (volatile uint32_t *)0xE000E010;
        *syst_csr &= ~(1U << 1); /* 清除 TICKINT, 改为 COUNTFLAG 轮询 */
    }

    /* 轮询 COUNTFLAG: 如果计数器到过 0, 手动调用 tick handler */
    volatile uint32_t *syst_csr = (volatile uint32_t *)0xE000E010;
    if (*syst_csr & (1U << 16))
    {
        /* 读 CSR 会自动清除 COUNTFLAG.
         * SysTick_Handler() (port.c:814) 调用 xTaskIncrementTick(),
         * 递增 tick 计数器, 检查是否有任务需要唤醒,
         * 如有则触发 PendSV 上下文切换. */
        SysTick_Handler();
    }
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
