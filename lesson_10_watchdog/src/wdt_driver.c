/*
 * =============================================================================
 * wdt_driver.c -- nRF51 Watchdog Timer 驱动实现
 * =============================================================================
 *
 * 注意:
 *   - WDT 一旦启动就无法停止 (这是安全设计)
 *   - 如果 CRV 配置为 0 就启动, 超时几乎是立即的
 *   - 必须先调用 wdt_init() 配置, 再调用 wdt_start() 启动
 *   - QEMU microbit 的 WDT 模拟可能不完整
 *   - 代码逻辑基于 nRF51822 产品规格书, 对真实硬件正确
 */
#include "wdt_driver.h"

void wdt_init(uint32_t crv, uint32_t rren, uint32_t config)
{
    wdt_regs_t *wdt = WDT_BASE;

    /* 配置超时周期 (LFCLK = 32.768 kHz) */
    wdt->CRV = crv;

    /* 启用请求的 reload 通道 */
    wdt->RREN = rren;

    /* 配置 sleep/halt 行为 */
    wdt->CONFIG = config;
}

void wdt_start(void)
{
    /* 写 1 到 TASKS_START 启动 WDT
     * 一旦启动, WDT 开始递减计数, 到 0 时系统复位
     * 此后无法停止 — 必须持续喂狗 */
    WDT_BASE->TASKS_START = 1;
}

void wdt_feed(uint32_t rr_channel)
{
    /* 向指定的 RR[n] 写魔数来 reload 计数器
     * 如果 RR[n] 未在 RREN 中启用, 写入无效 */
    if (rr_channel < 8)
    {
        WDT_BASE->RR[rr_channel] = WDT_RR_MAGIC;
    }
}

int wdt_feed_pending(uint32_t rr_channel)
{
    if (rr_channel < 8)
    {
        return (WDT_BASE->REQSTATUS & (1U << rr_channel)) ? 1 : 0;
    }
    return 0;
}

/*
 * =========================================================================
 * 复位原因检测
 * =========================================================================
 * RESETREAS 是 POWER 外设中的只读寄存器.
 * 写入 1 到某位来清除对应标志.
 * 上电后第一时间读取它可以判断复位原因.
 */

uint32_t reset_reason_read(void)
{
    return POWER_BASE->RESETREAS;
}

void reset_reason_clear(uint32_t mask)
{
    /* 写 1 清除对应的复位原因标志位 */
    POWER_BASE->RESETREAS = mask;
}

void system_reset(void)
{
    /* 通过 AIRCR 寄存器触发系统复位
     * 写 0x05FA0004 到 0xE000ED0C */
    volatile uint32_t *aircr = (volatile uint32_t *)0xE000ED0C;
    *aircr = 0x05FA0004;
    /* 不会执行到这里 */
    while (1)
    {
    }
}
