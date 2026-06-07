/* semihosting implementation */
#include "semihosting.h"
static int sh_call(int op, void *p)
{
    register int   r0 __asm__("r0") = op;
    register void *r1 __asm__("r1") = p;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
}
void sh_write0(const char *s)
{
    sh_call(0x04, (void *)s);
}
void sh_write_hex(uint32_t v)
{
    char b[] = "0x00000000 ";
    for (int i = 9; i >= 2; i--)
    {
        uint32_t n = v & 0xF;
        b[i]      = n < 10 ? '0' + n : 'A' + n - 10;
        v >>= 4;
    }
    sh_write0(b);
}
void sh_write_dec(uint32_t v)
{
    char b[12];
    int  p        = sizeof(b) - 1;
    b[p]          = '\0';
    if (v == 0) { b[--p] = '0'; }
    else { while (v) { b[--p] = '0' + (v % 10); v /= 10; } }
    sh_write0(&b[p]);
}
