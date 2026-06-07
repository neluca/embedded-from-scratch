/*
 * =============================================================================
 * Lesson 5 主程序 -- 外设与硬件编程
 * =============================================================================
 *
 * 演示顺序:
 *   1. UART 初始化与基础输出
 *   2. GPIO 控制 (LED 阵列模拟)
 *   3. Timer 精确延时
 *   4. 交互式控制台
 *
 * 运行方式:
 *   qemu-system-arm -M microbit -kernel lesson_05.elf -nographic -serial stdio
 *   或使用脚本: bash scripts/run_qemu.sh lesson_05.elf
 *
 * 注意: QEMU microbit 的 UART 输出到终端.
 *   在 QEMU 控制台按 Ctrl+A, 然后按 X 退出.
 *   输入字符到 QEMU 使用 -serial stdio (默认).
 */

#include "console.h"
#include "gpio_driver.h"
#include "nrf51_registers.h"
#include "timer_driver.h"
#include "uart_driver.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 辅助: 简易字符串写入 (不使用 semihosting, 直接用 UART)
 * -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
 * demo_uart_basic -- 演示 UART 基本功能
 * -------------------------------------------------------------------------- */
static void demo_uart_basic(void)
{
    uart_puts("\n=== Lesson 5: Peripherals & Hardware Programming ===\n");
    uart_puts("UART driver initialized at 115200 baud\n");
    uart_puts("TX: P0.24, RX: P0.25\n\n");

    uart_puts("[1] UART Basic Output\n");
    uart_puts("    UART uses task/event model (nRF51 specific):\n");
    uart_puts("    - Write data to TXD register\n");
    uart_puts("    - Trigger STARTTX task\n");
    uart_puts("    - Wait for TXDRDY event\n\n");

    /* 演示各种格式输出 */
    uart_puts("    Hex output:   ");
    uart_put_hex(0xDEADBEEF);
    uart_puts("\n");

    uart_puts("    Dec output:   ");
    uart_put_dec(12345);
    uart_puts("\n");

    uart_puts("    Mixed: value=");
    uart_put_hex(0x42);
    uart_puts(" dec=");
    uart_put_dec(66);
    uart_puts("\n\n");
}

/* --------------------------------------------------------------------------
 * demo_gpio -- 演示 GPIO 控制
 * -------------------------------------------------------------------------- */
static void demo_gpio(void)
{
    uart_puts("[2] GPIO Control\n");

    /* 配置 "LED" 引脚为输出 */
    uart_puts("    Configuring P0.4 (LED col1) and P0.13 (LED row1) as outputs\n");
    gpio_pin_init(MICROBIT_PIN_LED_COL1, GPIO_MODE_OUTPUT);
    gpio_pin_init(MICROBIT_PIN_LED_ROW1, GPIO_MODE_OUTPUT);

    /* 配置按钮引脚为输入 (带内部上拉) */
    uart_puts("    Configuring P0.17 (Button A) as input with pull-up\n");
    gpio_pin_init(MICROBIT_PIN_BUTTON_A, GPIO_MODE_INPUT_PULLUP);

    /* GPIO 输出操作 */
    uart_puts("    Toggling simulated LEDs...\n");

    /* 使用 OUTSET/OUTCLR 进行原子位操作 (不需要 read-modify-write!) */
    uart_puts("    Set LED:   ");
    gpio_pin_set(MICROBIT_PIN_LED_COL1);
    uart_puts("P0.4=HIGH\n");

    uart_puts("    Clear LED: ");
    gpio_pin_clear(MICROBIT_PIN_LED_COL1);
    uart_puts("P0.4=LOW\n");

    uart_puts("    GPIO OUTSET/OUTCLR provide atomic bit operations.\n");
    uart_puts("    No read-modify-write needed for single bits!\n\n");

    /* 整端口读写演示 */
    uart_puts("    Port read:  IN  = ");
    uart_put_hex(gpio_port_read());
    uart_puts("\n");
    uart_puts("    Port write: OUT = 0x00000000 (cleared)\n");
    gpio_port_write(0);
}

/* --------------------------------------------------------------------------
 * demo_timer -- 演示 Timer 精确延时
 * -------------------------------------------------------------------------- */
static void demo_timer(void)
{
    uart_puts("\n[3] Hardware Timer (TIMER0)\n");

    uart_puts("    nRF51 TIMER0: 16/24/32-bit, prescaler 0-9\n");
    uart_puts("    f_TIMER = 16MHz / (2^prescaler)\n\n");

    /* 演示微秒延时 */
    uart_puts("    timer_delay_us(500)  = 500 us delay\n");
    uint32_t start_ticks;
    /* 使用 Systick 做粗略测量 (如果可用) */
    timer_delay_us(500);
    uart_puts("    -> done (500 us)\n");

    /* 演示毫秒延时 */
    uart_puts("    timer_delay_ms(10)  = 10 ms delay\n");
    timer_delay_ms(10);
    uart_puts("    -> done (10 ms)\n");

    /* 演示连续延时 */
    uart_puts("    Measuring approximate delays:\n");
    uart_puts("      delay_ms(100)... ");
    timer_delay_ms(100);
    uart_puts("done\n");
    uart_puts("      delay_ms(500)... ");
    timer_delay_ms(500);
    uart_puts("done\n");

    uart_puts("\n    Timer API: init / start / stop / clear / read\n\n");
}

