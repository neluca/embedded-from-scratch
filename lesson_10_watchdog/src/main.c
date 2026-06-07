/*
 * =============================================================================
 * Lesson 10 主程序 -- 看门狗定时器与系统可靠性
 * =============================================================================
 *
 * 学习目标:
 * 1. 理解看门狗定时器 (WDT) 的工作原理和必要性
 * 2. 掌握 nRF51 WDT 的配置与喂狗策略
 * 3. 理解复位原因寄存器 (RESETREAS) 的诊断价值
 * 4. 学习生产级系统中的 WDT 最佳实践
 * 5. 了解 WDT 与 FreeRTOS 的配合模式
 *
 * WDT 是什么?
 *   - 一个独立运行的递减计数器
 *   - 减到 0 时产生系统复位
 *   - 应用程序必须在超时前"喂狗"(reload 计数器)
 *   - 一旦启动就无法停止 (硬件安全设计)
 *
 * 为什么需要 WDT?
 *   - 嵌入式系统可能因软件 bug 卡死
 *   - 内存损坏可能导致程序跑飞
 *   - 死锁/活锁会使系统失去响应
 *   - WDT 是最后的保障: 宁可复位, 也不要永久卡死
 *
 * nRF51822 WDT 特性:
 *   - 时钟源: 32.768 kHz LFCLK
 *   - 超时范围: ~30.5 us 到 ~36 小时
 *   - 8 个独立的 reload 通道 (用于多点喂狗策略)
 *   - 可配置在 CPU sleep/halt 时是否继续运行
 * =============================================================================
 */

#include "semihosting.h"
#include "wdt_driver.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 演示 1: 复位原因诊断
 * --------------------------------------------------------------------------
 * 系统启动后第一件事应该是检查"为什么复位".
 * RESETREAS 寄存器记录最近一次复位的原因.
 * 这可以区分: 上电复位、WDT 复位、软件复位、引脚复位等.
 *
 * 典型用法:
 *   uint32_t reason = reset_reason_read();
 *   reset_reason_clear(reason);  // 清除以备下次使用
 *   if (reason & POWER_RESET_WDT) {
 *       // 上次是 WDT 复位, 记录错误, 进入安全模式
 *   }
 */
