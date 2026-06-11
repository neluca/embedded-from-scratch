/*
 * =============================================================================
 * gdb_demo.c -- GDB 远程调试实战演示
 * =============================================================================
 *
 * 本模块提供一组精心设计的函数, 用于在 GDB 中练习嵌入式调试技能.
 *
 * 启动 GDB 调试会话:
 *   Terminal 1: qemu-system-arm -M microbit -kernel lesson_08_advanced.elf
 *               -semihosting -nographic -s -S
 *   Terminal 2: arm-none-eabi-gdb lesson_08_advanced.elf
 *               (gdb) target remote :1234
 *               (gdb) break gdb_demo_entry
 *               (gdb) continue
 *
 * 建议按照函数顺序逐步练习 (gdb_demo_1 → gdb_demo_5).
 * 每个函数在注释中标注了推荐的 GDB 练习命令.
 * =============================================================================
 */

#include "semihosting.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 全局数据 -- 用于练习内存检查和监视点
 * -------------------------------------------------------------------------- */

/* 一个已知 pattern 的数组, 方便在 GDB 中用 x/ 命令检查 */
static uint32_t g_pattern[8] = {
    0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0x0A0B0C0D,
    0xAA55AA55, 0x55AA55AA, 0xFFFFFFFF, 0x00000000
};

/* 被监视的变量 -- 用于 watchpoint 练习 */
static volatile uint32_t g_watch_target = 0;

/* 简单状态机的状态变量 -- 体会单步执行 */
typedef enum {
    STATE_IDLE,
    STATE_ARMED,
    STATE_TRIGGERED,
    STATE_COMPLETE
} state_t;

static volatile state_t g_fsm_state = STATE_IDLE;
static volatile uint32_t g_fsm_ticks = 0;

/* --------------------------------------------------------------------------
 * gdb_state_machine_step -- 简单状态机 (单步练习)
 * --------------------------------------------------------------------------
 *
 * GDB 练习:
 *   1. break gdb_state_machine_step
 *   2. continue (每次触发都会停在这里)
 *   3. print g_fsm_state (查看当前状态)
 *   4. print g_fsm_ticks (查看计数)
 *   5. info locals (查看局部变量)
 *
 * 状态转换: IDLE → ARMED → TRIGGERED → COMPLETE
 * -------------------------------------------------------------------------- */
static void gdb_state_machine_step(void)
{
    switch (g_fsm_state)
    {
    case STATE_IDLE:
        g_fsm_ticks++;
        if (g_fsm_ticks >= 3)
        {
            g_fsm_state = STATE_ARMED;
            g_fsm_ticks = 0;
        }
        break;

    case STATE_ARMED:
        g_fsm_ticks++;
        if (g_fsm_ticks >= 2)
        {
            g_fsm_state = STATE_TRIGGERED;
            g_fsm_ticks = 0;
        }
        break;

    case STATE_TRIGGERED:
        g_fsm_state = STATE_COMPLETE;
        break;

    case STATE_COMPLETE:
        /* 停留在完成状态 */
        break;
    }
}

/* --------------------------------------------------------------------------
 * sum_array_buggy -- 带 bug 的数组求和函数 (断点+排查练习)
 * --------------------------------------------------------------------------
 *
 * BUG: 循环条件 i <= len 应该是 i < len.
 *      当 i == len 时, arr[len] 越界访问!
 *
 * GDB 练习:
 *   1. break sum_array_buggy
 *   2. run (程序会在函数入口停下)
 *   3. step (单步进入循环)
 *   4. print i (第一次是 0)
 *   5. continue → print i → 观察 i 变得等于 len 时发生了什么
 *   6. print arr[len] (看到越界读取的值)
 *
 * 思考: 为什么这个 bug 在某些情况下不会立即崩溃?
 *   答: arr[len] 可能刚好在合法内存范围内 (栈上的其他变量).
 *       这是最危险的 bug -- 它"看起来能工作"但数据已经损坏.
 * -------------------------------------------------------------------------- */
