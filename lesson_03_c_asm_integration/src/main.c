/*
 * =============================================================================
 * Lesson 3 主程序 -- C 与汇编混合编程
 * =============================================================================
 * 编排各个演示模块:
 *   call_asm_from_c:   从 C 调用汇编函数
 *   inline_asm_demo:   GCC 内联汇编
 *   optimization_demo: 编译器优化输出分析
 * =============================================================================
 */

#include <stdint.h>

/* 模块入口声明 */
void demo_call_asm(void);
void demo_inline_asm(void);
void demo_optimization(void);

/* Semihosting 辅助 (复用 Lesson 1 的实现) */
static int semihosting_call(int op, void *param);
static void semihosting_write0(const char *str);
static void semihosting_exit(int code);
static void semihosting_write_hex(uint32_t val);
static void semihosting_write_dec(uint32_t val);

int main(void)
{
    semihosting_write0("\n=== Lesson 3: C & Assembly Integration ===\n\n");

    /* 演示 1: 汇编启动代码分析 */
    semihosting_write0(
        "[0] Startup in Assembly\n"
        "    startup.S replaces startup.c from Lesson 1.\n"
        "    Key differences:\n"
        "    - Manual .data copy and .bss zeroing in asm\n"
        "    - Direct control over AAPCS calling convention\n"
        "    - No compiler-generated prologue/epilogue overhead\n"
        "    Check startup.S for detailed comments.\n\n");

    /* 演示 2: 从 C 调用汇编函数 */
    demo_call_asm();

    /* 演示 3: GCC 内联汇编 */
    demo_inline_asm();

    /* 演示 4: 编译器优化输出 */
    demo_optimization();

    semihosting_write0("=== Lesson 3 complete! ===\n\n");
    semihosting_exit(0);

    while (1)
    {
    }
    return 0;
}

/* =========================================================================
 * Semihosting 辅助函数
 * ========================================================================= */

static int semihosting_call(int operation, void *param_block)
{
    register int r0 __asm__("r0") = operation;
    register void *r1 __asm__("r1") = param_block;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
}

static void semihosting_write0(const char *str)
{
    semihosting_call(0x04, (void *)str);
}

static void semihosting_exit(int code)
{
    int param = code;
    semihosting_call(0x18, &param);
}

static void semihosting_write_hex(uint32_t val)
{
    char buf[] = "0x00000000\n";
    for (int i = 9; i >= 2; i--)
    {
        uint32_t nibble = val & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        val >>= 4;
    }
    semihosting_write0(buf);
}

static void semihosting_write_dec(uint32_t val)
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
    semihosting_write0(&buf[pos]);
}
