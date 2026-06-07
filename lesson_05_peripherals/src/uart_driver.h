/*
 * =============================================================================
 * uart_driver.h -- nRF51 UART 驱动
 * =============================================================================
 */
#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>

/* 初始化 UART
 * @param baud_rate: 波特率 (使用 UART_BAUD_* 宏)
 * @param tx_pin:    TX 引脚号 (微位: P0.24)
 * @param rx_pin:    RX 引脚号 (微位: P0.25)
 */
void uart_init(uint32_t baud_rate, uint8_t tx_pin, uint8_t rx_pin);

/* 禁用 UART */
void uart_deinit(void);

/* 发送单个字符 (轮询, 等待 TX 就绪) */
void uart_putc(char c);

/* 发送字符串 */
void uart_puts(const char *str);

/* 检查是否有接收数据可用 */
int uart_rx_available(void);

/* 读取单个字符 (如果没有数据, 返回 -1) */
int uart_getc(void);

/* 发送十六进制 32 位值 (调试用) */
void uart_put_hex(uint32_t val);

/* 发送十进制 32 位值 */
void uart_put_dec(uint32_t val);

#endif /* UART_DRIVER_H */