static uint32_t sum_array_buggy(const uint32_t *arr, uint32_t len)
{
    uint32_t sum = 0;
    /* BUG: i <= len -- off by one! Should be i < len */
    for (uint32_t i = 0; i <= len; i++)
    {
        sum += arr[i]; /* 最后一次迭代读取 arr[len] (越界!) */
    }
    return sum;
}

/* --------------------------------------------------------------------------
 * recursive_fibonacci -- 深度递归函数 (backtrace 练习)
 * --------------------------------------------------------------------------
 *
 * 此函数故意使用递归而非迭代, 以便在 GDB 中观察栈帧.
 *
 * GDB 练习:
 *   1. break recursive_fibonacci
 *   2. continue (每次递归调用都会停在这里)
 *   3. backtrace (查看调用栈深度)
 *   4. info frame (查看当前栈帧详情)
 *   5. frame 0 → frame 1 → frame 2 ... (在栈帧间切换)
 *   6. info registers (查看 R0 (参数 n) 和 LR)
 *
 * 栈帧结构 (每次调用):
 *   PUSH {lr}          ← 保存返回地址 (4 bytes)
 *   SUB sp, sp, #...   ← 局部变量空间
 *   ... 计算 ...
 *   ADD sp, sp, #...   ← 恢复栈
 *   POP {pc}           ← 返回
 *
 * M0 的栈帧非常简洁: 没有帧指针 (R11), 没有保存 R4-R11
 * (除非被调用者保存寄存器需要在递归中存活).
 * -------------------------------------------------------------------------- */
static uint32_t recursive_fibonacci(uint32_t n)
{
    /* 用 volatile 阻止编译器优化为迭代 */
    volatile uint32_t local_copy = n;

    if (local_copy <= 1)
    {
        return local_copy;
    }

    uint32_t a = recursive_fibonacci(local_copy - 1);
    uint32_t b = recursive_fibonacci(local_copy - 2);
    return a + b;
}

/* --------------------------------------------------------------------------
 * watchpoint_demo -- 监视点演示 (观察变量何时被修改)
 * --------------------------------------------------------------------------
 *
 * GDB 练习:
 *   1. break watchpoint_demo
 *   2. continue (停在函数入口)
 *   3. watch g_watch_target (设置硬件监视点)
 *   4. continue (每次 g_watch_target 被写入时 GDB 会停下)
 *   5. print g_watch_target (查看新值)
 *   6. info watchpoints (列出所有监视点)
 *
 * M0 限制: M0 没有调试监视点单元 (DWT).
 *          但在 QEMU 中, GDB 使用软件模拟的监视点.
 *          在真实硬件上, 取决于调试器的能力.
 * -------------------------------------------------------------------------- */
static void watchpoint_demo(void)
{
    sh_write0("    Watchpoint demo: g_watch_target will be modified 5 times.\n");
    sh_write0("    Set: (gdb) watch g_watch_target\n\n");

    for (int i = 0; i < 5; i++)
    {
        g_watch_target = (uint32_t)(i * 0x1111); /* ← 每次都会触发监视点 */
    }

    sh_write0("    Final g_watch_target = ");
    sh_write_hex(g_watch_target);
    sh_write0("\n\n");
}

/* --------------------------------------------------------------------------
 * register_inspection -- 寄存器查看练习
 * --------------------------------------------------------------------------
 *
 * GDB 练习:
 *   1. break register_inspection
 *   2. continue
 *   3. info registers           (显示所有 CPU 寄存器)
 *   4. info registers r0 r1 r2  (只显示 R0-R2)
 *   5. print/x $sp              (查看栈指针, 十六进制)
 *   6. print/x $pc              (查看程序计数器)
 *   7. x/16x $sp                (查看栈顶 16 个 32 位字)
 * -------------------------------------------------------------------------- */
