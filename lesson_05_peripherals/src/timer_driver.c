/*
 * =============================================================================
 * timer_driver.c -- nRF51 Timer 驱动实现
 * =============================================================================
 *
 * nRF51 有 3 个定时器 (TIMER0/1/2).
 * 这里使用 TIMER0.
 *
 * Timer 使用 task/event 模型:
 *   - START task:    启动计数器
 *   - STOP task:     停止计数器
 *   - CLEAR task:    清零计数器
 *   - COUNT task:    计数模式 (从输入引脚计数)
 *   - COMPARE[0..3] events: 计数值 == CC[n] 时触发
 *
 * 预分频器 (PRESCALER):
 *   f_TIMER = 16 MHz / (2^PRESCALER)
 *   0: 16 MHz  (62.5 ns / tick)
 *   4: 1 MHz   (1 us / tick)
 *   6: 250 kHz (4 us / tick)
 *   9: 31.25 kHz (32 us / tick)
 */

#include "timer_driver.h"
#include "nrf51_registers.h"

/* --------------------------------------------------------------------------
 * timer_init -- 初始化定时器
 * -------------------------------------------------------------------------- */
void timer_init(uint32_t prescaler, uint32_t bitmode, uint32_t period)
{
    timer_regs_t *tmr = TIMER0_BASE;

    /* 1. 停止定时器 */
    TIMER0_TASK_STOP = 1;

    /* 2. 清零计数器 */
    TIMER0_TASK_CLEAR = 1;

    /* 3. 配置模式、位宽、预分频 */
    tmr->MODE      = TIMER_MODE_TIMER;
    tmr->BITMODE   = bitmode;
    tmr->PRESCALER = prescaler;

    /* 4. 设置比较值 */
    tmr->CC[0] = period;

    /* 5. 清除比较事件 */
    tmr->EVENTS_COMPARE[0] = 0;
}

/* --------------------------------------------------------------------------
 * timer_start / timer_stop / timer_clear
 * -------------------------------------------------------------------------- */
void timer_start(void)
{
    TIMER0_TASK_START = 1;
}

void timer_stop(void)
{
    TIMER0_TASK_STOP = 1;
}

void timer_clear(void)
{
    TIMER0_TASK_CLEAR = 1;
}

/* --------------------------------------------------------------------------
 * timer_read -- 读取当前计数值
 * --------------------------------------------------------------------------
 * nRF51 Timer 没有直接的 COUNT 寄存器!
 * 需要通过 CAPTURE task 将当前值捕获到 CC 寄存器.
 * 使用 CC[1] 作为捕获目标.
 * -------------------------------------------------------------------------- */
uint32_t timer_read(void)
{
    /* 触发 CAPTURE task (地址 = 基址 + 0x40 + n*4) */
    volatile uint32_t *capture_task = (volatile uint32_t *)(0x40008044);
    *capture_task                   = 1;
    return TIMER0_BASE->CC[1]; /* CC[1] 现在含有捕获的计数值 */
}

/* --------------------------------------------------------------------------
 * timer_compare_triggered / timer_compare_clear
 * -------------------------------------------------------------------------- */
int timer_compare_triggered(void)
{
    return (TIMER0_BASE->EVENTS_COMPARE[0] != 0);
}

void timer_compare_clear(void)
{
    TIMER0_BASE->EVENTS_COMPARE[0] = 0;
}

/* --------------------------------------------------------------------------
 * timer_delay_us -- 微秒级延时 (使用 1MHz 预分频器)
 * -------------------------------------------------------------------------- */
void timer_delay_us(uint32_t us)
{
    /* 设置 1MHz 预分频器 (1 tick = 1 us) */
    timer_init(4, TIMER_BITMODE_16, us);
    timer_clear();
    timer_start();

    /* 等待比较触发 */
    while (!timer_compare_triggered())
    {
        /* 忙等待 */
    }

    timer_compare_clear();
    timer_stop();
}

/* --------------------------------------------------------------------------
 * timer_delay_ms -- 毫秒级延时
 * --------------------------------------------------------------------------
 * 对于较长的延时, 降低频率以避免 16 位溢出
 * 使用 31.25kHz (prescaler=9): 1 tick = 32 us
 * 对于 1ms, 需要 1000/32 = 31.25 ≈ 32 ticks
 * 对于精确的 ms 延时, 分多次循环
 * -------------------------------------------------------------------------- */
void timer_delay_ms(uint32_t ms)
{
    /* 使用 31.25kHz 时钟 (prescaler=9, 1 tick = 32 us) */
    /* 每毫秒需要 ~32 ticks */
    for (uint32_t i = 0; i < ms; i++)
    {
        timer_init(9, TIMER_BITMODE_16, 32);
        timer_clear();
        timer_start();
        while (!timer_compare_triggered())
        {
        }
        timer_compare_clear();
        timer_stop();
    }
}