static void demo_reset_reason(void)
{
    sh_write0("[1] Reset Cause Detection\n\n");

    uint32_t reason = reset_reason_read();

    sh_write0("    RESETREAS = ");
    sh_write_hex(reason);
    sh_write0("\n");

    /* 解码复位原因 */
    if (reason == 0)
    {
        sh_write0("    (No reset cause bits set — may be power-on reset)\n");
    }

    if (reason & POWER_RESET_PIN)
    {
        sh_write0("    - RESETPIN: 外部复位引脚触发\n");
    }
    if (reason & POWER_RESET_WDT)
    {
        sh_write0("    - WDT: 看门狗超时复位!\n");
        sh_write0("      WARNING: Previous system crash detected!\n");
        sh_write0("      In production: log this, enter safe mode.\n");
    }
    if (reason & POWER_RESET_LOCKUP)
    {
        sh_write0("    - LOCKUP: Cortex-M 锁定复位 (严重故障!)\n");
    }
    if (reason & POWER_RESET_SREQ)
    {
        sh_write0("    - SREQ: 软件请求复位\n");
    }
    if (reason & POWER_RESET_OFF)
    {
        sh_write0("    - OFF: 从 OFF 模式唤醒 (类似上电复位)\n");
    }

    /* 清除原因, 为下次诊断做准备 */
    reset_reason_clear(0xFFFFFFFF);

    sh_write0("\n    Best practice: Always check RESETREAS at startup.\n");
    sh_write0("    This distinguishes cold boot from crash recovery.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 2: WDT 基本配置与喂狗
 * --------------------------------------------------------------------------
 * 配置 WDT, 演示正确的喂狗流程.
 *
 * 注意: 在 QEMU microbit 中, WDT 可能不完全模拟.
 * 此处代码基于 nRF51822 产品规格, 对真实硬件正确.
 */
static void demo_wdt_basic(void)
{
    sh_write0("[2] WDT Configuration & Feeding\n\n");

    sh_write0("    WDT clock source: 32.768 kHz LFCLK\n");
    sh_write0("    Timeout formula:  T = (CRV + 1) / 32768 seconds\n\n");

    /* 配置 WDT: 2 秒超时, 启用 RR0 通道, sleep 时继续运行 */
    sh_write0("    Configuring WDT:\n");
    sh_write0("      - Timeout: 2 seconds (CRV = 65535)\n");
    sh_write0("      - RREN: RR0 enabled (single-channel feeding)\n");
    sh_write0("      - CONFIG: SLEEP=1 (WDT runs during CPU sleep)\n\n");

    wdt_init(WDT_CRV_2S, WDT_RREN_RR0, WDT_CONFIG_SLEEP);

    /* 读取并显示配置 */
    wdt_regs_t *wdt = WDT_BASE;
    sh_write0("    [Register state after init]\n");
    sh_write0("    CRV    = ");
    sh_write_dec(wdt->CRV);
    sh_write0("\n");
    sh_write0("    RREN   = ");
    sh_write_hex(wdt->RREN);
    sh_write0("\n");
    sh_write0("    CONFIG = ");
    sh_write_hex(wdt->CONFIG);
    sh_write0("\n\n");

    /* 启动 WDT（注意: 一旦启动无法停止!）
     * 在演示中, 我们设置较长的超时并立即喂狗 */
    sh_write0("    Starting WDT... (CAUTION: cannot stop after this!)\n");
    wdt_start();
    sh_write0("    WDT is now counting down from 2 seconds.\n\n");

    /* 立即喂狗 */
    sh_write0("    Feeding WDT via RR0 (write magic 0x6E524635):\n");
    wdt_feed(0);
    sh_write0("      -> RR0 written, WDT counter reloaded to 2s.\n");

    /* 检查喂狗是否被确认 */
    while (wdt_feed_pending(0))
    {
        /* 等待硬件确认 reload
         * 在 32.768kHz 时钟域, 通常只需几个 LFCLK 周期 */
    }
    sh_write0("      -> Feed confirmed by hardware (REQSTATUS cleared).\n\n");

    sh_write0("    The feeding process must repeat before each timeout.\n");
    sh_write0("    A typical main loop pattern:\n");
    sh_write0("      while (1) {\n");
    sh_write0("          do_work();\n");
    sh_write0("          wdt_feed(0);  // pet the dog!\n");
    sh_write0("      }\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 3: 多点喂狗策略 (Multi-Channel Feeding)
 * --------------------------------------------------------------------------
 * nRF51 的 8 个 RR 通道允许"多点喂狗"模式:
 *   每个需要监控的子系统/任务使用独立的 RR 通道
 *   只有所有通道都在超时前喂狗, 才不会复位
 *
 * 例如在 FreeRTOS 中:
 *   - RR0: 主循环 / idle task 负责喂
 *   - RR1: 传感器任务负责喂
 *   - RR2: 通信任务负责喂
 *   如果任何一个任务卡死, 对应的 RR 不会 reload → WDT 复位
 *
 * 这是一种简单而有效任务监控机制.
 */
static void demo_multi_channel(void)
{
    sh_write0("[3] Multi-Channel Feeding Strategy\n\n");

    sh_write0("    nRF51 WDT has 8 independent reload channels (RR0-RR7).\n");
    sh_write0("    Each channel must be fed before timeout.\n\n");

    sh_write0("    Pattern: Task Monitoring with WDT\n");
    sh_write0("    +------------------+------------------+\n");
    sh_write0("    | RR Channel       | Responsible Task |\n");
    sh_write0("    +------------------+------------------+\n");
    sh_write0("    | RR0              | Idle / Monitor   |\n");
    sh_write0("    | RR1              | Sensor Task      |\n");
    sh_write0("    | RR2              | Communication    |\n");
    sh_write0("    | RR3              | UI / Display     |\n");
    sh_write0("    +------------------+------------------+\n\n");

    sh_write0("    Multi-channel config example:\n");
    sh_write0("      // Enable RR0 through RR3\n");
    sh_write0("      wdt_init(crv, 0x0F, WDT_CONFIG_SLEEP);\n");
    sh_write0("      wdt_start();\n");
    sh_write0("      // Each task feeds its own channel:\n");
    sh_write0("      wdt_feed(0);  // from idle loop\n");
    sh_write0("      wdt_feed(1);  // from sensor task\n");
    sh_write0("      wdt_feed(2);  // from comm task\n");
    sh_write0("      wdt_feed(3);  // from UI task\n\n");

    sh_write0("    If any task hangs, its RR channel is not fed,\n");
    sh_write0("    the WDT resets the system — protecting against\n");
    sh_write0("    individual task failures.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 4: WDT 最佳实践与常见错误
 * --------------------------------------------------------------------------
 */
static void demo_best_practices(void)
{
    sh_write0("[4] WDT Best Practices & Common Mistakes\n\n");

    sh_write0("    === DO ===\n\n");

    sh_write0("    1. Feed from main loop, NOT from ISR.\n");
    sh_write0("       ISR can still fire even if main loop is stuck.\n\n");

    sh_write0("    2. Use multi-channel for multi-task systems.\n");
    sh_write0("       Each task feeds its own RR channel.\n\n");

    sh_write0("    3. Check RESETREAS at startup.\n");
    sh_write0("       Distinguish cold boot from WDT recovery.\n\n");

    sh_write0("    4. Log WDT reset events.\n");
    sh_write0("       Store diagnostics before clearing RESETREAS.\n\n");

    sh_write0("    5. Enable WDT during sleep (CONFIG.SLEEP=1).\n");
    sh_write0("       Protects against clock failure in sleep modes.\n\n");

    sh_write0("    === DON'T ===\n\n");

    sh_write0("    1. Don't feed WDT inside an ISR.\n");
    sh_write0("       Main loop may be dead while ISR keeps feeding.\n\n");

    sh_write0("    2. Don't feed WDT in a tight sub-loop.\n");
    sh_write0("       The outer logic may still be broken.\n\n");

    sh_write0("    3. Don't use a single feed point for critical systems.\n");
    sh_write0("       Use multi-channel to monitor each subsystem.\n\n");

    sh_write0("    4. Don't ignore WDT in development.\n");
    sh_write0("       If you can't test with WDT on, fix your code!\n\n");

    sh_write0("    5. Don't set timeout too short.\n");
    sh_write0("       Account for worst-case main loop duration.\n");
    sh_write0("       Include firmware update, flash write, etc.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: WDT + FreeRTOS 集成模式
 * --------------------------------------------------------------------------
 * 在实际 RTOS 系统中, WDT 的用法有所不同:
 *   - 空闲任务中喂狗 (保证调度器仍在运行)
 *   - 使用独立的高优先级监控任务
 *   - 每个关键任务喂自己的 RR 通道
 */
static void demo_wdt_freertos(void)
{
    sh_write0("[5] WDT with FreeRTOS Integration\n\n");

    sh_write0("    In an RTOS system, WDT feeding is more nuanced:\n\n");

    sh_write0("    Pattern A: Idle Task Feeding\n");
    sh_write0("      void vApplicationIdleHook(void) {\n");
    sh_write0("          wdt_feed(0);  // Scheduler is running\n");
    sh_write0("      }\n");
    sh_write0("      Proves: all tasks are blocked/yielded → OK.\n\n");

    sh_write0("    Pattern B: Dedicated Monitor Task (recommended)\n");
    sh_write0("      void task_monitor(void *pv) {\n");
    sh_write0("          while (1) {\n");
    sh_write0("              // Check each critical task's heartbeat\n");
    sh_write0("              for (int i = 0; i < NUM_TASKS; i++) {\n");
    sh_write0("                  if (task_checkin[i] == 0) {\n");
    sh_write0("                      // Task missed its heartbeat!\n");
    sh_write0("                      log_error_and_reset();\n");
    sh_write0("                  }\n");
    sh_write0("                  task_checkin[i] = 0;\n");
    sh_write0("              }\n");
    sh_write0("              wdt_feed(0);  // All tasks alive\n");
    sh_write0("              vTaskDelay(pdMS_TO_TICKS(500));\n");
    sh_write0("          }\n");
    sh_write0("      }\n\n");

    sh_write0("    Pattern C: Multi-Channel per Task\n");
    sh_write0("      void task_sensor(void *pv) {\n");
    sh_write0("          while (1) {\n");
    sh_write0("              read_sensor();\n");
    sh_write0("              wdt_feed(1);  // This task is alive\n");
    sh_write0("              vTaskDelay(pdMS_TO_TICKS(100));\n");
    sh_write0("          }\n");
    sh_write0("      }\n\n");

    sh_write0("    Key principle: The WDT must not be fed if the\n");
    sh_write0("    system is in an unrecoverable or unknown state.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 6: 计算合适的 WDT 超时值
 * --------------------------------------------------------------------------
 */
static void demo_timeout_calculation(void)
{
    sh_write0("[6] Timeout Calculation\n\n");

    sh_write0("    WDT clock: 32.768 kHz LFCLK\n");
    sh_write0("    Formula: timeout_sec = (CRV + 1) / 32768\n\n");

    sh_write0("    Common timeout values:\n");
    sh_write0("    +-----------+-----------+----------------+\n");
    sh_write0("    | CRV       | Timeout   | Use Case       |\n");
    sh_write0("    +-----------+-----------+----------------+\n");
    sh_write0("    | 32767     | 1 second  | Fast recovery  |\n");
    sh_write0("    | 65535     | 2 seconds | General purpose|\n");
    sh_write0("    | 163839    | 5 seconds | Complex systems|\n");
    sh_write0("    | 327679    | 10 seconds| Boot + init    |\n");
    sh_write0("    +-----------+-----------+----------------+\n\n");

    sh_write0("    How to choose:\n");
    sh_write0("      1. Measure worst-case main loop time\n");
    sh_write0("      2. Add 50-100% margin\n");
    sh_write0("      3. Consider flash write operations (can block)\n");
    sh_write0("      4. Consider firmware update mode\n");
    sh_write0("      5. Test with worst-case load\n\n");

    /* 演示计算公式 */
    uint32_t crv_1s = WDT_CRV_1S;
    uint32_t crv_100ms = WDT_TIMEOUT_MS(100);
    uint32_t crv_500ms = WDT_TIMEOUT_MS(500);

    sh_write0("    Macro usage examples:\n");
    sh_write0("      WDT_CRV_1S          = ");
    sh_write_dec(crv_1s);
    sh_write0(" (1 sec)\n");
    sh_write0("      WDT_TIMEOUT_MS(100) = ");
    sh_write_dec(crv_100ms);
    sh_write0(" (100 ms)\n");
    sh_write0("      WDT_TIMEOUT_MS(500) = ");
    sh_write_dec(crv_500ms);
    sh_write0(" (500 ms)\n\n");
}

/* --------------------------------------------------------------------------
 * main — 主程序入口
 * -------------------------------------------------------------------------- */
int main(void)
{
    sh_write0("\n==============================================\n");
    sh_write0("  Lesson 10: Watchdog Timer\n");
    sh_write0("  System Reliability & Fault Recovery\n");
    sh_write0("==============================================\n\n");

    sh_write0(
        "The Watchdog Timer is the last line of defense in\n"
        "embedded systems. When everything else fails — the\n"
        "WDT resets the system, bringing it back to a known\n"
        "good state.\n\n");

    /* 1. 首先检查: 我们为什么复位? */
    demo_reset_reason();

    /* 2. WDT 基本使用 */
    demo_wdt_basic();

    /* 3. 多点喂狗策略 */
    demo_multi_channel();

    /* 4. 最佳实践 */
    demo_best_practices();

    /* 5. FreeRTOS 集成 */
    demo_wdt_freertos();

    /* 6. 超时计算 */
    demo_timeout_calculation();

    /* --- 总结 --- */
    sh_write0("==============================================\n");
    sh_write0("  Key Takeaways:\n\n");
    sh_write0("  1. WDT is essential for system reliability.\n");
    sh_write0("  2. Feed from main loop or monitor task — NOT from ISR.\n");
    sh_write0("  3. Use multi-channel feeding to monitor each subsystem.\n");
    sh_write0("  4. Check RESETREAS at startup to detect crash recovery.\n");
    sh_write0("  5. Once started, WDT cannot be stopped — design for it.\n");
    sh_write0("  6. Choose timeout with adequate margin for worst case.\n");
    sh_write0("==============================================\n\n");

    sh_write0("NOTE: WDT register access works, but QEMU may not\n");
    sh_write0("simulate WDT counter or reset behavior.\n");
    sh_write0("The code here is correct for real nRF51822 hardware.\n\n");

    sh_write0("=== Lesson 10 complete! ===\n\n");
    sh_exit(0);

    while (1)
    {
    }
    return 0;
}
