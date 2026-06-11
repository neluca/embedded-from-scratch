/*
 * =============================================================================
 * semihosting.c -- ARM Semihosting 辅助函数实现
 * =============================================================================
 *
 * Semihosting 操作码 (R0):
 *   SYS_OPEN    = 0x01   打开文件
 *   SYS_CLOSE   = 0x02   关闭文件
 *   SYS_WRITEC  = 0x03   写一个字符到调试控制台
 *   SYS_WRITE0  = 0x04   写 NUL 结尾的字符串到调试控制台
 *   SYS_WRITE   = 0x05   写指定长度的数据到文件
 *   SYS_READ    = 0x06   从文件读取
 *   SYS_READC   = 0x07   从调试控制台读取一个字符
 *   SYS_EXIT    = 0x18   退出 QEMU (QEMU 扩展)
 *
 * 调用约定:
 *   mov  r0, #operation
 *   mov  r1, #parameter    (对于 SYS_WRITE0, R1 = 字符串地址)
 *   bkpt #0xAB
 * =============================================================================
 */

#include "semihosting.h"

/* --------------------------------------------------------------------------
 * sh_write0 -- 通过 semihosting 输出字符串
 * -------------------------------------------------------------------------- */
void sh_write0(const char *s)
{
    register int r0 __asm__("r0") = 0x04; /* SYS_WRITE0 */
    register const char *r1 __asm__("r1") = s;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}

/* --------------------------------------------------------------------------
 * sh_write_hex -- 以 "0xXXXXXXXX" 格式输出 32 位十六进制值
 * -------------------------------------------------------------------------- */
void sh_write_hex(uint32_t v)
{
    char buf[] = "0x00000000 ";
    for (int i = 9; i >= 2; i--)
    {
        uint32_t nibble = v & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        v >>= 4;
    }
    sh_write0(buf);
}

/* --------------------------------------------------------------------------
 * sh_write_dec -- 输出 32 位无符号十进制值
 * -------------------------------------------------------------------------- */
void sh_write_dec(uint32_t v)
{
    char buf[11]; /* max "4294967295" + NUL = 11 chars */
    int pos = 10;
    buf[pos--] = '\0';

    if (v == 0)
    {
        buf[pos--] = '0';
    }
    else
    {
        while (v > 0 && pos >= 0)
        {
            buf[pos--] = '0' + (v % 10);
            v /= 10;
        }
    }
    sh_write0(&buf[pos + 1]);
}

/* --------------------------------------------------------------------------
 * sh_writec -- 输出单个字符
 * -------------------------------------------------------------------------- */
void sh_writec(char c)
{
    register int r0 __asm__("r0") = 0x03; /* SYS_WRITEC */
    register const char *r1 __asm__("r1") = &c;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}

/* --------------------------------------------------------------------------
 * sh_exit -- 通过 semihosting 退出 QEMU
 *
 * 注意: 此操作码是 QEMU 的扩展, 在真实硬件上需要调试器支持.
 * 在没有调试器的真实硬件上, semihosting BKPT 会导致 HardFault.
 * ========================================================================== */
void sh_exit(int code)
{
    /* ADP_Stopped_ApplicationExit */
    register int r0 __asm__("r0") = 0x18;
    register int *r1 __asm__("r1") = &(int){code};
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
