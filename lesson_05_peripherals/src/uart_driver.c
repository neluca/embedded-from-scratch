/*
 * =============================================================================
 * uart_driver.c -- nRF51 UART 驱动实现
 * =============================================================================
 *
 * nRF51 UART 特性:
 *   - 基于 task/event 模型 (不是传统的状态寄存器)
 *   - TX: 写 TXD → 触发 STARTTX task → 等待 TXDRDY event
 *   - RX: 触发 STARTRX task → 等待 RXDRDY event → 读 RXD
 *   - 无硬件 FIFO (单字节缓冲)
 *   - 支持硬件流控 (CTS/RTS), 但我们不使用
 *
 * 在 QEMU microbit 中:
 *   - UART 输出连接到 QEMU 的串口 (-serial stdio 参数)
 *   - 可以从终端看到 UART 输出
 *   - UART 输入在 QEMU 中可能不工作 (取决于 QEMU 版本)
 */

#include "uart_driver.h"
#include "nrf51_registers.h"

/* --------------------------------------------------------------------------
 * uart_init -- 初始化 UART
 * -------------------------------------------------------------------------- */
void uart_init(uint32_t baud_rate, uint8_t tx_pin, uint8_t rx_pin)
{
    uart_regs_t *uart = UART_BASE;

    /* 1. 禁用 UART (在配置期间) */
    uart->ENABLE = UART_ENABLE_DISABLED;

    /* 2. 配置引脚: TXD, RXD */
    uart->PSELTXD = tx_pin;
    uart->PSELRXD = rx_pin;

    /* 3. 配置: 无硬件流控, 无奇偶校验 */
    uart->CONFIG = UART_CONFIG_PARITY_EXCLUDED | UART_CONFIG_HWFC_DISABLED;

    /* 4. 设置波特率 */
    uart->BAUDRATE = baud_rate;

    /* 5. 使能 UART */
    uart->ENABLE = UART_ENABLE_ENABLED;

    /* 6. 启动接收 (如果我们想接收数据) */
    UART_TASK_STARTRX = 1;
}

/* --------------------------------------------------------------------------
 * uart_deinit -- 禁用 UART
 * -------------------------------------------------------------------------- */
void uart_deinit(void)
{
    UART_TASK_STOPTX  = 1;
    UART_TASK_STOPRX  = 1;
    UART_BASE->ENABLE = UART_ENABLE_DISABLED;
}

/* --------------------------------------------------------------------------
 * uart_putc -- 发送单个字符
 * --------------------------------------------------------------------------
 * nRF51 UART 发送流程:
 *   1. 写数据到 TXD 寄存器
 *   2. 触发 STARTTX task
 *   3. 等待 TXDRDY event (硬件置 1)
 *   4. 清除 TXDRDY event (写 0)
 * -------------------------------------------------------------------------- */
void uart_putc(char c)
{
    uart_regs_t *uart = UART_BASE;

    /* 写要发送的字符 */
    uart->TXD = (uint8_t)c;

    /* 启动发送 */
    UART_TASK_STARTTX = 1;

    /* 等待发送完成 */
    while (uart->EVENTS_TXDRDY == 0)
    {
        /* 忙等待 */
    }

    /* 清除事件 */
    uart->EVENTS_TXDRDY = 0;
}

/* --------------------------------------------------------------------------
 * uart_puts -- 发送字符串
 * -------------------------------------------------------------------------- */
void uart_puts(const char *str)
{
    while (*str != '\0')
    {
        /* 对换行符特殊处理: 输出 CR+LF */
        if (*str == '\n')
        {
            uart_putc('\r');
        }
        uart_putc(*str);
        str++;
    }
}

/* --------------------------------------------------------------------------
 * uart_rx_available -- 检查是否有数据可读
 * -------------------------------------------------------------------------- */
int uart_rx_available(void)
{
    return (UART_BASE->EVENTS_RXDRDY != 0);
}

/* --------------------------------------------------------------------------
 * uart_getc -- 读取单个字符
 * -------------------------------------------------------------------------- */
int uart_getc(void)
{
    uart_regs_t *uart = UART_BASE;

    /* 检查是否有数据 */
    if (uart->EVENTS_RXDRDY == 0)
    {
        return -1; /* 无数据可用 */
    }

    /* 读取数据 */
    int c = (int)(uart->RXD & 0xFF);

    /* 清除事件 */
    uart->EVENTS_RXDRDY = 0;

    return c;
}

/* --------------------------------------------------------------------------
 * uart_put_hex -- 以十六进制格式输出 uint32_t
 * -------------------------------------------------------------------------- */
void uart_put_hex(uint32_t val)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    char             buf[]        = "0x00000000 ";
    for (int i = 9; i >= 2; i--)
    {
        buf[i] = hex_chars[val & 0xF];
        val >>= 4;
    }
    uart_puts(buf);
}

/* --------------------------------------------------------------------------
 * uart_put_dec -- 以十进制格式输出 uint32_t
 * -------------------------------------------------------------------------- */
void uart_put_dec(uint32_t val)
{
    char buf[12];
    int  pos = sizeof(buf) - 1;
    buf[pos] = '\0';

    if (val == 0)
    {
        buf[--pos] = '0';
    }
    else
    {
        while (val > 0)
        {
            buf[--pos] = '0' + (val % 10);
            val /= 10;
        }
    }

    uart_puts(&buf[pos]);
}
