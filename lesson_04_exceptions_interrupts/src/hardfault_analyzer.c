/*
 * =============================================================================
 * hardfault_analyzer.c -- HardFault 异常分析与栈帧解码
 * =============================================================================
 *
 * HardFault 是 Cortex-M0 中最重要的异常之一.
 * M0 没有 MemManage/BusFault/UsageFault, 所有故障都汇聚到 HardFault.
 *
 * 当 HardFault 发生时, 硬件自动在栈上保存 8 个字的异常帧:
 *
 *   高地址 (栈顶方向)
 *   +----------------+
 *   | xPSR           | ← SP + 28 (进入 handler 前的 SP 指向这里之后)
 *   | PC (返回地址)  | ← SP + 24
 *   | LR (R14)       | ← SP + 20
 *   | R12            | ← SP + 16
 *   | R3             | ← SP + 12
 *   | R2             | ← SP + 8
 *   | R1             | ← SP + 4
 *   | R0             | ← SP + 0 (当前 SP 在进入 handler 后)
 *   低地址
 *
 * 从栈帧中可以找到:
 *   - 故障指令的地址 (PC) → 定位问题代码
 *   - 故障时的寄存器状态 → 分析故障原因
 *   - 故障前的 xPSR → 了解处理器状态
 *
 * 常见 HardFault 原因 (M0):
 *   1. 非对齐内存访问 (M0 不支持非对齐访问)
 *   2. 非法指令 (如尝试执行 ARM 状态代码)
 *   3. 除零 (如果启用了除法异常)
 *   4. 访问未定义的地址 (如 0xFFFFFFxx 区域)
 *   5. 栈溢出 (SP 指针跑到 RAM 之外)
 *   6. 从 BKPT 返回但没有调试器 (QEMU 可能不适用)
 */

#include "semihosting.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 异常栈帧结构 (硬件自动压栈的 8 个字)
 * -------------------------------------------------------------------------- */
typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;  /* 异常发生前的 LR */
    uint32_t pc;  /* 故障指令的地址 */
    uint32_t xpsr; /* 程序状态寄存器 */
} exception_frame_t;

/* --------------------------------------------------------------------------
 * hardfault_handler_c -- HardFault 异常处理 (C 部分)
 * --------------------------------------------------------------------------
 * 由 startup.S 的 hardfault_handler 调用.
 * @param frame: 指向异常栈帧的指针
 *
 * 此函数分析栈帧并打印诊断信息.
 * 在真实系统中, 可以:
 *   - 记录故障信息到非易失存储
 *   - 尝试恢复 (如重置出错的子系统)
 *   - 执行系统复位
 * -------------------------------------------------------------------------- */
void hardfault_handler_c(uint32_t *frame)
{
    exception_frame_t *ef = (exception_frame_t *)frame;

    sh_write0("\n========================================\n");
    sh_write0("!!! HARDFAULT DETECTED !!!\n");
    sh_write0("========================================\n\n");

    /* --- 寄存器状态 --- */
    sh_write0("[Stacked Registers]\n");
    sh_write0("  R0   = ");
    sh_write_hex(ef->r0);
    sh_write0("\n  R1   = ");
    sh_write_hex(ef->r1);
    sh_write0("\n  R2   = ");
    sh_write_hex(ef->r2);
    sh_write0("\n  R3   = ");
    sh_write_hex(ef->r3);
    sh_write0("\n  R12  = ");
    sh_write_hex(ef->r12);
    sh_write0("\n  LR   = ");
    sh_write_hex(ef->lr);
    sh_write0("\n");

    /* --- 故障地址 --- */
    sh_write0("\n[Fault Information]\n");
    sh_write0("  Fault PC   = ");
    sh_write_hex(ef->pc);
    sh_write0(" (instruction that caused the fault)\n");
    sh_write0("  Fault xPSR = ");
    sh_write_hex(ef->xpsr);
    sh_write0("\n");

    /* 解码 xPSR */
    uint32_t ipsr = ef->xpsr & 0x1FF; /* 异常编号 (bits 0-8) */
    sh_write0("    IPSR (exception #) = ");
    sh_write_dec(ipsr);
    if (ipsr == 0)
    {
        sh_write0(" (Thread mode - fault in application code)\n");
    }
    else if (ipsr == 3)
    {
        sh_write0(" (HardFault itself - nested fault!)\n");
    }
    else
    {
        sh_write0(" (Handler mode - fault in ISR!)\n");
    }

    /* 解码 xPSR 标志位 */
    uint32_t apsr = ef->xpsr >> 27;
    sh_write0("    APSR: N=");
    sh_writec((apsr & 0x10) ? '1' : '0');
    sh_write0(" Z=");
    sh_writec((apsr & 0x08) ? '1' : '0');
    sh_write0(" C=");
    sh_writec((apsr & 0x04) ? '1' : '0');
    sh_write0(" V=");
    sh_writec((apsr & 0x02) ? '1' : '0');
    sh_write0(" Q=");
    sh_writec((apsr & 0x01) ? '1' : '0');
    sh_write0("\n");

    /* --- 诊断信息 --- */
    sh_write0("\n[Diagnosis]\n");

    /* 检查 PC 是否看起来合理 */
    if (ef->pc < 0x00040000 && ef->pc >= 0x00000000)
    {
        sh_write0("  PC is in Flash region (0x00000000-0x0003FFFF)\n");
    }
    else if (ef->pc >= 0x20000000 && ef->pc < 0x20004000)
    {
        sh_write0("  PC is in RAM region! Code running from RAM?\n");
    }
    else
    {
        sh_write0("  PC is in UNEXPECTED region - likely invalid jump!\n");
    }

    /* 检查 LR 的 EXC_RETURN 值 */
    sh_write0("  LR (EXC_RETURN) = ");
    sh_write_hex(ef->lr);
    if (ef->lr == 0xFFFFFFF9)
    {
        sh_write0(" (return to Thread mode with MSP)\n");
    }
    else if (ef->lr == 0xFFFFFFFD)
    {
        sh_write0(" (return to Thread mode with PSP)\n");
    }
    else if (ef->lr == 0xFFFFFFF1)
    {
        sh_write0(" (return to Handler mode - nested fault!)\n");
    }
    else
    {
        sh_write0(" (INVALID EXC_RETURN VALUE!)\n");
    }

    /* 检查 LR bit[0] 是否为 1 (Thumb 状态) */
    if ((ef->pc & 1) == 0)
    {
        sh_write0("  WARNING: PC bit[0] = 0! M0 requires Thumb state (bit[0]=1)\n");
    }

    /* 检查栈指针是否在有效范围内 */
    uint32_t sp_val;
    __asm__ volatile("mov %0, sp" : "=r"(sp_val));
    sh_write0("  Current SP = ");
    sh_write_hex(sp_val);
    if (sp_val >= 0x20000000 && sp_val < 0x20004000)
    {
        sh_write0(" (in valid RAM)\n");
    }
    else
    {
        sh_write0(" (OUTSIDE valid RAM! Stack overflow?)\n");
    }

    sh_write0("\n========================================\n");
    sh_write0("  System halted. (Ctrl+A X to exit QEMU)\n");
    sh_write0("========================================\n\n");
}

