/*
 * =============================================================================
 * console.c -- 简易串口控制台实现
 * =============================================================================
 *
 * 支持的命令:
 *   help    - 显示帮助
 *   led on  - LED 阵列全亮 (模拟)
 *   led off - LED 阵列全灭 (模拟)
 *   btn     - 读按钮状态
 *   delay N - 延时 N 毫秒
 *   info    - 系统信息
 */

#include "console.h"
#include "gpio_driver.h"
#include "nrf51_registers.h"
#include "timer_driver.h"
#include "uart_driver.h"
#include <stdint.h>

/* 简易字符串比较 */
static int str_eq(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (*a != *b)
        {
            return 0;
        }
        a++;
        b++;
    }
    return (*a == *b);
}

/* 简易 atoi */
static int atoi_simple(const char *s)
{
    int result = 0;
    while (*s >= '0' && *s <= '9')
    {
        result = result * 10 + (*s - '0');
        s++;
    }
    return result;
}

/* 简易 strlen */
static int strlen_simple(const char *s)
{
    int len = 0;
    while (*s)
    {
        len++;
        s++;
    }
    return len;
}

/* --------------------------------------------------------------------------
 * 命令缓冲区
 * -------------------------------------------------------------------------- */
#define CMD_BUF_SIZE 32
static char cmd_buf[CMD_BUF_SIZE];
static int  cmd_pos = 0;

/* --------------------------------------------------------------------------
 * simulate_led -- 模拟 microbit 的 5x5 LED 阵列
 * -------------------------------------------------------------------------- */
static void simulate_led_row(int row, uint8_t pattern)
{
    /* microbit LED 阵列有 5 行 (row 1-5)
     * 每行 5 列 (col 1-5)
     * 在 QEMU 中, LEDs 没有图形输出, 所以用 UART 显示 */
    uart_puts("  [");
    for (int col = 0; col < 5; col++)
    {
        if (pattern & (1U << col))
        {
            uart_puts("*");
        }
        else
        {
            uart_puts(" ");
        }
    }
    uart_puts("]\n");
}

/* --------------------------------------------------------------------------
 * process_command -- 处理输入的命令
 * -------------------------------------------------------------------------- */
static void process_command(const char *cmd)
{
    uart_puts("\n");

    if (str_eq(cmd, "help"))
    {
        uart_puts("Commands:\n");
        uart_puts("  help     - Show this help\n");
        uart_puts("  info     - System information\n");
        uart_puts("  led on   - Show LED pattern (simulated)\n");
        uart_puts("  led off  - Clear LED display\n");
        uart_puts("  delay N  - Delay N milliseconds\n");
        uart_puts("  echo X  - Echo back X\n");
    }
    else if (str_eq(cmd, "info"))
    {
        uart_puts("Platform: BBC micro:bit (nRF51822 Cortex-M0)\n");
        uart_puts("UART:    115200 baud, P0.24(TX) P0.25(RX)\n");
        uart_puts("Lesson 5: Peripheral Drivers Demo\n");
    }
    else if (str_eq(cmd, "led on"))
    {
        uart_puts("LED Matrix (simulated):\n");
        /* 模拟一个笑脸图案 */
        simulate_led_row(0, 0b01010);
        simulate_led_row(1, 0b01010);
        simulate_led_row(2, 0b00000);
        simulate_led_row(3, 0b10001);
        simulate_led_row(4, 0b01110);
    }
    else if (str_eq(cmd, "led off"))
    {
        uart_puts("LEDs off (cleared)\n");
        for (int i = 0; i < 5; i++)
        {
            simulate_led_row(i, 0);
        }
    }
    else if (str_eq(cmd, "btn"))
    {
        /* 在 QEMU 中, 按钮输入状态可能不可用 */
        uart_puts("Button A: ");
        uart_putc(gpio_pin_read(MICROBIT_PIN_BUTTON_A) ? '1' : '0');
        uart_puts(" (QEMU may not support GPIO input)\n");
        uart_puts("Button B: ");
        uart_putc(gpio_pin_read(MICROBIT_PIN_BUTTON_B) ? '1' : '0');
        uart_puts("\n");
    }
    else if (cmd[0] == 'd' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'a'
             && cmd[4] == 'y')
    {
        int ms = atoi_simple(cmd + 6);
        if (ms > 0 && ms <= 5000)
        {
            uart_puts("Delaying ");
            uart_put_dec(ms);
            uart_puts(" ms...\n");
            timer_delay_ms(ms);
            uart_puts("Done!\n");
        }
        else
        {
            uart_puts("Usage: delay <1-5000>\n");
        }
    }
    else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o')
    {
        uart_puts("Echo: ");
        if (cmd[4] == ' ')
        {
            uart_puts(cmd + 5);
        }
        uart_puts("\n");
    }
    else if (strlen_simple(cmd) > 0)
    {
        uart_puts("Unknown command: ");
        uart_puts(cmd);
        uart_puts("\nType 'help' for available commands.\n");
    }
}

/* --------------------------------------------------------------------------
 * console_init -- 初始化控制台
 * -------------------------------------------------------------------------- */
void console_init(void)
{
    /* 115200 baud, TX=P0.24, RX=P0.25 */
    uart_init(UART_BAUD_115200, MICROBIT_PIN_UART_TX, MICROBIT_PIN_UART_RX);
    cmd_pos = 0;
}

/* --------------------------------------------------------------------------
 * console_prompt -- 输出命令提示符
 * -------------------------------------------------------------------------- */
void console_prompt(void)
{
    uart_puts("\n> ");
}

/* --------------------------------------------------------------------------
 * console_poll -- 轮询 UART 输入并处理命令
 * -------------------------------------------------------------------------- */
int console_poll(void)
{
    int c = uart_getc();
    if (c < 0)
    {
        return 0; /* 无输入 */
    }

    /* 处理回车或换行 → 执行命令 */
    if (c == '\r' || c == '\n')
    {
        cmd_buf[cmd_pos] = '\0';
        process_command(cmd_buf);
        cmd_pos = 0;
        console_prompt();
        return 1;
    }
    /* 处理退格 */
    if (c == 0x08 || c == 0x7F)
    {
        if (cmd_pos > 0)
        {
            cmd_pos--;
            uart_puts("\b \b"); /* 删除屏幕上的字符 */
        }
        return 1;
    }

    /* 正常字符 */
    if (cmd_pos < CMD_BUF_SIZE - 1 && c >= 32 && c < 127)
    {
        cmd_buf[cmd_pos++] = (char)c;
        uart_putc((char)c); /* 回显 */
    }

    return 1;
}

/* --------------------------------------------------------------------------
 * console_write -- 输出字符串
 * -------------------------------------------------------------------------- */
void console_write(const char *str)
{
    uart_puts(str);
}
