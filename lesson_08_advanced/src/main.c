/*
 * =============================================================================
 * Lesson 8 -- ARM Cortex-M0 高级主题与生产部署
 * =============================================================================
 *
 * 本课在 QEMU 中演示嵌入式开发的四个核心高级主题:
 *
 *   Module 1: GDB 远程调试实战
 *     - 断点 (硬件/软件/条件)
 *     - 监视点 (数据变化检测)
 *     - 栈回溯 (调用链分析)
 *     - 寄存器/内存检查
 *     - Bug 定位练习 (找到故意植入的 off-by-one bug)
 *
 *   Module 2: 编译器优化分析
 *     - 除法优化 (常数逆元 vs 通用除法)
 *     - volatile 的性能代价
 *     - 结构体打包与对齐
 *     - 内联 vs 函数调用
 *     - 定点数 vs 浮点数 (M0 无 FPU)
 *
 *   Module 3: 低功耗模式
 *     - WFI/WFE 指令 (QEMU 中立即返回, 但代码对真实硬件正确)
 *     - Sleep-on-Exit 模式
 *     - nRF51 功耗数据 (2.4mA 运行 → 2μA 睡眠)
 *
 *   Module 4: 生产部署
 *     - 固件版本嵌入
 *     - 内存使用分析
 *     - 栈水印技术
 *     - ELF/BIN/HEX 格式说明
 *     - 生产检查清单
 *
 * 运行方式:
 *   正常: cmake --build build --target run_lesson_08_advanced
 *   调试: cmake --build build --target debug_lesson_08_advanced
 * =============================================================================
 */

#include "semihosting.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 演示模块声明
 * -------------------------------------------------------------------------- */
void gdb_demo_entry(void);
void optimization_demo_entry(void);
void low_power_demo_entry(void);
void production_demo_entry(void);

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */
int main(void)
{
    sh_write0("\n");
    sh_write0("========================================================\n");
    sh_write0("  Lesson 8: Advanced Topics & Production Deployment\n");
    sh_write0("  ARM Cortex-M0 Embedded Learning\n");
    sh_write0("========================================================\n\n");

    sh_write0("  This lesson covers 4 modules:\n");
    sh_write0("    1. GDB Remote Debugging Practical\n");
    sh_write0("    2. Compiler Optimization Deep Dive\n");
    sh_write0("    3. Low-Power Modes on Cortex-M0\n");
    sh_write0("    4. From Development to Production\n\n");

    sh_write0("  Re-run with GDB: -s -S flags to debug interactively.\n\n");

    /* Module 1: GDB 调试实战 */
    gdb_demo_entry();

    /* Module 2: 编译器优化 */
    optimization_demo_entry();

    /* Module 3: 低功耗模式 */
    low_power_demo_entry();

    /* Module 4: 生产部署 */
    production_demo_entry();

    /* 总结 */
    sh_write0("========================================================\n");
    sh_write0("  All lessons complete!\n");
    sh_write0("========================================================\n\n");

    sh_write0("  Key takeaways:\n");
    sh_write0("    - GDB + QEMU = zero-cost debugging (no hardware needed)\n");
    sh_write0("    - Optimization: understand what the compiler does for you\n");
    sh_write0("    - Low power: WFI saves 1000x current on real hardware\n");
    sh_write0("    - Production: versioning, sizing, and checklists matter\n");
    sh_write0("    - ALL skills transfer directly to M3/M4/M7/M23/M33\n\n");

    sh_write0("  Next steps:\n");
    sh_write0("    - Run again with GDB: cmake --build build --target debug_lesson_08_advanced\n");
    sh_write0("    - Build with Release flags: -DCMAKE_BUILD_TYPE=Release\n");
    sh_write0("    - Compare .disasm files between Debug and Release\n");
    sh_write0("    - Try on real BBC micro:bit hardware (~$15)\n\n");

    /* 完成. 所有输出已通过 semihosting 发送. 退出 QEMU. */
    sh_exit(0);

    /* sh_exit 应退出 QEMU; 如果未成功 (某些 QEMU 版本), 进入死循环. */
    while (1) { }
    return 0;
}
