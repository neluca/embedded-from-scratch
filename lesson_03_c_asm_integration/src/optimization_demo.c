/*
 * =============================================================================
 * optimization_demo.c -- 编译器优化输出分析
 * =============================================================================
 *
 * 学习目标:
 * 1. 理解编译器优化级别 (-O0, -Os, -O2) 对生成代码的影响
 * 2. 学会用 objdump 分析编译器输出
 * 3. 掌握常见 C 代码模式对应的汇编实现
 *
 * 使用方法:
 *   1. 用不同的 -O 级别编译
 *   2. objdump -d -S 查看反汇编
 *   3. 比较同一段 C 代码在不同优化级别下的汇编
 *
 * M0 编译器优化要点:
 *   - -Os: 优化代码大小 (嵌入式默认)
 *   - -O2: 优化速度 (平衡)
 *   - -O0: 无优化 (调试用)
 *   - -O3: 激进优化 (可能增加代码大小)
 * =============================================================================
 */

#include <stdint.h>

/* 外部声明 */
void demo_optimization(void);

/* 辅助输出 */
static void semihosting_write0(const char *s);
static void semihosting_write_hex(uint32_t v);
static void semihosting_write_dec(uint32_t v);

/*
 * =========================================================================
 * volatile 关键字的正确使用
 * =========================================================================
 *
 * volatile 告诉编译器: "这个变量的值可能在任何时刻改变,
 * 不要对它做任何优化假设".
 *
 * 使用场景:
 *   - 内存映射的硬件寄存器 (必须使用!)
 *   - 中断服务程序修改的全局变量
 *   - 多线程/多任务共享的变量
 *
 * 反例: 将 volatile 用于普通变量会导致:
 *   - 每次都从内存重新加载 (效率低)
 *   - 阻止编译器优化 (如常量传播)
 */

/* 模拟一个硬件寄存器 */
static volatile uint32_t g_hw_register = 0;

/* 错误用法: 累加操作在 volatile 上不是原子的! */
static void bad_volatile_usage(void)
{
    g_hw_register += 1; /* 这会生成 load-modify-store, 不是原子的! */
    g_hw_register += 1;
    g_hw_register += 1;
}

/* 正确用法: 对 volatile 变量的每个操作都是显式的 */
static void good_volatile_usage(void)
{
    uint32_t local = g_hw_register; /* 一次 load */
    local += 3;
    g_hw_register = local; /* 一次 store */
}

/*
 * =========================================================================
 * 函数内联 -- inline vs 普通函数
 * =========================================================================
 *
 * M0 上, 函数调用有开销: BL (4 字节) + BX LR (2 字节) + 可能的 PUSH/POP
 * 对于小型函数, 内联可以消除这些开销, 但会增加代码大小.
 */

/* 普通函数: 编译器生成 BL 指令调用它 */
static uint32_t square_noninline(uint32_t x)
{
    return x * x;
}

/* 内联函数: 编译器将函数体直接展开到调用点 */
static inline uint32_t square_inline(uint32_t x)
{
    return x * x;
}

/*
 * =========================================================================
 * 数组访问模式 -- 编译器如何生成寻址指令
 * =========================================================================
 *
 * 不同的循环写法会导致不同的汇编代码
 * M0 只有简单的 LDR/STR 寻址模式, 编译器需要手动计算地址
 */

/* 模式 1: 下标访问 */
static uint32_t sum_array_index(const uint32_t *arr, uint32_t n)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        sum += arr[i]; /* 编译器生成: ldr rx, [base, offset] */
    }
    return sum;
}

/* 模式 2: 指针递增 */
static uint32_t sum_array_pointer(const uint32_t *arr, uint32_t n)
{
    uint32_t sum = 0;
    const uint32_t *end = arr + n;
    while (arr < end)
    {
        sum += *arr++; /* 编译器生成: ldr rx, [base]; adds base, #4 */
    }
    return sum;
}

/* 这两个函数在 -O2 下通常生成相同的汇编代码 */

/*
 * =========================================================================
 * 除法 -- M0 没有硬件除法器!
 * =========================================================================
 *
 * 任何使用 / 或 % 的代码都会调用 __aeabi_uidiv / __aeabi_uidivmod
 * 这些是 libgcc 中的软件除法函数, 非常耗时.
 *
 * 优化技巧:
 *   - 用移位代替 2 的幂次除法: x / 8 → x >> 3
 *   - 用乘法+移位代替常数除法: x / 10 → (x * 0xCCCCCCCD) >> 35
 *   - 避免在中断处理中使用除法
 */

static uint32_t div_by_8_slow(uint32_t x)
{
    return x / 8; /* 调用 __aeabi_uidiv, ~100 周期 */
}

static uint32_t div_by_8_fast(uint32_t x)
{
    return x >> 3; /* 单周期 LSRS 指令 */
}

/*
 * =========================================================================
 * 循环展开 -- 手动 vs 编译器
 * =========================================================================
 *
 * M0 没有分支预测器, 每次分支可能导致 2 周期流水线刷新.
 * 循环展开可以减少分支次数, 但增加代码大小.
 */

/* 普通循环 */
static uint32_t sum_u32_normal(const uint32_t *arr, uint32_t n)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        sum += arr[i];
    }
    return sum;
}

/*
 * =========================================================================
 * 位域操作 -- C vs 直接寄存器操作
 * =========================================================================
 *
 * M0 没有 BFI/UBFX/SBFX 等位域指令 (这些是 ARMv7-M 的)
 * 位域操作完全依赖 AND/ORR/LSL/LSR 指令组合
 */

