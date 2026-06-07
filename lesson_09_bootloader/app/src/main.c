/*
 * =============================================================================
 * Application — 由 Bootloader 加载执行
 * =============================================================================
 *
 * 运行在 Flash 0x00002000 (bootloader 之后).
 * 由 bootloader 通过设置 SP + 跳转到 app_reset_handler 启动.
 *
 * Cortex-M0 限制: 硬件异常通过 bootloader 的向量表 (0x00000000).
 * App 的中断处理函数不会被硬件直接调用.
 */

#include <stdint.h>

/* Semihosting helpers */
static void sh_write0(const char *s)
{
    register int r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = s;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
static void sh_write_hex(uint32_t v)
{
    char b[] = "0x00000000\n";
    for (int i = 9; i >= 2; i--)
    {
        uint32_t n = v & 0xF;
        b[i] = n < 10 ? '0' + n : 'A' + n - 10;
        v >>= 4;
    }
    sh_write0(b);
}

/* 读取当前 SP */
static uint32_t get_sp(void)
{
    uint32_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));
    return sp;
}

int app_main(void)
{
    sh_write0("=== Application ===\n");
    sh_write0("Launched by bootloader!\n");
    sh_write0("Running from Flash 0x00002000\n");

    sh_write0("Current SP = ");
    sh_write_hex(get_sp());

    /* 演示应用功能 */
    sh_write0("\nApp is running normally.\n");
    sh_write0("In a real system, this would be the main firmware.\n\n");

    /* 模拟应用工作 */
    uint32_t result = 0;
    for (uint32_t i = 1; i <= 10; i++)
    {
        result += i;
    }
    sh_write0("Sum 1..10 = ");
    sh_write_hex(result);
    sh_write0("(expect 55 / 0x37)\n");

    sh_write0("\n=== Application Complete ===\n");

    /* 正常退出 (semihosting) */
    register int r0 __asm__("r0") = 0x18;
    register int r1 __asm__("r1") = 0;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");

    while (1)
    {
    }
    return 0;
}
