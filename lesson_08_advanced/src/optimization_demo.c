/*
 * =============================================================================
 * optimization_demo.c -- Cortex-M0 编译器优化深度分析
 * =============================================================================
 *
 * 本模块通过具体代码示例, 展示编译器优化对 ARMv6-M (Thumb-1) 代码的影响.
 * 每个函数都附有反汇编注释, 帮助理解 C 代码如何映射到机器指令.
 *
 * 构建不同优化级别的版本进行对比:
 *   cmake -B build_opt -S . -DCMAKE_BUILD_TYPE=Debug       → -O0
 *   cmake -B build_opt -S . -DCMAKE_BUILD_TYPE=MinSizeRel  → -Os
 *   cmake -B build_opt -S . -DCMAKE_BUILD_TYPE=Release     → -O2
 *
 * 对比反汇编:
 *   diff <(objdump -d build_Debug/lesson_08_advanced.elf) <(objdump -d build_Release/lesson_08_advanced.elf)
 * =============================================================================
 */

#include "semihosting.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 辅助: 从 SysTick 读取当前计数值 (用于简单性能测量)
 *
 * M0 没有 DWT CYCCNT, 但 SysTick 是一个 24 位递减计数器.
 * 在 QEMU microbit 中, SysTick 计数器可以运行 (但中断不触发).
 *
 * 使用前需要先配置 SysTick: CSR=0x05 (ENABLE | CLKSOURCE=CPU clock)
 * -------------------------------------------------------------------------- */
static void systick_init(void)
{
    volatile uint32_t *csr = (volatile uint32_t *)0xE000E010;
    volatile uint32_t *rvr = (volatile uint32_t *)0xE000E014;
    volatile uint32_t *cvr = (volatile uint32_t *)0xE000E018;

    *csr = 0x00; /* 停止 SysTick */
    *rvr = 0x00FFFFFF; /* 最大重载值 */
    *cvr = 0x00; /* 清零当前值 */
    *csr = 0x05; /* ENABLE | CLKSOURCE (CPU clock) -- 但不启用中断 */
}

static uint32_t systick_read(void)
{
    volatile uint32_t *cvr = (volatile uint32_t *)0xE000E018;
    return *cvr;
}

/* --------------------------------------------------------------------------
 * 演示 1: 除以常数 -- 编译器的魔法
 * --------------------------------------------------------------------------
 *
 * M0 没有硬件除法器. `x / y` 在 -O0 会调用 __aeabi_uidiv (~100 周期).
 * 但 `x / 10` 在 -Os/-O2 会被编译器转换为 (x * 0xCCCCCCCD) >> 35 !
 * 因为编译器知道 10 是常数, 可以使用乘法逆元.
 *
 * 反汇编对比 (-O0):
 *   ldr r2, =10        ; 加载除数
 *   bl  __aeabi_uidiv  ; 调用软件除法 (~70 字节, ~100 周期)
 *
 * 反汇编对比 (-Os):
 *   ldr r3, =0xCCCCCCCD  ; 乘法逆元 (2^35 / 10)
 *   mov  r1, r3
 *   umull r2, r3, r0, r1  ; 64-bit 乘法 (注意: M0 没有 UMULL!)
 *                          ; 实际上 M0 用多个 MUL + ADC 模拟 64-bit 乘法
 *   lsrs r0, r3, #3       ; 右移 35 - 32 = 3 位
 *   ; 总计 ~20 bytes, ~30-50 周期 (仍比 __aeabi_uidiv 快)
 *
 * 经验: 当除数是编译时常数时, 编译器会自动优化.
 *       当除数是变量时, 必须调用 __aeabi_uidiv.
 * -------------------------------------------------------------------------- */
static uint32_t div_by_10(uint32_t x)
{
    return x / 10;
}

static uint32_t div_by_8(uint32_t x)
{
    /* 编译器将 x/8 优化为 x>>3 (单周期指令!) */
    return x / 8;
}

