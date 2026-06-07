/*
 * =============================================================================
 * inline_asm_demo.c -- GCC 内联汇编演示
 * =============================================================================
 *
 * GCC 支持两种内联汇编:
 * 1. 基本 asm: asm("instruction");  -- 简单, 无法与 C 变量交互
 * 2. 扩展 asm: asm("template" : outputs : inputs : clobbers);
 *
 * 扩展 asm 语法详解:
 *   asm [volatile] (
 *       "assembly template"   @ 汇编指令, 用 %0,%1... 引用操作数
 *       : output operands     @ "=r"(var)  = 写入, "+r"(var) = 读写
 *       : input operands      @ "r"(var) 只读
 *       : clobber list        @ 被破坏的寄存器, "memory" 等
 *   );
 *
 * M0 常用约束:
 *   "r"  - 通用寄存器 (R0-R12 或 R0-R7 取决于指令)
 *   "l"  - 低寄存器 (R0-R7, 强制)
 *   "I"  - 8 位立即数 (0-255)
 *   "m"  - 内存操作数
 *   "=r" - 只写寄存器
 *   "+r" - 读写寄存器
 *
 * 实际用途:
 *   - 访问特殊寄存器 (PRIMASK, CONTROL, etc.)
 *   - 执行 WFI/WFE 等特殊指令
 *   - 临界区 (disable/enable interrupts)
 *   - 精确的周期计数
 * =============================================================================
 */

#include <stdint.h>

/* 外部声明 */
void demo_inline_asm(void);

/* 辅助输出 */
static void semihosting_write0(const char *s);
static void semihosting_write_hex(uint32_t v);

/*
 * =========================================================================
 * 示例 1: 基本内联汇编 -- 读取/写入特殊寄存器
 * =========================================================================
 *
 * Cortex-M0 有两个重要的特殊寄存器:
 *   PRIMASK: 全局中断使能 (bit0: 1=屏蔽中断, 0=允许中断)
 *   CONTROL: 控制寄存器 (bit0: nPRIV, bit1: SPSEL)
 *
 * 这些寄存器必须通过 MRS/MSR 指令访问, 没有 C 语言等价物.
 * 内联汇编是访问它们的唯一方式.
 */

/* 读取 PRIMASK 寄存器 (中断屏蔽状态) */
static uint32_t get_primask(void)
{
    uint32_t result;
    __asm__ volatile(
        "mrs %0, PRIMASK\n" /* MRS: Move from Special Register */
        : "=r"(result)       /* %0 = result, 只写 */
    );
    return result;
}

/* 设置 PRIMASK (屏蔽/开启中断) */
static void set_primask(uint32_t value)
{
    __asm__ volatile(
        "msr PRIMASK, %0\n" /* MSR: Move to Special Register */
        :                    /* 无输出 */
        : "r"(value)         /* %0 = value, 只读 */
        : "memory"           /* 影响全局状态, 通知编译器 */
    );
}

/* 读取 CONTROL 寄存器 */
static uint32_t get_control(void)
{
    uint32_t result;
    __asm__ volatile("mrs %0, CONTROL\n" : "=r"(result));
    return result;
}

/*
 * =========================================================================
 * 示例 2: WFI/WFE 低功耗指令
 * =========================================================================
 * WFI: Wait For Interrupt  -- 休眠直到中断发生
 * WFE: Wait For Event      -- 休眠直到事件发生
 *
 * 这些是 M0 低功耗的基础, 只能通过内联汇编访问.
 */

static void cpu_wfi(void)
{
    __asm__ volatile("wfi");
}

static void cpu_wfe(void)
{
    __asm__ volatile("wfe");
}

/* 发送事件 (唤醒另一个 WFE 等待者) */
static void cpu_sev(void)
{
    __asm__ volatile("sev");
}

/*
 * =========================================================================
 * 示例 3: 临界区保护 -- 关中断/开中断
 * =========================================================================
 * 在访问共享资源时, 短暂关闭中断以保证原子性.
 * 这是嵌入式系统中最重要的内联汇编应用之一.
 *
 * 实现: 保存 PRIMASK → 关中断 → 临界区操作 → 恢复 PRIMASK
 */

/* 进入临界区: 屏蔽中断, 返回之前的中断状态 */
static uint32_t critical_enter(void)
{
    uint32_t primask;
    __asm__ volatile(
        "mrs  %0, PRIMASK\n" /* 读取当前中断状态 */
        "cpsid i\n"          /* 屏蔽中断 (Change Processor State, Disable IRQ) */
        : "=r"(primask)
        :
        : "memory");
    return primask; /* 返回之前的状态, 供 critical_exit 恢复 */
}

/* 退出临界区: 恢复中断状态 */
static void critical_exit(uint32_t saved_primask)
{
    __asm__ volatile(
        "msr PRIMASK, %0\n" /* 恢复之前的中断状态 */
        :
        : "r"(saved_primask)
        : "memory");
}

/*
 * =========================================================================
 * 示例 4: 精确的指令序列控制
 * =========================================================================
 *
 * 编译器可能重新排序指令, 内联汇编可以:
 *   - 插入编译器屏障 (防止指令重排)
 *   - 插入数据同步屏障 (DSB) -- 等待所有内存访问完成
 *   - 插入指令同步屏障 (ISB) -- 刷新流水线
 *
 * M0 没有 DSB/ISB 指令! (这些是 ARMv7-M 的)
 * M0 只有 DMB (Data Memory Barrier), 但 nRF51 可能也不支持.
 * 实际上, M0 的简单流水线 (3 级) 使大多数屏障操作不必要.
 */

