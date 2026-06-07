/*
 * =============================================================================
 * timer_driver.h -- nRF51 Timer 驱动
 * =============================================================================
 */
#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H

#include <stdint.h>

/* 初始化 Timer0 为定时器模式
 * @param prescaler: 预分频器 (0-9, 分频系数 = 2^prescaler)
 *                   0=16MHz, 4=1MHz, 6=250kHz, 9=31250Hz
 * @param bitmode:   计数器位宽 (TIMER_BITMODE_16/24/32)
 * @param period:    比较值 (计数器计数到此值后触发 COMPARE[0] 事件)
 */
void timer_init(uint32_t prescaler, uint32_t bitmode, uint32_t period);

/* 启动定时器 */
void timer_start(void);

/* 停止定时器 */
void timer_stop(void);

/* 清零定时器计数值 */
void timer_clear(void);

/* 读取当前计数值 */
uint32_t timer_read(void);

/* 检查 COMPARE[0] 事件是否触发 */
int timer_compare_triggered(void);

/* 清除 COMPARE[0] 事件 */
void timer_compare_clear(void);

/* 微秒级延时 (阻塞)
 * @param us: 延时的微秒数
 * 使用 Timer0 的 1MHz 预分频器 (prescaler=4)
 */
void timer_delay_us(uint32_t us);

/* 毫秒级延时 (阻塞) */
void timer_delay_ms(uint32_t ms);

#endif /* TIMER_DRIVER_H */