static void register_inspection(void)
{
    /* 故意把数据放在寄存器中, 方便在 GDB 中查看 */
    register uint32_t r4 __asm__("r4") = 0xDEAD0001;
    register uint32_t r5 __asm__("r5") = 0xDEAD0002;
    register uint32_t r6 __asm__("r6") = 0xDEAD0003;

    sh_write0("    Register inspection demo:\n");
    sh_write0("    R4 = 0xDEAD0001, R5 = 0xDEAD0002, R6 = 0xDEAD0003\n");
    sh_write0("    Check with: (gdb) info registers r4 r5 r6\n\n");

    /* 阻止编译器优化掉未使用的 register 变量 */
    uint32_t check = r4 ^ r5 ^ r6;
    sh_write0("    XOR check = ");
    sh_write_hex(check);
    sh_write0(" (expect 0xDEAD0000)\n\n");
}

/* --------------------------------------------------------------------------
 * memory_examination -- 内存检查练习
 * --------------------------------------------------------------------------
 *
 * GDB 练习:
 *   1. break memory_examination
 *   2. continue (停在第一个 sh_write0)
 *   3. x/8x g_pattern         (以十六进制查看 g_pattern 数组)
 *   4. x/8x &g_pattern        (另一种写法)
 *   5. x/4x g_pattern+16      (从偏移 16 字节开始看)
 *   6. x/s (char*)g_pattern   (以字符串形式查看 -- 会是乱码)
 *   7. print sizeof(g_pattern)
 * -------------------------------------------------------------------------- */
static void memory_examination(void)
{
    sh_write0("    Memory examination demo:\n");
    sh_write0("    Global array g_pattern[8] at &g_pattern.\n");
    sh_write0("    Inspect with: (gdb) x/8x &g_pattern\n");
    sh_write0("    Contents: DEADBEEF CAFEBABE 12345678 0A0B0C0D ...\n\n");

    /* 验证数组内容 */
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++)
    {
        sum += g_pattern[i];
    }
    sh_write0("    Sum of g_pattern[0..7] = ");
    sh_write_hex(sum);
    sh_write0("\n\n");
}

/* --------------------------------------------------------------------------
 * disassembly_demo -- 反汇编查看练习
 * --------------------------------------------------------------------------
 *
 * GDB 练习:
 *   1. break disassembly_demo
 *   2. continue
 *   3. disas (查看当前函数的反汇编)
 *   4. disas /m (混合源码和汇编 -- 非常有用!)
 *   5. x/10i $pc (从当前 PC 开始显示 10 条指令)
 *
 * 建议对比 M0 (Thumb-1) 和 M3/M4 (Thumb-2) 的反汇编输出.
 * M0 的指令都只有 16 位宽 (除了 BL 是 32 位).
 * -------------------------------------------------------------------------- */
static void disassembly_demo(void)
{
    /* 这段代码故意写得简单, 方便看反汇编 */
    uint32_t a = 100;
    uint32_t b = 7;
    /* 加法: ADDS r?, r?, r? -- 2 bytes, 1 cycle */
    uint32_t sum = a + b;
    (void)sum;

    /* 除法: BL __aeabi_uidiv -- 4 bytes + ~70 bytes in libgcc, ~100 cycles */
    uint32_t quotient = a / b;
    (void)quotient;

    /* 移位代替除法: LSRS r?, r?, #3 -- 2 bytes, 1 cycle */
    uint32_t shifted = a >> 3;
    (void)shifted;

    sh_write0("    Disassembly demo: a=100, b=7\n");
    sh_write0("    a + b = ");
    sh_write_dec(a + b);
    sh_write0(", a / b = ");
    sh_write_dec(a / b);
    sh_write0(", a >> 3 = ");
    sh_write_dec(a >> 3);
    sh_write0("\n\n");

    sh_write0("    Disassemble step by step from (gdb) disas /m disassembly_demo\n");
    sh_write0("    Note: 'a / b' generates BL __aeabi_uidiv (software division!)\n");
    sh_write0("          'a >> 3' generates LSRS r?, r?, #3 (single instruction!)\n\n");
}