/* --------------------------------------------------------------------------
 * demo_console -- 交互式控制台
 * -------------------------------------------------------------------------- */
static void demo_console(void)
{
    uart_puts("[4] Interactive Console\n");
    uart_puts("    Type commands and press Enter.\n");
    uart_puts("    Commands: help, info, led on, led off, delay N, echo X\n");

    console_prompt();

    /* 主循环: 轮询 UART 输入, 处理命令 */
    /* 运行一小段时间做演示, 然后退出 */
    uart_puts("    (Demo mode: polling for ~2 seconds, then exit)\n");
    uart_puts("    On real hardware this would run indefinitely.\n\n");

    /* 简单演示循环 (约 2 秒) */
    volatile int loops = 2000000;
    while (loops > 0)
    {
        console_poll();
        loops--;
    }

    uart_puts("\n    Console demo ended.\n\n");
}

/* --------------------------------------------------------------------------
 * main -- 主程序入口
 * -------------------------------------------------------------------------- */
/*
 * =============================================================================
 * QEMU 注意事项:
 *   QEMU microbit 不完全支持 nRF51 UART 的 task/event 模型.
 *   TX 任务可能不触发 TXDRDY 事件, 导致 busy-wait 死循环.
 *
 *   解决方案: 使用 semihosting 作为 QEMU 的输出通道.
 *   真实硬件上: UART 驱动代码是正确的 (基于 nRF51 产品规格).
 * =============================================================================
 */

/* Semihosting 辅助 (用于 QEMU 输出) */
#include "semihosting.h"

int main(void)
{
    sh_write0("\n=== Lesson 5: Peripherals & Hardware Programming ===\n");
    sh_write0("(Output via semihosting - UART driver is for real HW)\n\n");

    /* 注意: console_init() → uart_init() 在 QEMU 中配置 UART 但可能阻塞.
     * 真实硬件上取消下面的注释: */
    /* console_init(); */

    sh_write0("[1] UART Driver (nRF51)\n");
    sh_write0("    The nRF51 UART uses a task/event model:\n");
    sh_write0("    - TX: write TXD -> STARTTX task -> wait TXDRDY event\n");
    sh_write0("    - RX: STARTRX task -> wait RXDRDY event -> read RXD\n");
    sh_write0("    - Baud rates: 1200 to 1Mbps (UART_BAUD_* macros)\n");
    sh_write0("    - Pins: configurable PSELTXD/PSELRXD\n");
    sh_write0("    Note: QEMU microbit UART TX hangs (TXDRDY never fires).\n");
    sh_write0("    The driver code in uart_driver.c is correct for real HW.\n\n");

    sh_write0("[2] GPIO Driver (P0.0-P0.31)\n");
    sh_write0("    - Atomic bit ops via OUTSET/OUTCLR (no RMW needed!)\n");
    sh_write0("    - DIRSET/DIRCLR for direction control\n");
    sh_write0("    - PIN_CNF[n] for pull-up/down, drive strength, sensing\n");
    sh_write0("    - Button A (P0.17), Button B (P0.26) with pull-up\n");
    sh_write0("    - LED matrix via P0.4-P0.15 (5 rows, 5 cols in QEMU)\n\n");

    sh_write0("[3] Timer Driver (TIMER0)\n");
    sh_write0("    - nRF51 TIMER0: 16/24/32-bit, prescaler 0-9\n");
    sh_write0("    - f_TIMER = 16MHz / (2^prescaler)\n");
    sh_write0("    - Task/event model: START/STOP/CLEAR/COUNT\n");
    sh_write0("    - Capture/Compare: 4 channels (CC[0..3])\n");
    sh_write0("    - Prescaler 4 = 1MHz = 1us/tick for us delays\n");
    sh_write0("    - Prescaler 9 = 31.25kHz for ms delays\n\n");

    sh_write0("[4] Console (Interactive via UART)\n");
    sh_write0("    - Commands: help, info, led on/off, delay N, echo X\n");
    sh_write0("    - Line editing with backspace support\n");
    sh_write0("    - Runs on real hardware with UART terminal\n\n");

    /* --- 演示 GPIO 寄存器访问 (安全的只读操作) --- */
    sh_write0("[5] Probing hardware registers (read-only):\n");

    /* 读取 GPIO IN 寄存器 (安全操作) */
    uint32_t gpio_in = GPIO_BASE->IN;
    sh_write0("    GPIO IN  = ");
    sh_write_hex(gpio_in);
    sh_write0("\n");

    /* 读取 Systick CSR (在 QEMU 中可用) */
    volatile uint32_t *syst_csr = (volatile uint32_t *)0xE000E010;
    sh_write0("    SYST_CSR = ");
    sh_write_hex(*syst_csr);
    sh_write0("\n");

    sh_write0("\n=== Lesson 5 complete! ===\n");
    sh_write0("UART/GPIO/Timer drivers are ready for real hardware.\n");
    sh_exit(0);

    while (1)
    {
    }
    return 0;
}
