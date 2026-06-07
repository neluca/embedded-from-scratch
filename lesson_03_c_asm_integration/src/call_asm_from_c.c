/*
 * =============================================================================
 * call_asm_from_c.c -- 从 C 语言调用汇编函数
 * =============================================================================
 *
 * 学习目标:
 * 1. 理解 AAPCS (ARM Architecture Procedure Call Standard) 调用约定
 * 2. 掌握如何在 C 中声明和调用汇编函数
 * 3. 理解参数传递 (R0-R3) 和返回值 (R0-R1) 的映射
 *
 * AAPCS 关键规则 (Cortex-M0):
 *   - 前 4 个 32 位参数通过 R0, R1, R2, R3 传递
 *   - 多于 4 个参数通过栈传递 (从右到左压栈)
 *   - 32 位返回值在 R0 中
 *   - 64 位返回值在 R0 (低 32 位) 和 R1 (高 32 位) 中
 *   - 被调用者必须保留 R4-R11, R13(SP)
 *   - 调用者可以自由使用 R0-R3, R12, R14(LR)
 *   - 栈在公共接口处必须 8 字节对齐
 * =============================================================================
 */

#include <stdint.h>
#include <stddef.h>

/* 外部声明 -- 这些符号在 startup.S 中定义 */
extern char __stack_top;

/*
 * =========================================================================
 * 从 asm_funcs.S 导入的汇编函数
 * =========================================================================
 * 这些函数完全用汇编实现, 可从 C 直接调用.
 * 参数通过 R0-R3 寄存器传递 (AAPCS 约定).
 */

/* 简单加法: R0 = R0 + R1 */
uint32_t asm_add(uint32_t a, uint32_t b);

/* 乘法: R0 = R0 * R1 */
uint32_t asm_mul(uint32_t a, uint32_t b);

/* 绝对值: R0 = |R0| */
int32_t asm_abs(int32_t x);

/* 前导零计数: R0 = clz(R0) */
uint32_t asm_clz(uint32_t value);

/* 字节序反转: R0 = swap32(R0) */
uint32_t asm_swap_bytes(uint32_t value);

/* 优化的 memcpy: R0 = memcpy32(dst, src, n) */
void *asm_memcpy32(void *dst, const void *src, uint32_t n);

/* 周期级延时: 空操作 n 次 */
void asm_delay_cycles(uint32_t cycles);

/* C 函数声明 (供 main.c 调用) */
void demo_call_asm(void);

/* =========================================================================
 * 辅助输出
 * ========================================================================= */
static void semihosting_write0(const char *s);
static void semihosting_write_hex(uint32_t v);
static void semihosting_write_dec(uint32_t v);

/*
 * =========================================================================
 * demo_call_asm -- 演示所有从 C 调用汇编的功能
 * =========================================================================
 */
void demo_call_asm(void)
{
    semihosting_write0("[1] Calling Assembly Functions from C\n");
    semihosting_write0("    AAPCS convention: R0-R3=params, R0=return\n\n");

    /* --- 基本运算 --- */
    semihosting_write0("    asm_add(42, 58) = ");
    semihosting_write_dec(asm_add(42, 58));
    semihosting_write0(" (expect 100)\n");

    semihosting_write0("    asm_mul(6, 7)   = ");
    semihosting_write_dec(asm_mul(6, 7));
    semihosting_write0(" (expect 42)\n");

    semihosting_write0("    asm_abs(-10)    = ");
    semihosting_write_dec(asm_abs(-10));
    semihosting_write0(" (expect 10)\n");

    semihosting_write0("    asm_abs(5)      = ");
    semihosting_write_dec(asm_abs(5));
    semihosting_write0(" (expect 5)\n");

    /* --- 位操作 (M0 不原生支持, 纯软件实现!) --- */
    semihosting_write0("\n    [Bit operations -- software emulated on M0]\n");

    semihosting_write0("    asm_clz(0x80000000) = ");
    semihosting_write_dec(asm_clz(0x80000000));
    semihosting_write0(" (expect 0)\n");

    semihosting_write0("    asm_clz(0x00000001) = ");
    semihosting_write_dec(asm_clz(0x00000001));
    semihosting_write0(" (expect 31)\n");

    semihosting_write0("    asm_clz(0)          = ");
    semihosting_write_dec(asm_clz(0));
    semihosting_write0(" (expect 32)\n");

    semihosting_write0("    asm_swap_bytes(0x12345678) = ");
    semihosting_write_hex(asm_swap_bytes(0x12345678));
    semihosting_write0(" (expect 0x78563412)\n");

    /* --- 优化的 memcpy --- */
    semihosting_write0("\n    [Optimized memcpy (word-aligned)]\n");

    uint32_t src_buf[]   = {0xDEADBEEF, 0xCAFEBABE, 0x12345678};
    uint32_t dst_buf[3]  = {0, 0, 0};
    asm_memcpy32(dst_buf, src_buf, 12);

    semihosting_write0("    asm_memcpy32: src[0]=");
    semihosting_write_hex(dst_buf[0]);
    semihosting_write0(" src[1]=");
    semihosting_write_hex(dst_buf[1]);
    semihosting_write0(" src[2]=");
    semihosting_write_hex(dst_buf[2]);
    semihosting_write0("\n");

    /* --- 查看汇编代码在内存中的布局 --- */
    semihosting_write0(
        "\n    [Inspecting symbols from C]\n"
        "    __stack_top  = ");
    semihosting_write_hex((uint32_t)(uintptr_t)&__stack_top);
    semihosting_write0(" (end of RAM)\n");

    semihosting_write0("    &asm_add     = ");
    semihosting_write_hex((uint32_t)(uintptr_t)asm_add);
    semihosting_write0(" (address in flash)\n");

    semihosting_write0("    &vector_table = ");
    extern void *vector_table[];
    semihosting_write_hex((uint32_t)(uintptr_t)&vector_table[0]);
    semihosting_write0(" (should be 0x00000000)\n");
}

/* =========================================================================
 * Semihosting 辅助 (本地副本, 避免循环依赖)
 * ========================================================================= */
static void semihosting_write0(const char *str)
{
    register int         r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = str;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}

static void semihosting_write_hex(uint32_t val)
{
    char buf[] = "0x00000000 ";
    for (int i = 9; i >= 2; i--)
    {
        uint32_t n = val & 0xF;
        buf[i]     = n < 10 ? '0' + n : 'A' + n - 10;
        val >>= 4;
    }
    semihosting_write0(buf);
}

static void semihosting_write_dec(uint32_t val)
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
    semihosting_write0(&buf[pos]);
}
