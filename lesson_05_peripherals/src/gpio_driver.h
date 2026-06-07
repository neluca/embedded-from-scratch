/*
 * =============================================================================
 * gpio_driver.h -- nRF51 GPIO 驱动
 * =============================================================================
 */
#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>

/* 引脚模式 */
typedef enum
{
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1,
    GPIO_MODE_INPUT_PULLUP = 2,
    GPIO_MODE_INPUT_PULLDOWN = 3
} gpio_mode_t;

/* 初始化指定引脚 */
void gpio_pin_init(uint8_t pin, gpio_mode_t mode);

/* 设置引脚输出高电平 */
void gpio_pin_set(uint8_t pin);

/* 设置引脚输出低电平 */
void gpio_pin_clear(uint8_t pin);

/* 翻转引脚输出 */
void gpio_pin_toggle(uint8_t pin);

/* 读取引脚输入值 (返回 0 或 1) */
int gpio_pin_read(uint8_t pin);

/* 写入 32 位值到整个 GPIO 端口 */
void gpio_port_write(uint32_t value);

/* 读取整个 GPIO 端口 */
uint32_t gpio_port_read(void);

#endif /* GPIO_DRIVER_H */
