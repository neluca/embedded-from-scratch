/*
 * =============================================================================
 * gpio_driver.c -- nRF51 GPIO 驱动实现
 * =============================================================================
 *
 * nRF51 GPIO 特性:
 *   - 32 个引脚 (P0.0 - P0.31)
 *   - 每个引脚独立配置 (PIN_CNF[n])
 *   - OUTSET/OUTCLR 实现原子位操作 (不需要 read-modify-write!)
 *   - DIRSET/DIRCLR 实现原子方向设置
 */

#include "gpio_driver.h"
#include "nrf51_registers.h"

/* --------------------------------------------------------------------------
 * gpio_pin_init -- 配置引脚模式和属性
 * -------------------------------------------------------------------------- */
void gpio_pin_init(uint8_t pin, gpio_mode_t mode)
{
    if (pin > 31)
    {
        return;
    }

    gpio_regs_t *gpio = GPIO_BASE;
    uint32_t cnf = PIN_CNF_INPUT_CONNECT | PIN_CNF_DRIVE_S0S1;

    switch (mode)
    {
    case GPIO_MODE_OUTPUT:
        cnf |= PIN_CNF_DIR_OUTPUT;
        break;
    case GPIO_MODE_INPUT_PULLUP:
        cnf |= PIN_CNF_PULL_UP;
        break;
    case GPIO_MODE_INPUT_PULLDOWN:
        cnf |= PIN_CNF_PULL_DOWN;
        break;
    case GPIO_MODE_INPUT:
    default:
        /* 默认: 输入, 无上下拉 */
        break;
    }

    gpio->PIN_CNF[pin] = cnf;
}

/* --------------------------------------------------------------------------
 * gpio_pin_set -- 设置引脚输出高 (使用 OUTSET 避免读-改-写)
 * -------------------------------------------------------------------------- */
void gpio_pin_set(uint8_t pin)
{
    GPIO_BASE->OUTSET = (1U << pin);
}

/* --------------------------------------------------------------------------
 * gpio_pin_clear -- 清除引脚输出低 (使用 OUTCLR 原子操作)
 * -------------------------------------------------------------------------- */
void gpio_pin_clear(uint8_t pin)
{
    GPIO_BASE->OUTCLR = (1U << pin);
}

/* --------------------------------------------------------------------------
 * gpio_pin_toggle -- 翻转引脚输出
 * -------------------------------------------------------------------------- */
void gpio_pin_toggle(uint8_t pin)
{
    uint32_t current = GPIO_BASE->OUT;
    if (current & (1U << pin))
    {
        GPIO_BASE->OUTCLR = (1U << pin);
    }
    else
    {
        GPIO_BASE->OUTSET = (1U << pin);
    }
}

/* --------------------------------------------------------------------------
 * gpio_pin_read -- 读取引脚输入值
 * -------------------------------------------------------------------------- */
int gpio_pin_read(uint8_t pin)
{
    return (GPIO_BASE->IN & (1U << pin)) ? 1 : 0;
}

/* --------------------------------------------------------------------------
 * 整端口操作
 * -------------------------------------------------------------------------- */
void gpio_port_write(uint32_t value)
{
    GPIO_BASE->OUT = value;
}

uint32_t gpio_port_read(void)
{
    return GPIO_BASE->IN;
}
