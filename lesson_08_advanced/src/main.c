/*
 * =============================================================================
 * Lesson 8 -- ARM Cortex-M0 高级主题与完整演示
 * =============================================================================
 *
 * 综合运用 Phases 1-7 的知识, 展示:
 *   1. GDB 远程调试 (QEMU -s -S + arm-none-eabi-gdb)
 *   2. 编译优化分析 (-O0 vs -Os vs -O2 代码大小对比)
 *   3. WFI 低功耗指令
 *   4. 从仿真到真实硬件的迁移指南
 *   5. 构建管道与版本信息
 *
 * 运行方式:
 *   正常: qemu-system-arm -M microbit -kernel lesson_08.elf -semihosting -nographic
 *   调试: qemu-system-arm -M microbit -kernel lesson_08.elf -semihosting -nographic -s -S
 *         arm-none-eabi-gdb -x scripts/debug.gdb lesson_08.elf
 * =============================================================================
 */

#include <stdint.h>

/* --------------------------------------------------------------------------
 * Semihosting 辅助
 * -------------------------------------------------------------------------- */
static void sh_write0(const char *s)
{
    register int r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = s;
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
static void sh_write_hex(uint32_t v)
{
    char b[] = "0x00000000 ";
    for (int i = 9; i >= 2; i--)
    {
        uint32_t n = v & 0xF;
        b[i] = n < 10 ? '0' + n : 'A' + n - 10;
        v >>= 4;
    }
    sh_write0(b);
}

/* --------------------------------------------------------------------------
 * 编译时版本信息 (通过 CMake 传入或在命令行定义)
 * -------------------------------------------------------------------------- */
#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif
#ifndef BUILD_DATE
#define BUILD_DATE __DATE__ " " __TIME__
#endif

/* --------------------------------------------------------------------------
 * 演示 1: GDB 调试接口
 * -------------------------------------------------------------------------- */
static void demo_gdb_debugging(void)
{
    sh_write0("[1] GDB Remote Debugging\n\n");

    sh_write0("    QEMU provides GDB server on localhost:1234.\n\n");
    sh_write0("    Terminal 1 (QEMU):\n");
    sh_write0("      qemu-system-arm -M microbit -kernel lesson_08.elf \\\n");
    sh_write0("        -semihosting -nographic -s -S\n\n");
    sh_write0("    Terminal 2 (GDB):\n");
    sh_write0("      arm-none-eabi-gdb -x scripts/debug.gdb lesson_08.elf\n\n");

    sh_write0("    Key GDB commands for embedded debugging:\n");
    sh_write0("      info registers          - 显示所有 CPU 寄存器\n");
    sh_write0("      x/10x \\$sp              - 查看栈顶 10 个 32 位字\n");
    sh_write0("      disas /m                - 混合源码和汇编\n");
    sh_write0("      hbreak function_name    - 硬件断点 (Flash 中)\n");
    sh_write0("      info breakpoints        - 列出断点\n");
    sh_write0("      monitor system_reset    - 复位 CPU\n\n");

    /* 故意设置一个断点演示位置 */
    volatile int debug_here = 42;
    (void)debug_here; /* 在这里设置断点: b *(&debug_here) */
    sh_write0("    (Set breakpoint at debug_here variable to test GDB)\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 2: 编译优化分析
 * -------------------------------------------------------------------------- */
static void demo_optimization(void)
{
    sh_write0("[2] Compiler Optimization Analysis\n\n");

    sh_write0("    Optimization flags for Cortex-M0:\n\n");
    sh_write0("    -O0: No optimization. Easy to debug, largest code.\n");
    sh_write0("         Use during development.\n\n");
    sh_write0("    -Os: Optimize for SIZE. Default for embedded.\n");
    sh_write0("         Balances size reduction with speed.\n");
    sh_write0("         Typical saving: 30-50% vs -O0\n\n");
    sh_write0("    -O2: Optimize for SPEED. More aggressive.\n");
    sh_write0("         May increase code size vs -Os.\n");
    sh_write0("         Use for performance-critical code.\n\n");

    sh_write0("    M0 specific optimization tips:\n");
    sh_write0("      - Use 'uint32_t' instead of 'int' (native 32-bit)\n");
    sh_write0("      - Avoid 64-bit types (software emulated, SLOW)\n");
    sh_write0("      - Avoid float/double (no FPU, software ~100x slower)\n");
    sh_write0("      - Shift instead of multiply/divide by powers of 2\n");
    sh_write0("      - Use __attribute__((aligned(4))) for arrays\n");
    sh_write0("      - Enable -flto (Link-Time Optimization) for final build\n");
    sh_write0("      - Use -ffunction-sections -fdata-sections + --gc-sections\n\n");

    /* 展示各种操作的代码大小 */
    sh_write0("    Code size examples (M0, -Os):\n");
    sh_write0("      uint32_t add(uint32_t a, uint32_t b) { return a+b; }\n");
    sh_write0("        → 2 bytes: adds r0, r0, r1; bx lr\n\n");
    sh_write0("      uint32_t div10(uint32_t x) { return x/10; }\n");
    sh_write0("        → ~10 bytes + __aeabi_uidiv call (~70 bytes)\n\n");
    sh_write0("      uint32_t div10_fast(uint32_t x) { return x>>3; }\n");
    sh_write0("        → 2 bytes: lsrs r0, r0, #3; bx lr\n\n");

    sh_write0(
        "    For detailed analysis, build with different -O flags\n"
        "    and compare the .disasm files:\n"
        "      cmake -DCMAKE_BUILD_TYPE=Release ..\n"
        "      cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 3: 低功耗与 WFI
 * -------------------------------------------------------------------------- */
static void demo_low_power(void)
{
    sh_write0("[3] Low-Power Design on Cortex-M0\n\n");

    sh_write0("    WFI (Wait For Interrupt):\n");
    sh_write0("      - CPU enters sleep mode until interrupt occurs\n");
    sh_write0("      - All peripherals continue running\n");
    sh_write0("      - Wake-up latency: ~16 cycles (M0)\n");
    sh_write0("      - Used in FreeRTOS idle task: for (;;) { __WFI(); }\n\n");

    sh_write0("    WFE (Wait For Event):\n");
    sh_write0("      - Sleep until event (interrupt or SEV)\n");
    sh_write0("      - Useful in multi-core systems (not applicable to M0)\n\n");

    sh_write0("    Sleep-on-Exit:\n");
    sh_write0("      - CPU sleeps after ISR returns (no unstacking)\n");
    sh_write0("      - Reduces power in interrupt-driven systems\n");
    sh_write0("      - Set SCR.SLEEPONEXIT = 1 (0xE000ED10 bit 1)\n\n");

    sh_write0("    FreeRTOS Tickless Idle:\n");
    sh_write0("      - Stops SysTick during idle periods\n");
    sh_write0("      - Wakes CPU only when task is ready\n");
    sh_write0("      - Saves significant power in battery applications\n");
    sh_write0("      - Configure: configUSE_TICKLESS_IDLE = 1\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 4: 从仿真到真实硬件
 * -------------------------------------------------------------------------- */
static void demo_real_hardware(void)
{
    sh_write0("[4] From Simulation to Real Hardware\n\n");

    sh_write0("    QEMU vs Real Silicon (nRF51822):\n\n");
    sh_write0("    +---------------------------+---------------------------+\n");
    sh_write0("    | QEMU                      | Real Hardware             |\n");
    sh_write0("    +---------------------------+---------------------------+\n");
    sh_write0("    | No SysTick interrupts     | SysTick works correctly   |\n");
    sh_write0("    | PendSV causes HardFault   | PendSV works correctly    |\n");
    sh_write0("    | UART TX hangs             | UART works correctly      |\n");
    sh_write0("    | No timing guarantees      | Deterministic timing      |\n");
    sh_write0("    | Free (no hardware needed) | Need dev board + debugger  |\n");
    sh_write0("    +---------------------------+---------------------------+\n\n");

    sh_write0("    Migrating to real hardware:\n");
    sh_write0("      1. Get a BBC micro:bit board (~$15)\n");
    sh_write0("      2. Flash via USB drag-and-drop or OpenOCD+SWD\n");
    sh_write0("      3. Use the SAME code! (startup.S, linker script, drivers)\n");
    sh_write0("      4. Changes needed: only UART baud rate tuning\n");
    sh_write0("      5. Debug with: OpenOCD + arm-none-eabi-gdb\n\n");

    sh_write0("    Flash programming:\n");
    sh_write0("      arm-none-eabi-objcopy -O ihex lesson_08.elf lesson_08.hex\n");
    sh_write0("      Copy lesson_08.hex to MICROBIT USB drive\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: 学习路线总结
 * -------------------------------------------------------------------------- */
static void demo_learning_summary(void)
{
    sh_write0("[5] Learning Journey Summary\n\n");

    sh_write0("    Lessons completed:\n");
    sh_write0("      1. Bare-Metal Hello World (semihosting, CMake, linker)\n");
    sh_write0("      2. ARMv6-M Assembly (60+ Thumb-1 instructions)\n");
    sh_write0("      3. C + Assembly Integration (AAPCS, inline asm)\n");
    sh_write0("      4. Exception Handling (NVIC, SysTick, PendSV, HardFault)\n");
    sh_write0("      5. Peripheral Drivers (UART, GPIO, Timer - nRF51)\n");
    sh_write0("      6. C Standard Library (newlib-nano, malloc, printf)\n");
    sh_write0("      7. FreeRTOS Kernel (tasks, queues, scheduler)\n");
    sh_write0("      8. Advanced Topics (GDB, optimization, production)\n");
    sh_write0("      9. Bootloader + Application (dual-firmware)\n\n");

    sh_write0("    Core skills gained:\n");
    sh_write0("      - ARM Cortex-M0 architecture (ARMv6-M)\n");
    sh_write0("      - GNU toolchain (GCC, LD, AS, OBJDUMP, GDB)\n");
    sh_write0("      - CMake build system for cross-compilation\n");
    sh_write0("      - QEMU system emulation for embedded\n");
    sh_write0("      - FreeRTOS real-time operating system\n");
    sh_write0("      - Embedded C best practices (volatile, memory-mapped I/O)\n");
    sh_write0("      - Debugging and optimization techniques\n\n");
}

/* --------------------------------------------------------------------------
 * 系统信息
 * -------------------------------------------------------------------------- */
static void print_system_info(void)
{
    /* 读取 CPUID 寄存器 */
    uint32_t cpuid;
    __asm__ volatile("ldr %0, [%1, #0]" : "=r"(cpuid) : "r"(0xE000ED00));

    sh_write0("=== System Information ===\n");
    sh_write0("  Build:    " BUILD_DATE "\n");
    sh_write0("  Git:      " GIT_HASH "\n");
    sh_write0("  CPUID:    ");
    sh_write_hex(cpuid);
    sh_write0("\n");
    sh_write0("  QEMU:     v11.0.0\n");
    sh_write0("  GCC:      10.3.1 20210824\n");
    sh_write0("  Optimize: ");
#ifdef __OPTIMIZE_SIZE__
    sh_write0("-Os (size)\n");
#elif defined(__OPTIMIZE__)
    sh_write0("-O2 (speed)\n");
#else
    sh_write0("-O0 (debug)\n");
#endif
    sh_write0("\n");
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */
int main(void)
{
    sh_write0("\n==============================================\n");
    sh_write0("  Lesson 8: Advanced Topics & Final Demo\n");
    sh_write0("  ARM Cortex-M0 Embedded Learning\n");
    sh_write0("==============================================\n\n");

    print_system_info();

    demo_gdb_debugging();
    demo_optimization();
    demo_low_power();
    demo_real_hardware();
    demo_learning_summary();

    sh_write0("==============================================\n");
    sh_write0("  All lessons complete!\n");
    sh_write0("  You now have the foundation for embedded\n");
    sh_write0("  development on ARM Cortex-M microcontrollers.\n");
    sh_write0("==============================================\n\n");

    /* SYS_EXIT */
    register int r0 __asm__("r0") = 0x18;
    register int *r1 __asm__("r1") = &(int){ 0 };
    __asm__ volatile("bkpt #0xAB" : "+r"(r0) : "r"(r1) : "memory");

    while (1)
    {
    }
    return 0;
}