static uint32_t div_by_variable(uint32_t x, uint32_t y)
{
    /* y 是运行时变量, 必须调用 __aeabi_uidiv */
    return x / y;
}

static void demo_division_optimization(void)
{
    sh_write0("  [2.1] Division by Constant vs Variable\n\n");

    sh_write0("    Division on M0 (NO hardware divider!):\n\n");
    sh_write0("    ┌────────────────────┬──────────────────────────────────┐\n");
    sh_write0("    │ Expression         │ M0 assembly (rough cost)          │\n");
    sh_write0("    ├────────────────────┼──────────────────────────────────┤\n");
    sh_write0("    │ x / 10 (constant)  │ multiply inverse, ~30 cycles     │\n");
    sh_write0("    │ x / 8  (powerof2)  │ LSRS r0, r0, #3  →  1 cycle     │\n");
    sh_write0("    │ x / y  (variable)  │ BL __aeabi_uidiv → ~100 cycles   │\n");
    sh_write0("    │ x % 8  (powerof2)  │ AND r0, r0, #7   →  1 cycle     │\n");
    sh_write0("    └────────────────────┴──────────────────────────────────┘\n\n");

    /* 实际运行验证 */
    uint32_t x = 12345;
    sh_write0("    x = ");
    sh_write_dec(x);
    sh_write0("\n");
    sh_write0("    x / 10 = ");
    sh_write_dec(div_by_10(x));
    sh_write0("  (compiler optimizes to multiply inverse)\n");
    sh_write0("    x / 8  = ");
    sh_write_dec(div_by_8(x));
    sh_write0("  (compiler optimizes to x >> 3)\n");
    sh_write0("    x / 7  = ");
    sh_write_dec(div_by_variable(x, 7));
    sh_write0("  (must call __aeabi_uidiv)\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 2: volatile 的代价
 * --------------------------------------------------------------------------
 *
 * volatile 阻止编译器优化对该变量的读写.
 * 不加 volatile 时, 编译器可以把循环中的重复读取优化为一次.
 *
 * 反汇编对比 (无 volatile, -Os):
 *   ldr  r3, [r0]    ; 只读一次
 *   movs r2, #0
 * loop:
 *   adds r2, r2, r3  ; 每次加法用寄存器中的值
 *   subs r1, r1, #1
 *   bne  loop
 *
 * 反汇编对比 (有 volatile, -Os):
 * loop:
 *   ldr  r3, [r0]    ; 每次循环都从内存重新读取!
 *   adds r2, r2, r3
 *   subs r1, r1, #1
 *   bne  loop
 *
 * volatile 是必要的 (用于硬件寄存器), 但有性能代价.
 * -------------------------------------------------------------------------- */
static uint32_t sum_loop_no_volatile(const uint32_t *src, uint32_t count)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        sum += *src; /* 编译器可以把 *src 提升到循环外 */
    }
    return sum;
}

static uint32_t sum_loop_volatile(const volatile uint32_t *src, uint32_t count)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        sum += *src; /* 每次迭代必须重新读取 *src */
    }
    return sum;
}

