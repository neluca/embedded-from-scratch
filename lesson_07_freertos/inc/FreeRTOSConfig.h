/*
 * =============================================================================
 * FreeRTOSConfig.h -- microbit (nRF51822 Cortex-M0, 16KB RAM) 配置
 * =============================================================================
 *
 * 关键配置决策 (针对 16KB RAM 的严格约束):
 *   - 最大优先级: 4 (够用, 且减少 TCB 大小)
 *   - 最小栈大小: 60 字 (240 字节, M0 堆栈帧 8 字 + R4-R11 8 字 + 局部变量)
 *   - Tick 频率: 1000Hz (1ms 分辨率, 不要设太高以免过多上下文切换)
 *   - 堆分配: heap_4 (最佳适配, 支持碎片合并)
 *   - 总堆大小: 4096 字节 (留足够 RAM 给任务栈)
 *   - 不使用: 协程(coroutines), 软件定时器(software timers), 运行统计
 *   - 不使用: 任务通知, 流缓冲区, 消息缓冲区 (节省代码空间)
 *
 * 内存预算 (microbit 16KB = 16384 字节):
 *   系统栈 (MSP):                512 字节
 *   FreeRTOS 堆 (heap_4):      4096 字节
 *   任务栈 (3 个任务 + Idle):    ~600 字节
 *   .data + .bss (内核变量):     ~300 字节
 *   剩余空间:                    ~10KB (用于任务栈扩展和未来功能)
 * =============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* --------------------------------------------------------------------------
 * 调度器配置
 * -------------------------------------------------------------------------- */
#define configUSE_PREEMPTION         1 /* 抢占式调度 */
#define configUSE_TIME_SLICING       1 /* 同优先级任务时间片轮转 */
#define configUSE_IDLE_HOOK          0 /* 不使用空闲钩子 (节省空间) */
#define configUSE_TICK_HOOK          0 /* 不使用节拍钩子 */
#define configTICK_RATE_HZ           1000 /* 1ms 节拍 (SysTick) */

#define configMAX_PRIORITIES         4 /* 0-3, M0 仅支持 4 级 NVIC 优先级 */
#define configMINIMAL_STACK_SIZE     60 /* 最小任务栈 (字), 约 240 字节 */
#define configMAX_TASK_NAME_LEN      12 /* 任务名称最大长度 */

/* --------------------------------------------------------------------------
 * 内存分配
 * -------------------------------------------------------------------------- */
#define configSUPPORT_STATIC_ALLOCATION 0 /* 不使用静态分配 (节省代码) */
#define configSUPPORT_DYNAMIC_ALLOCATION 1 /* 使用动态分配 */
#define configTOTAL_HEAP_SIZE           (4096) /* 堆大小 4KB */

/* --------------------------------------------------------------------------
 * 可选功能 (关闭以节省代码空间)
 * -------------------------------------------------------------------------- */
#define configUSE_TASK_NOTIFICATIONS     1 /* 任务通知 (M0 友好的轻量同步, stream_buffer 需要) */
#define configUSE_MUTEXES                1 /* 互斥锁 (含优先级继承) */
#define configUSE_RECURSIVE_MUTEXES      0 /* 递归互斥锁 */
#define configUSE_COUNTING_SEMAPHORES    1 /* 计数信号量 */
#define configUSE_QUEUE_SETS             0 /* 队列集 */
#define configUSE_TASK_FPU_SUPPORT       0 /* M0 无 FPU */
#define configUSE_APPLICATION_TASK_TAG   0 /* 任务标签 */
#define configUSE_NEWLIB_REENTRANT       0 /* newlib 重入结构 */

/* 软件定时器: 需要独立的定时器守护任务, 消耗 ~200 字节栈 + RAM */
#define configUSE_TIMERS          0
#define configTIMER_TASK_PRIORITY 0
#define configTIMER_QUEUE_LENGTH  0
#define configTIMER_TASK_STACK_DEPTH 0

/* --------------------------------------------------------------------------
 * 钩子函数 (全部关闭以节省空间)
 * -------------------------------------------------------------------------- */
#define configCHECK_FOR_STACK_OVERFLOW 1 /* 栈溢出检测 (1=基本, 2=更严格) */
#define configUSE_MALLOC_FAILED_HOOK    1 /* malloc 失败钩子 */
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0

/* --------------------------------------------------------------------------
 * 运行时统计 (关闭以节省空间)
 * -------------------------------------------------------------------------- */
#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_TRACE_FACILITY      0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0

/* --------------------------------------------------------------------------
 * 协程 (已废弃, 关闭)
 * -------------------------------------------------------------------------- */
#define configUSE_CO_ROUTINES    0
#define configMAX_CO_ROUTINE_PRIORITIES 0

/* --------------------------------------------------------------------------
 * Tick 计数器宽度 (V11.1.0 新增要求)
 * -------------------------------------------------------------------------- */
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS

/* --------------------------------------------------------------------------
 * MPU (M0 nRF51 没有 MPU 或我们选择不使用)
 * -------------------------------------------------------------------------- */
#define configENABLE_MPU 0

/* --------------------------------------------------------------------------
 * 时钟和硬件配置
 * -------------------------------------------------------------------------- */
#define configCPU_CLOCK_HZ       (16000000UL) /* nRF51822 @ 16MHz */

/* --------------------------------------------------------------------------
 * Cortex-M0 特定配置
 * -------------------------------------------------------------------------- */
/* PendSV 优先级: 最低 (不抢占其他中断) */
#define configKERNEL_INTERRUPT_PRIORITY 0xC0 /* M0: bits[7:6]=11, 最低 */
/* SysTick 优先级: 应低于 PendSV, 但通常设为中等 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 0x80 /* 允许从中断调用 API */

/* --------------------------------------------------------------------------
 * 断言
 * -------------------------------------------------------------------------- */
#define configASSERT(x) \
    if ((x) == 0)       \
    {                    \
        taskDISABLE_INTERRUPTS(); \
        for (;;)         \
        {                \
        }                \
    }

/* --------------------------------------------------------------------------
 * 标准头文件 (newlib-nano 需要)
 * -------------------------------------------------------------------------- */
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_vTaskSuspend              1
#define INCLUDE_vTaskDelayUntil           1
#define INCLUDE_vTaskDelay                1
#define INCLUDE_xTaskGetSchedulerState    0

/* 为 demo 启用的 API */
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_uxTaskPriorityGet           0
#define INCLUDE_vTaskPrioritySet            0
#define INCLUDE_eTaskGetState               0
#define INCLUDE_vTaskDelete                 0
#define INCLUDE_vTaskCleanUpResources       0

#endif /* FREERTOS_CONFIG_H */