/* 提取 bits 4-7 */
static uint32_t extract_bits(uint32_t reg)
{
    return (reg >> 4) & 0xF; /* 编译器生成: LSRS + ANDS */
}

/* 设置 bits 8-11 为 value (假设 value < 16) */
static uint32_t set_bits(uint32_t reg, uint32_t value)
{
    reg &= ~(0xF << 8);     /* 清除目标位 */
    reg |= (value & 0xF) << 8; /* 设置新值 */
    return reg;
}

/*
 * =========================================================================
 * switch 语句的编译结果
 * =========================================================================
 *
 * 编译器根据 case 密度选择:
 *   - 稀疏 case: 级联 if-else (CMP + Bcc)
 *   - 密集 case: 跳转表 (TBB/TBH, M0 不支持! 所以也用 if-else)
 *   - 只有几个 case: 直接比较
 *
 * M0 不支持 TBB/TBH 指令, 所以 switch 总是生成 if-else 链.
 */

static const char *switch_demo(uint32_t code)
{
    switch (code)
    {
    case 0:
        return "ZERO";
    case 1:
        return "ONE";
    case 2:
        return "TWO";
    default:
        return "OTHER";
    }
}

/*
 * =========================================================================
 * 主演示函数
 * =========================================================================
 */
void demo_optimization(void)
{
    semihosting_write0("[3] Compiler Optimization Analysis\n");
    semihosting_write0("    Understanding C-to-asm mapping on M0\n\n");

    /* --- volatile --- */
    semihosting_write0("    [volatile usage]\n");
    g_hw_register = 42;
    bad_volatile_usage();
    semihosting_write0("    After bad_volatile_usage:  g_hw_register = ");
    semihosting_write_dec(g_hw_register);
    semihosting_write0("\n");

    g_hw_register = 42;
    good_volatile_usage();
    semihosting_write0("    After good_volatile_usage: g_hw_register = ");
    semihosting_write_dec(g_hw_register);
    semihosting_write0("\n");

    /* --- 内联 vs 非内联 --- */
    semihosting_write0("\n    [inline vs non-inline]\n");
    uint32_t r1 = square_noninline(9);
    uint32_t r2 = square_inline(9);
    semihosting_write0("    square_noninline(9) = ");
    semihosting_write_dec(r1);
    semihosting_write0("\n");
    semihosting_write0("    square_inline(9)    = ");
    semihosting_write_dec(r2);
    semihosting_write0("\n");
    semihosting_write0("    (Check objdump to see BL call vs inlined code)\n");

    /* --- 数组访问 --- */
    semihosting_write0("\n    [Array access patterns]\n");
    uint32_t arr[] = { 10, 20, 30, 40, 50 };
    uint32_t s1 = sum_array_index(arr, 5);
    uint32_t s2 = sum_array_pointer(arr, 5);
    semihosting_write0("    sum_array_index   = ");
    semihosting_write_dec(s1);
    semihosting_write0("\n");
    semihosting_write0("    sum_array_pointer = ");
    semihosting_write_dec(s2);
    semihosting_write0("\n");
    semihosting_write0("    (Both should = 150; check objdump for differences)\n");

    /* --- 除法 --- */
    semihosting_write0("\n    [Division on M0 (NO hardware divider!)]\n");
    semihosting_write0("    div_by_8_slow(100) = ");
    semihosting_write_dec(div_by_8_slow(100));
    semihosting_write0(" (uses __aeabi_uidiv)\n");
    semihosting_write0("    div_by_8_fast(100) = ");
    semihosting_write_dec(div_by_8_fast(100));
    semihosting_write0(" (uses LSRS, single cycle)\n");

    /* --- 位操作 --- */
    semihosting_write0("\n    [Bitfield operations]\n");
    uint32_t bits = extract_bits(0xAB);
    semihosting_write0("    extract_bits(0xAB) = ");
    semihosting_write_hex(bits);
    semihosting_write0(" (expect 0xA)\n");

    uint32_t set = set_bits(0, 0xC);
    semihosting_write0("    set_bits(0, 0xC)   = ");
    semihosting_write_hex(set);
    semihosting_write0(" (expect 0xC00)\n");

    /* --- switch --- */
    semihosting_write0("\n    [switch statement]\n");
    semihosting_write0("    switch(0) -> ");
    semihosting_write0(switch_demo(0));
    semihosting_write0("\n");
    semihosting_write0("    switch(3) -> ");
    semihosting_write0(switch_demo(3));
    semihosting_write0("\n");

    semihosting_write0(
        "\n    === Key takeaways ===\n"
        "    1. Use objdump -d -S to see C-to-asm mapping\n"
        "    2. -Os is the default for embedded (size matters)\n"
        "    3. volatile is for HW registers and ISR-shared vars\n"
        "    4. M0 lacks: UDIV/SDIV, CLZ, REV, BFI/UBFX, TBB/TBH\n"
        "    5. Use inline for tiny functions in hot paths\n"
        "    6. Shift instead of divide when possible\n"
        "    7. Pointer-based loops often = index-based (with -O2)\n");
}

/* =========================================================================
 * 本地辅助
 * ========================================================================= */
static void semihosting_write0(const char *str)
{
    register int r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = str;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}

static void semihosting_write_hex(uint32_t val)
{
    char buf[] = "0x00000000 ";
    for (int i = 9; i >= 2; i--)
    {
        uint32_t n = val & 0xF;
        buf[i] = n < 10 ? '0' + n : 'A' + n - 10;
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