/* --------------------------------------------------------------------------
 * breakpoint_types -- 断点类型演示
 * --------------------------------------------------------------------------
 *
 * GDB 练习:
 *   1. info breakpoints     (列出所有断点)
 *   2. break gdb_demo_4     (在函数入口设断点)
 *   3. hbreak *0xNNNN       (设置硬件断点, 用于 Flash 中的代码)
 *   4. break gdb_demo_4 if g_watch_target > 100 (条件断点)
 *   5. delete 1             (删除 1 号断点)
 *   6. disable 2            (禁用 2 号断点)
 *   7. ignore 1 5           (忽略 1 号断点的前 5 次触发)
 *
 * 注意: QEMU 中 hbreak 和 break 行为类似.
 *       真实硬件上, software breakpoint 修改 Flash 中的指令为 BKPT,
 *       hardware breakpoint 使用调试单元的硬件比较器.
 * -------------------------------------------------------------------------- */
static void breakpoint_types(void)
{
    sh_write0("    Breakpoint types demo:\n");
    sh_write0("    Try setting different breakpoint types on the next statements:\n\n");

    /* 每行都是一个理想的断点目标 */
    volatile uint32_t bp1 = 111; /* 设断点: break *&bp1 */
    volatile uint32_t bp2 = 222; /* 条件断点: break *&bp2 if bp1 == 111 */
    volatile uint32_t bp3 = 333; /* 硬件断点: hbreak *&bp3 */

    sh_write0("    bp1=");
    sh_write_dec(bp1);
    sh_write0(" bp2=");
    sh_write_dec(bp2);
    sh_write0(" bp3=");
    sh_write_dec(bp3);
    sh_write0("\n\n");
}

/* --------------------------------------------------------------------------
 * gdb_command_reference -- GDB 命令速查表
 * --------------------------------------------------------------------------
 *
 * 此函数仅作文档参考, 打印到 semihosting 输出.
 * 建议同时打开 scripts/debug.gdb 查看预配置的 GDB 初始化脚本.
 * -------------------------------------------------------------------------- */
static void gdb_command_reference(void)
{
    sh_write0("  ┌──────────────────────────────────────────────────────┐\n");
    sh_write0("  │         GDB Command Quick Reference (Embedded)       │\n");
    sh_write0("  ├────────────┬─────────────────────────────────────────┤\n");
    sh_write0("  │ break func │ Set breakpoint at function              │\n");
    sh_write0("  │ hbreak *addr│ Hardware breakpoint (Flash-compatible)  │\n");
    sh_write0("  │ watch var  │ Stop when variable is written           │\n");
    sh_write0("  │ step / s   │ Step one line (enters function calls)   │\n");
    sh_write0("  │ next / n   │ Step one line (over function calls)     │\n");
    sh_write0("  │ stepi      │ Step one machine instruction            │\n");
    sh_write0("  │ continue   │ Resume execution                        │\n");
    sh_write0("  │ finish     │ Run until current function returns      │\n");
    sh_write0("  │ bt         │ Print backtrace (call stack)            │\n");
    sh_write0("  │ info regs  │ Show all CPU registers                  │\n");
    sh_write0("  │ print/x    │ Print expression in hex                 │\n");
    sh_write0("  │ x/NFU addr │ Examine memory: N=count, F=fmt, U=size  │\n");
    sh_write0("  │   Fmt: x=hex d=dec s=string i=instruction            │\n");
    sh_write0("  │   Size: b=1 h=2 w=4 g=8                              │\n");
    sh_write0("  │ disas /m   │ Disassemble with source lines            │\n");
    sh_write0("  │ monitor     │ Send command to QEMU (e.g. system_reset)│\n");
    sh_write0("  │ set var=val │ Modify variable during debug            │\n");
    sh_write0("  └────────────┴─────────────────────────────────────────┘\n\n");
}

/* ==========================================================================
 * 入口函数: 按顺序调用所有 GDB 练习
 * ========================================================================== */

