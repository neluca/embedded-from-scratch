/*
 * =============================================================================
 * semihosting.c -- Semihosting implementation
 * =============================================================================
 */
#include "semihosting.h"

static int sh_call(int op, void *param)
{
    register int r0 __asm__("r0") = op;
    register void *r1 __asm__("r1") = param;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
}

void sh_write0(const char *str)
{
    sh_call(0x04, (void *)str);
}

void sh_exit(int code)
{
    int p = code;
    sh_call(0x18, &p);
}

void sh_write_hex(uint32_t val)
{
    char buf[] = "0x00000000 ";
    for (int i = 9; i >= 2; i--)
    {
        uint32_t n = val & 0xF;
        buf[i] = n < 10 ? '0' + n : 'A' + n - 10;
        val >>= 4;
    }
    sh_write0(buf);
}

void sh_write_dec(uint32_t val)
{
    char buf[12];
    int pos = sizeof(buf) - 1;
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
    sh_write0(&buf[pos]);
}
