/*
 * =============================================================================
 * wdt_driver.h -- nRF51 Watchdog Timer (WDT) 驱动接口
 * =============================================================================
 *
 * nRF51822 WDT 特性:
 *   - 32.768 kHz LFCLK 时钟源
 *   - 超时周期: (CRV + 1) / 32768 秒
 *   - CRV 范围: 0x00000 - 0xFFFFFFFF (最大约 36 小时)
 *   - 8 个独立的 reload request (RR[0..7]) 通道
 *   - 可配置在 CPU sleep/halt 时是否继续运行
 *   - 一旦启动无法停止 (硬件安全设计)
 *
 * WDT 工作原理:
 *   1. 配置 CRV (超时周期) 和 RREN (使能哪些 reload 通道)
 *   2. 写 TASKS_START = 1 → 启动 WDT
 *   3. 应用程序在超时前写 RR[n] 寄存器来"喂狗"
 *   4. 如果超时 → 系统复位 (RESETREAS.WDTRST = 1)
 *
 * 喂狗规则:
 *   - 向任何使能的 RR[n] 写入魔数 0x6E524635
 *   - 必须在上次 reload 请求被确认后才能再次写同一个 RR[n]
 *   - REQSTATUS 寄存器指示哪些 RR[n] 的 reload 请求还在 pending
 *   - 使用多个 RR 通道可以防止某个"卡死"的喂狗路径
 */
#ifndef WDT_DRIVER_H
#define WDT_DRIVER_H

#include <stdint.h>

/* =========================================================================
 * WDT 寄存器定义 (0x40010000)
 * ========================================================================= */
typedef struct
{
    volatile uint32_t TASKS_START;               /* 0x000: 写 1 启动 WDT */
    volatile uint32_t RESERVED0[63];
    volatile uint32_t RR[8];                     /* 0x100-0x11C: Reload Request */
    volatile uint32_t RESERVED1[249];
    volatile uint32_t CRV;                       /* 0x504: Counter Reload Value */
    volatile uint32_t RREN;                      /* 0x508: Reload Request Enable */
    volatile uint32_t CONFIG;                    /* 0x50C: Configuration */
    volatile uint32_t RESERVED2[60];
    volatile const uint32_t REQSTATUS;           /* 0x600: Reload Request Status (RO) */
} wdt_regs_t;

#define WDT_BASE ((wdt_regs_t *)0x40010000)

/* RREN 位: bit[n] = 1 启用 RR[n] 通道 */
#define WDT_RREN_RR0 (1U << 0)
#define WDT_RREN_RR1 (1U << 1)
#define WDT_RREN_RR2 (1U << 2)
#define WDT_RREN_RR3 (1U << 3)
#define WDT_RREN_RR4 (1U << 4)
#define WDT_RREN_RR5 (1U << 5)
#define WDT_RREN_RR6 (1U << 6)
#define WDT_RREN_RR7 (1U << 7)

/* CONFIG 位 */
#define WDT_CONFIG_SLEEP (1U << 0)  /* 1=CPU sleep 时 WDT 继续运行 */
#define WDT_CONFIG_HALT  (1U << 3)  /* 1=CPU halt 时 WDT 继续运行 */

/* RR 魔数: 写此值到 RR[n] 来喂狗 */
#define WDT_RR_MAGIC 0x6E524635

/* =========================================================================
 * POWER 外设 — 复位原因寄存器 (0x40000000)
 * ========================================================================= */
typedef struct
{
    volatile uint32_t RESERVED0[256];
    volatile uint32_t RESETREAS;                /* 0x400: Reset Reason (write 1 to clear) */
    volatile uint32_t RESERVED1[6];
    volatile uint32_t RESET;                     /* 0x41C: Soft Reset */
} power_regs_t;

#define POWER_BASE ((power_regs_t *)0x40000000)

/* RESETREAS 位 */
#define POWER_RESET_PIN    (1U << 0)  /* 复位引脚 */
#define POWER_RESET_WDT    (1U << 1)  /* 看门狗复位 */
#define POWER_RESET_LOCKUP (1U << 2)  /* 锁定复位 */
#define POWER_RESET_SREQ   (1U << 3)  /* 软件复位 */
#define POWER_RESET_OFF    (1U << 4)  /* 从 OFF 模式唤醒 */

/* =========================================================================
 * WDT 超时计算
 * ========================================================================= */
/* LFCLK = 32768 Hz, 超时秒数 = (crv + 1) / 32768 */
#define WDT_TIMEOUT_MS(ms) \
    ((uint32_t)(((uint64_t)(ms) * 32768ULL + 500ULL) / 1000ULL) - 1)

/* 常用超时值 (约值) */
#define WDT_CRV_1S  32767   /* 1 秒 */
#define WDT_CRV_2S  65535   /* 2 秒 */
#define WDT_CRV_5S  163839  /* 5 秒 */
#define WDT_CRV_10S 327679  /* 10 秒 */

/* =========================================================================
 * API 函数声明
 * ========================================================================= */

/* 初始化 WDT (配置超时和 reload 通道, 但不启动) */
void wdt_init(uint32_t crv, uint32_t rren, uint32_t config);

/* 启动 WDT (一旦启动无法停止!) */
void wdt_start(void);

/* 喂狗: 通过指定 RR 通道 reload */
void wdt_feed(uint32_t rr_channel);

/* 检查指定 RR 通道的 reload 请求是否已完成 */
int wdt_feed_pending(uint32_t rr_channel);

/* 读取复位原因寄存器并清除标志 */
uint32_t reset_reason_read(void);

/* 清除复位原因 */
void reset_reason_clear(uint32_t mask);

/* 软件复位 */
void system_reset(void);

#endif /* WDT_DRIVER_H */