void gdb_demo_1_breakpoints_and_stepping(void)
{
    sh_write0("\n--- GDB Demo 1: Breakpoints & Stepping ---\n\n");

    sh_write0("  Goal: practice breakpoint placement and single-stepping.\n");
    sh_write0("  Set breakpoint at gdb_state_machine_step and 'continue' 3 times.\n");
    sh_write0("  Watch g_fsm_state change: IDLE→ARMED→TRIGGERED→COMPLETE.\n\n");

    /* 运行状态机 6 步: 3(IDLE) + 2(ARMED) + 1(TRIGGERED) = 6 次调用 */
    while (g_fsm_state != STATE_COMPLETE)
    {
        gdb_state_machine_step();
    }

    sh_write0("  State machine reached COMPLETE. Verify with: print g_fsm_state\n\n");
}

void gdb_demo_2_watchpoints_and_memory(void)
{
    sh_write0("\n--- GDB Demo 2: Watchpoints & Memory Examination ---\n\n");

    watchpoint_demo();
    memory_examination();
}

void gdb_demo_3_registers_and_backtrace(void)
{
    sh_write0("\n--- GDB Demo 3: Register Inspection & Backtrace ---\n\n");

    register_inspection();

    sh_write0("  Now computing fibonacci(8) recursively...\n");
    sh_write0("  Set: (gdb) break recursive_fibonacci\n");
    sh_write0("  Then 'continue' a few times and type 'backtrace'\n\n");

    uint32_t result = recursive_fibonacci(8);
    sh_write0("  fibonacci(8) = ");
    sh_write_dec(result);
    sh_write0(" (expect 21)\n\n");
}

void gdb_demo_4_disassembly_and_code_exploration(void)
{
    sh_write0("\n--- GDB Demo 4: Disassembly & Code Exploration ---\n\n");

    disassembly_demo();
    breakpoint_types();
}

void gdb_demo_5_bug_hunting(void)
{
    sh_write0("\n--- GDB Demo 5: Bug Hunting Exercise ---\n\n");

    sh_write0("  The function sum_array_buggy() has an off-by-one bug.\n");
    sh_write0("  It accesses arr[len] which is out of bounds.\n\n");

    sh_write0("  Exercise:\n");
    sh_write0("    1. (gdb) break sum_array_buggy\n");
    sh_write0("    2. (gdb) continue\n");
    sh_write0("    3. (gdb) step (single-step through the loop)\n");
    sh_write0("    4. (gdb) print i (watch the index)\n");
    sh_write0("    5. Find where i == len and observe arr[i] value\n");
    sh_write0("    6. Fix: change 'i <= len' to 'i < len'\n\n");

    uint32_t test_arr[5] = {10, 20, 30, 40, 50};
    uint32_t result = sum_array_buggy(test_arr, 5);

    sh_write0("  Expected sum = 150 (10+20+30+40+50)\n");
    sh_write0("  Buggy result = ");
    sh_write_dec(result);
    sh_write0("\n");
    sh_write0("  (The extra value is whatever happens to be at test_arr[5] on the stack!)\n\n");
}

/* --------------------------------------------------------------------------
 * gdb_demo_entry -- 主入口, 运行所有 GDB 练习
 * -------------------------------------------------------------------------- */
void gdb_demo_entry(void)
{
    sh_write0("\n");
    sh_write0("========================================================\n");
    sh_write0("  [Module 1] GDB Remote Debugging Practical\n");
    sh_write0("========================================================\n\n");

    sh_write0("  QEMU is a GDB server on localhost:1234.\n");
    sh_write0("  All exercises in this module assume GDB is attached.\n\n");

    gdb_command_reference();

    gdb_demo_1_breakpoints_and_stepping();
    gdb_demo_2_watchpoints_and_memory();
    gdb_demo_3_registers_and_backtrace();
    gdb_demo_4_disassembly_and_code_exploration();
    gdb_demo_5_bug_hunting();

    sh_write0("  All GDB demos complete.\n");
    sh_write0("  Tip: re-run in GDB with: qemu-system-arm -M microbit\n");
    sh_write0("       -kernel <elf> -semihosting -nographic -s -S\n\n");
}
