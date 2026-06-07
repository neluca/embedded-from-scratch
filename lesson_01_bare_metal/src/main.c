/*
 * =============================================================================
 * Lesson 1: Bare-Metal "Hello World" via Semihosting
 * =============================================================================
 *
 * 学习目标:
 * 1. 理解 semihosting 机制 -- BKPT 0xAB 指令触发 QEMU 拦截
 * 2. 掌握裸机编程基础 -- 无标准库、无操作系统
 * 3. 熟悉 QEMU 模拟器使用
 *
 * Semihosting 简介:
 *   Semihosting 是 ARM 定义的一种调试机制, 允许嵌入式程序通过调试器
 *   (或 QEMU) 访问宿主机的 I/O 功能。程序执行 BKPT 0xAB 指令时,
 *   QEMU 会读取 R0 (操作码) 和 R1 (参数块指针), 执行对应操作,
 *   然后将结果写回 R0 并继续执行下一条指令。
 *
 * 常用的 Semihosting 操作:
 *   SYS_OPEN    (0x01): 打开文件
 *   SYS_CLOSE   (0x02): 关闭文件
 *   SYS_WRITEC  (0x03): 写一个字符到控制台
 *   SYS_WRITE0  (0x04): 写一个以 NUL 结尾的字符串到控制台
 *   SYS_WRITE   (0x05): 写 N 个字节到文件
 *   SYS_READ    (0x06): 从文件读取 N 个字节
 *   SYS_EXIT    (0x18): 退出程序 (QEMU 会退出)
 *
 * 参数传递 (ARM 过程调用标准 AAPCS):
 *   R0 = 操作码
 *   R1 = 参数块指针
 *   返回值在 R0 中
 * =============================================================================
 */

#include <stdint.h>

/* --------------------------------------------------------------------------
 * Semihosting 操作码定义
 * -------------------------------------------------------------------------- */
#define SYS_OPEN   0x01
#define SYS_CLOSE  0x02
#define SYS_WRITEC 0x03
#define SYS_WRITE0 0x04
#define SYS_WRITE  0x05
#define SYS_READ   0x06
#define SYS_EXIT   0x18

/* SYS_OPEN 模式位 */
#define OPEN_MODE_READ        0x0  /* r */
#define OPEN_MODE_READ_BINARY 0x1 /* rb */
#define OPEN_MODE_READ_WRITE  0x2  /* r+ */
#define OPEN_MODE_WRITE       0x4  /* w */
#define OPEN_MODE_APPEND      0x8  /* a */

/* --------------------------------------------------------------------------
 * semihosting_call -- 执行 semihosting 调用
 * --------------------------------------------------------------------------
 * 使用内联汇编执行 BKPT 0xAB 指令.
 * R0 传递操作码, R1 传递参数块指针.
 *
 * M0 注意事项:
 *   - 必须使用 -mthumb 编译 (M0 不支持 ARM 状态)
 *   - BKPT 指令在 Thumb-1 中是 16 位编码
 *   - 如果 QEMU 未启用 -semihosting, BKPT 会触发 HardFault
 * -------------------------------------------------------------------------- */
static int semihosting_call(int operation, void *param_block)
{
    register int r0 __asm__("r0") = operation;
    register void *r1 __asm__("r1") = param_block;

    __asm__ volatile(
        "bkpt #0xAB"
        : "+r"(r0)         /* 输出: R0 被修改为返回值, 同时作为输入 */
        : "r"(r1)           /* 输入: R1 只在输入中 */
        : "memory"          /* 可能修改内存 */
    );
    return r0;
}

/* --------------------------------------------------------------------------
 * semihosting_write0 -- 通过 semihosting 输出字符串
 * --------------------------------------------------------------------------
 * SYS_WRITE0 (0x04):
 *   参数块 = 指向 NUL 结尾字符串的指针
 *   字符串中的每个字符依次输出到宿主控制台
 *   注意: 字符串必须位于可读内存中
 * -------------------------------------------------------------------------- */