/* 编译器屏障: 防止编译器跨此点重排内存访问 */
static void compiler_barrier(void)
{
    __asm__ volatile("" ::: "memory");
}

/* M0 使用 MRS/MSR 实现粗略的同步 */

/*
 * =========================================================================
 * 示例 5: 内联汇编中的算术 (演示约束使用)
 * =========================================================================
 */

/* 用内联汇编实现饱和加法 (saturating add) */
__attribute__((unused))
static uint32_t saturating_add(uint32_t a, uint32_t b)
{
    /* saturating_add via inline asm
     * Thumb-1 2-operand form: ADDS Rd, Rm  (Rd = Rd + Rm) */
    register uint32_t ra __asm__("r0") = a;
    register uint32_t rb __asm__("r1") = b;
    /* .syntax unified 解决 GCC 生成的 .syntax divided 模式下
     * ADDS Rd,Rm 不被接受的问题 */
    __asm__ volatile(
        ".syntax unified\n"
        "adds %0, %1\n"   /* Rd += Rm, Thumb-1: both must be low regs */
        "bcc   1f\n"      /* branch if no carry (no overflow) */
        "ldr   %0, =0xFFFFFFFF\n" /* saturate */
        "1:\n"
        : "+r"(ra)
        : "r"(rb)
        : "cc"
    );
    return ra;
}

/* 用内联汇编实现 64 位加法 (演示多寄存器输出)
 * 注意: 此函数未在 demo 中调用, 仅作为内联 asm 多寄存器输出示例保留
 */
__attribute__((unused))
static uint64_t add64(uint64_t x, uint64_t y)
{
    /* M0 上 64 位加法: 编译器自动生成正确的 Thumb-1 序列:
     *   adds r0, r0, r2    @ lo += lo_y
     *   adcs r1, r1, r3    @ hi += hi_y + C
     * 对大多数算术运算, 信任编译器比手写内联汇编更好.
     * 内联汇编仅用于: 特殊指令 (WFI/CPSID/MRS/MSR) 和临界区.
     */
    return x + y;
}

/*
 * =========================================================================
 * 示例 6: 访问 Cortex-M0 系统控制寄存器
 * =========================================================================
 */

/* 读取 CPUID (CPUID 寄存器在地址 0xE000ED00) */
static uint32_t read_cpuid(void)
{
    uint32_t cpuid;
    __asm__ volatile(
        "ldr %0, [%1, #0]\n"
        : "=r"(cpuid)
        : "r"(0xE000ED00)
    );
    return cpuid;
}

/* M0 的 CPUID 值应该是 0x410CC200 或 0x410CC600 (M0 r0p0/r0p1) */
/* 实际在 QEMU cortex-m0 中可能不同 */

/*
 * =========================================================================
 * 主演示函数
 * =========================================================================
 */
void demo_inline_asm(void)
{
    semihosting_write0("[2] GCC Inline Assembly\n");
    semihosting_write0("    Accessing special registers & instructions\n\n");

    /* --- PRIMASK 操作 --- */
    semihosting_write0("    [PRIMASK: Global interrupt mask]\n");
    uint32_t pm = get_primask();
    semihosting_write0("    PRIMASK = ");
    semihosting_write_hex(pm);
    semihosting_write0(" (0=IRQ enabled, 1=masked)\n");

    /* --- CONTROL 寄存器 --- */
    semihosting_write0("\n    [CONTROL: Privilege & stack selection]\n");
    uint32_t ct = get_control();
    semihosting_write0("    CONTROL = ");
    semihosting_write_hex(ct);
    semihosting_write0("\n");
    semihosting_write0("    bit0 (nPRIV): ");
    semihosting_write_hex(ct & 1);
    semihosting_write0(" (0=privileged, 1=unprivileged)\n");
    semihosting_write0("    bit1 (SPSEL): ");
    semihosting_write_hex((ct >> 1) & 1);
    semihosting_write0(" (0=MSP, 1=PSP)\n");

    /* --- 临界区 --- */
    semihosting_write0("\n    [Critical section example]\n");
    semihosting_write0("    Entering critical section...\n");
    uint32_t saved = critical_enter();
    semihosting_write0("    Inside critical section (IRQs masked)\n");
    semihosting_write0("    Saved PRIMASK = ");
    semihosting_write_hex(saved);
    semihosting_write0("\n");
    critical_exit(saved);
    semihosting_write0("    Exited critical section (IRQs restored)\n");

    /* --- 饱和加法 --- */
    semihosting_write0("\n    [Saturating add via inline asm]\n");
    uint32_t sat = saturating_add(0xFFFFFFF0, 0x20);
    semihosting_write0("    saturating_add(0xFFFFFFF0, 0x20) = ");
    semihosting_write_hex(sat);
    semihosting_write0(" (saturated to 0xFFFFFFFF)\n");

    /* --- CPUID --- */
    semihosting_write0("\n    [CPUID register]\n");
    uint32_t id = read_cpuid();
    semihosting_write0("    CPUID @ 0xE000ED00 = ");
    semihosting_write_hex(id);
    semihosting_write0("\n");

    /* --- 低功耗指令 --- */
    semihosting_write0("\n    [Low-power instructions: WFI/WFE/SEV]\n");
    semihosting_write0("    WFI/WFE not executed (would stall QEMU)\n");
    semihosting_write0("    Use: __asm__ volatile(\"wfi\"); in idle loop\n");
}

/* =========================================================================
 * 本地辅助
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