static void demo_volatile_cost(void)
{
    sh_write0("  [2.2] volatile Overhead\n\n");

    uint32_t value = 42;
    volatile uint32_t vvalue = 42;
    const uint32_t iters = 10;  /* small count for fast QEMU testing */

    sh_write0("    Summing the same value 1000 times:\n\n");

    /* 测量非 volatile 版本 */
    systick_init();
    uint32_t start = systick_read();
    uint32_t r1 = sum_loop_no_volatile(&value, iters);
    uint32_t end = systick_read();
    uint32_t cycles_no_vol = start - end; /* SysTick 递减 */

    /* 测量 volatile 版本 */
    start = systick_read();
    uint32_t r2 = sum_loop_volatile(&vvalue, iters);
    end = systick_read();
    uint32_t cycles_vol = start - end;

    sh_write0("    Without volatile: result=");
    sh_write_dec(r1);
    sh_write0(", ~");
    sh_write_dec(cycles_no_vol);
    sh_write0(" SysTick ticks\n");

    sh_write0("    With volatile:    result=");
    sh_write_dec(r2);
    sh_write0(", ~");
    sh_write_dec(cycles_vol);
    sh_write0(" SysTick ticks\n");

    sh_write0("    volatile overhead: ~");
    sh_write_dec(cycles_vol > cycles_no_vol ? cycles_vol - cycles_no_vol : 0);
    sh_write0(" extra ticks (");
    if (cycles_no_vol > 0)
    {
        sh_write_dec((cycles_vol * 100) / cycles_no_vol);
    }
    else
    {
        sh_write0("?");
    }
    sh_write0("% of baseline)\n\n");

    sh_write0("    Conclusion: volatile prevents load-hoisting optimization.\n");
    sh_write0("    Use volatile ONLY for hardware registers and ISR-shared vars.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 3: 结构体打包与对齐
 * --------------------------------------------------------------------------
 *
 * M0 仅支持对齐的内存访问 (32-bit 访问必须 4 字节对齐).
 * 未对齐访问会触发 HardFault.
 *
 * struct padding 会增加 RAM 使用量.
 * __attribute__((packed)) 消除 padding, 但可能导致未对齐访问.
 * -------------------------------------------------------------------------- */
struct unpacked
{
    uint8_t a;  /* offset 0 */
    /* 3 bytes padding */
    uint32_t b; /* offset 4 */
    uint8_t c;  /* offset 8 */
    /* 3 bytes padding */
    uint16_t d; /* offset 10 */
    /* 2 bytes padding (to align to 4) */
}; /* total: 12 bytes */

struct __attribute__((packed)) packed
{
    uint8_t a;  /* offset 0 */
    uint32_t b; /* offset 1 ← 未对齐! M0 访问此字段会 HardFault */
    uint8_t c;  /* offset 5 */
    uint16_t d; /* offset 6 (对齐到 2) */
}; /* total: 8 bytes */

static void demo_struct_packing(void)
{
    sh_write0("  [2.3] Structure Packing & Alignment\n\n");

    sh_write0("    M0 requires aligned memory access (no hardware fixup).\n\n");

    sh_write0("    ┌──────────────────┬──────────┬──────────┬───────────┐\n");
    sh_write0("    │ Struct           │ sizeof   │ Safe?    │ RAM saved │\n");
    sh_write0("    ├──────────────────┼──────────┼──────────┼───────────┤\n");
    sh_write0("    │ unpacked         │ 12 bytes │ Yes      │ —         │\n");
    sh_write0("    │ packed           │ 8 bytes  │ NO!      │ 4 bytes    │\n");
    sh_write0("    │ packed (reorder) │ 8 bytes  │ Yes      │ 4 bytes    │\n");
    sh_write0("    └──────────────────┴──────────┴──────────┴───────────┘\n\n");

    sh_write0("    sizeof(unpacked) = ");
    sh_write_dec(sizeof(struct unpacked));
    sh_write0(" (12 bytes: 3 paddings)\n");

    sh_write0("    sizeof(packed)   = ");
    sh_write_dec(sizeof(struct packed));
    sh_write0(" (8 bytes: no padding but unsafe!)\n\n");

    sh_write0("    Best practice: order fields by size (largest first)\n");
    sh_write0("    to minimize padding without using packed attribute:\n");
    sh_write0("      struct { uint32_t b; uint16_t d; uint8_t a; uint8_t c; };\n");
    sh_write0("    → sizeof = 8 bytes, all fields naturally aligned.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 4: 内联 vs 函数调用
 * --------------------------------------------------------------------------
 *
 * M0 的函数调用开销:
 *   BL func  → 4 bytes 指令, 3 cycles
 *   BX  lr   → 2 bytes, 3 cycles
 *   合计: 6 bytes 代码, 6 cycles 调用/返回开销
 *
 * 如果函数体本身只有 2-4 bytes (如 add), 调用开销反而比函数体大!
 * inline 可以消除这个开销, 但会增加调用处的代码大小.
 * -------------------------------------------------------------------------- */
static uint32_t __attribute__((noinline)) add_noinline(uint32_t a, uint32_t b)
{
    return a + b; /* ADDS r0, r0, r1; BX lr → 函数体 4 bytes, 调用+返回 6 bytes = 10 bytes total */
}

static inline uint32_t add_inline(uint32_t a, uint32_t b)
{
    return a + b; /* 在调用处展开为 ADDS r?, r?, r? → 2 bytes, 无调用开销 */
}

static void demo_inline_vs_call(void)
{
    sh_write0("  [2.4] Inline Functions vs Function Calls\n\n");

    uint32_t a = 100, b = 200;
    uint32_t r1, r2;

    r1 = add_noinline(a, b);
    r2 = add_inline(a, b);

    sh_write0("    add_noinline(100,200) = ");
    sh_write_dec(r1);
    sh_write0("  (BL + BX overhead: ~6 bytes, ~6 cycles)\n");
    sh_write0("    add_inline(100,200)   = ");
    sh_write_dec(r2);
    sh_write0("  (expands inline: 2 bytes, 1 cycle)\n\n");

    sh_write0("    Rule of thumb: inline if function body < call overhead.\n");
    sh_write0("    On M0: inline functions smaller than ~6 bytes of code.\n");
    sh_write0("    Use 'static inline' in headers for small accessors.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: 循环展开
 * --------------------------------------------------------------------------
 *
 * 编译器在 -O2/-Os 下会自动进行循环展开.
 * 手动展开适合已知小循环次数的场景 (如固定大小的数据包处理).
 * -------------------------------------------------------------------------- */
static uint32_t sum_unrolled4(const uint32_t *arr)
{
    /* 手动 4 路展开 -- 比 naive 循环快, 但代码更多 */
    return arr[0] + arr[1] + arr[2] + arr[3];
}

static void demo_loop_unrolling(void)
{
    sh_write0("  [2.5] Loop Unrolling\n\n");

    uint32_t arr[4] = {10, 20, 30, 40};
    uint32_t result = sum_unrolled4(arr);

    sh_write0("    sum_unrolled4([10,20,30,40]) = ");
    sh_write_dec(result);
    sh_write0("\n\n");

    sh_write0("    Naive loop: 4 loads + 4 adds + loop counter = ~12 instructions\n");
    sh_write0("    Unrolled:   4 loads + 3 adds = 7 instructions (no loop overhead)\n\n");
    sh_write0("    Trade-off: speed ↑ ~40% but code size ↑ ~50%\n");
    sh_write0("    Best for: fixed-size blocks (e.g. protocol headers, hash rounds)\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 6: M0 专用优化技巧
 * -------------------------------------------------------------------------- */
static void demo_m0_optimization_tips(void)
{
    sh_write0("  [2.6] Cortex-M0 Specific Optimization Cheat Sheet\n\n");

    sh_write0("    ┌─────────────────────────────────────────────────────────┐\n");
    sh_write0("    │ DO                                  │ AVOID             │\n");
    sh_write0("    ├─────────────────────────────────────┼───────────────────┤\n");
    sh_write0("    │ uint32_t (native word size)         │ uint64_t (SW emul)│\n");
    sh_write0("    │ x >> 3 (1 cycle)                   │ x / 8 (uses div)  │\n");
    sh_write0("    │ x & 0x7 (1 cycle)                  │ x % 8 (uses div)  │\n");
    sh_write0("    │ x * 4 → LSLS (1 cycle)             │ general multiply  │\n");
    sh_write0("    │ ringbuf & (size-1) (1 cycle AND)   │ ringbuf % size    │\n");
    sh_write0("    │ uint32_t (aligned, 1-cycle ldr)    │ uint8_t (ldrb,ok) │\n");
    sh_write0("    │ fixed-point math                   │ float/double (SW) │\n");
    sh_write0("    │ static inline small funcs          │ many tiny calls   │\n");
    sh_write0("    │ __attribute__((aligned(4))) arrays │ misaligned data   │\n");
    sh_write0("    └─────────────────────────────────────┴───────────────────┘\n\n");

    sh_write0("    Build flags for minimal size:\n");
    sh_write0("      -Os -flto -ffunction-sections -fdata-sections\n");
    sh_write0("      -Wl,--gc-sections -Wl,--strip-all\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 7: 浮点 vs 定点 -- M0 上的数学策略
 * -------------------------------------------------------------------------- */
static void demo_fixed_point(void)
{
    sh_write0("  [2.7] Fixed-Point Math (M0 has no FPU)\n\n");

    sh_write0("    M0 has NO floating-point unit (FPU).\n");
    sh_write0("    float/double operations are software-emulated, ~100x slower.\n\n");

    sh_write0("    Use fixed-point math instead:\n\n");

    /* 定点数示例: 用 Q16.16 格式表示 1.5 */
    /* 1.5 * 65536 = 98304 = 0x18000 */
    uint32_t fixed_one_point_five = 98304; /* Q16.16: 1.5 */
    uint32_t fixed_three = (3 << 16); /* Q16.16: 3.0 */

    /* 乘法: (a * b) >> 16 (对于 Q16.16) */
    /* 1.5 * 3.0 = 4.5 → (98304 * 196608) >> 16 = 294912 → 4.5 */
    uint64_t product = (uint64_t)fixed_one_point_five * (uint64_t)fixed_three;
    uint32_t result = (uint32_t)(product >> 16);

    sh_write0("    Q16.16 format: 1.5 × 3.0 = 4.5\n");
    sh_write0("      0x18000 × 0x30000 = ");
    sh_write_hex((uint32_t)product);
    sh_write0("\n");
    sh_write0("      After >> 16: ");
    sh_write_hex(result);
    sh_write0(" (integer part = 4)\n\n");

    sh_write0("    Conversion table:\n");
    sh_write0("      float → Q16.16: (int32_t)(value * 65536.0f + 0.5f)\n");
    sh_write0("      Q16.16 → float: value / 65536.0f\n");
    sh_write0("      Q16.16 add:      a + b  (same as integer)\n");
    sh_write0("      Q16.16 multiply: (int32_t)(((int64_t)a * b) >> 16)\n");
    sh_write0("      Q16.16 divide:   (int32_t)(((int64_t)a << 16) / b)\n\n");
}

/* ==========================================================================
 * 入口
 * ========================================================================== */
void optimization_demo_entry(void)
{
    sh_write0("\n");
    sh_write0("========================================================\n");
    sh_write0("  [Module 2] Compiler Optimization Deep Dive\n");
    sh_write0("========================================================\n\n");

    sh_write0("  To compare optimization levels:\n");
    sh_write0("    1. Build Debug:     cmake -DCMAKE_BUILD_TYPE=Debug ..\n");
    sh_write0("    2. Build MinSizeRel: cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..\n");
    sh_write0("    3. Build Release:   cmake -DCMAKE_BUILD_TYPE=Release ..\n");
    sh_write0("    4. Compare:  diff <(objdump -d Debug/*.elf) <(objdump -d Release/*.elf)\n");
    sh_write0("    5. Check .disasm file in build dir for annotated assembly.\n\n");

    demo_division_optimization();
    demo_volatile_cost();
    demo_struct_packing();
    demo_inline_vs_call();
    demo_loop_unrolling();
    demo_fixed_point();
    demo_m0_optimization_tips();

    sh_write0("  Optimization demo complete.\n\n");
}