static void semihosting_write0(const char *str)
{
    /* SYS_WRITE0 的参数块就是字符串指针本身
     * R1 直接指向字符串, 不需要额外的参数块结构
     */
    semihosting_call(SYS_WRITE0, (void *)str);
}

/* --------------------------------------------------------------------------
 * semihosting_writec -- 通过 semihosting 输出单个字符
 * --------------------------------------------------------------------------
 * SYS_WRITEC (0x03):
 *   参数块 = 指向字符的指针
 * -------------------------------------------------------------------------- */
static void semihosting_writec(char c)
{
    semihosting_call(SYS_WRITEC, &c);
}

/* --------------------------------------------------------------------------
 * semihosting_exit -- 通知 QEMU 程序正常退出
 * --------------------------------------------------------------------------
 * SYS_EXIT (0x18):
 *   参数块 = 指向退出码的指针 (0 = 成功)
 *   在 QEMU 中, 如果同时指定了 -semihosting-config enable=on,target=native,
 *   这个调用会使 QEMU 退出.
 *
 *   对于我们的用例, 此函数使 QEMU 干净地退出,
 *   而不是卡在死循环中.
 * -------------------------------------------------------------------------- */
static void semihosting_exit(int exit_code)
{
    int param = exit_code;
    semihosting_call(SYS_EXIT, &param);
}

/* --------------------------------------------------------------------------
 * 辅助函数: 通过 semihosting 打印整数 (用于调试)
 * -------------------------------------------------------------------------- */
static void semihosting_write_dec(uint32_t value)
{
    char buf[12]; /* 最大 10 位十进制数 + 符号 + NUL */
    int pos = sizeof(buf) - 1;

    buf[pos] = '\0';

    if (value == 0)
    {
        buf[--pos] = '0';
    }
    else
    {
        while (value > 0)
        {
            buf[--pos] = '0' + (value % 10);
            value /= 10;
        }
    }

    semihosting_write0(&buf[pos]);
}

/* --------------------------------------------------------------------------
 * main -- 程序入口
 * -------------------------------------------------------------------------- */
int main(void)
{
    /* 使用 SYS_WRITE0 输出字符串
     * SYS_WRITE0 自动处理 NUL 结尾, 一次调用输出整个字符串
     */
    semihosting_write0("Hello Embedded World!\n");
    semihosting_write0("Platform: BBC micro:bit (nRF51822, Cortex-M0)\n");
    semihosting_write0("Build: " __DATE__ " " __TIME__ "\n");
    semihosting_write0("\n--- ARM Cortex-M0 Learning Journey Begins ---\n\n");

    /* 展示 semihosting 的其他功能 */
    semihosting_write0("Testing semihosting operations:\n");
    semihosting_write0("  SYS_WRITE0:  Print null-terminated string (used above)\n");
    semihosting_write0("  SYS_WRITEC:  Print single characters: ");

    /* 逐个字符输出 */
    const char *hello = "HELLO\n";
    for (int i = 0; hello[i] != '\0'; i++)
    {
        semihosting_writec(hello[i]);
    }

    semihosting_write0("\n  Simple arithmetic on Cortex-M0:\n");
    semihosting_write0("    42 + 58 = ");

    /* 在 M0 上执行简单计算 */
    int a = 42;
    int b = 58;
    semihosting_write_dec(a + b);
    semihosting_write0("\n");

    semihosting_write0("\nPress Ctrl+A then X to exit QEMU, or program will exit itself.\n");

    /* 通过 semihosting ADC 块来测试 SYS_EXIT
     * 注意: 需要 QEMU 支持 SYS_EXIT, 旧版可能不支持
     * 如果 SYS_EXIT 失败, QEMU 继续运行 (Ctrl+A X 退出)
     */
    semihosting_exit(0);

    /* 如果 semihosting_exit 失败, 进入死循环 */
    while (1)
    {
        /* 在真实硬件上, 看门狗会复位芯片 */
    }
    return 0;
}