/* --------------------------------------------------------------------------
 * trigger_hardfault -- 故意触发 HardFault 以演示分析器
 * --------------------------------------------------------------------------
 * 以下几种触发方式:
 *   1. 执行未定义指令 (M0 上的非法指令)
 *   2. 非对齐内存访问
 *   3. 跳转到无效地址
 *
 * 通过参数选择触发方式.
 * -------------------------------------------------------------------------- */
void trigger_hardfault(int method)
{
    sh_write0("    Triggering HardFault via method #");
    sh_write_dec(method);
    sh_write0("...\n");

    switch (method)
    {
    case 1:
    {
        /* 方法 1: 跳转到无效地址 (RAM 中无法执行的区域)
         * 创建一个函数指针指向 RAM 地址, 然后"调用"它 */
        sh_write0("    (Jump to invalid address in RAM)\n");
        /* 定义一个函数指针指向 RAM 中的随机地址 */
        void (*bad_func)(void) = (void (*)(void))0x20001000;
        bad_func(); /* ← 这会触发 HardFault! */
        break;
    }
    case 2:
    {
        /* 方法 2: 非对齐的 32 位内存访问
         * M0 不支持非对齐的字/半字访问 */
        sh_write0("    (Unaligned 32-bit memory access)\n");
        uint8_t buf[8];
        uint32_t addr = (uint32_t)&buf[1]; /* 非字对齐地址 */
        volatile uint32_t *p = (volatile uint32_t *)addr;
        (void)*p; /* ← 这会触发 HardFault (非对齐访问)! */
        break;
    }
    case 3:
    {
        /* 方法 3: 执行 SVC 指令但没有 SVC 处理函数
         * (如果 SVC handler 未正确配置) */
        sh_write0("    (This method is safe for now)\n");
        break;
    }
    default:
        sh_write0("    Unknown method\n");
        break;
    }
}

/* --------------------------------------------------------------------------
 * demo_hardfault -- 演示 HardFault 分析
 * -------------------------------------------------------------------------- */
void demo_hardfault(void)
{
    sh_write0("[3] HardFault Analyzer\n");
    sh_write0(
        "    Cortex-M0 has only HardFault (no MemManage/BusFault/UsageFault).\n"
        "    When a fault occurs, hardware pushes 8 words onto the stack.\n"
        "    The HardFault handler can decode this frame to diagnose the cause.\n\n");

    sh_write0(
        "    [Stack frame layout on exception entry]:\n"
        "      SP+28: xPSR\n"
        "      SP+24: PC (fault address!)\n"
        "      SP+20: LR\n"
        "      SP+16: R12\n"
        "      SP+12: R3\n"
        "      SP+8:  R2\n"
        "      SP+4:  R1\n"
        "      SP+0:  R0\n\n");

    sh_write0(
        "    To keep the demo safe, we will NOT actually trigger a HardFault.\n"
        "    Uncomment trigger_hardfault(1) in main.c to test the analyzer.\n"
        "    (HardFault would halt the CPU, ending the demo.)\n\n");
}
