/*
 * =============================================================================
 * production_demo.c -- 从开发到生产部署
 * =============================================================================
 *
 * 嵌入式固件从"能跑"到"可部署"的过程涉及:
 *   1. 版本信息嵌入 (构建时间、Git hash)
 *   2. 内存使用分析 (.map 文件、链接符号)
 *   3. 栈使用量测量 (stack watermark)
 *   4. 固件格式转换 (ELF → BIN → HEX)
 *   5. 完整性验证 (CRC, 签名)
 *
 * 本模块展示这些生产就绪技术.
 * =============================================================================
 */

#include "semihosting.h"
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 链接脚本中定义的符号 (用于内存使用报告)
 * -------------------------------------------------------------------------- */
extern uint8_t __data_start;
extern uint8_t __data_end;
extern uint8_t __data_load_start;
extern uint8_t __bss_start;
extern uint8_t __bss_end;
extern uint8_t __heap_start;
extern uint8_t __stack_top;

/* --------------------------------------------------------------------------
 * 版本信息 (由 CMake 在构建时注入)
 * -------------------------------------------------------------------------- */
#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif
#ifndef BUILD_DATE
#define BUILD_DATE __DATE__ " " __TIME__
#endif
#ifndef FW_VERSION
#define FW_VERSION "1.0.0"
#endif

/* --------------------------------------------------------------------------
 * 演示 1: 固件版本信息
 * -------------------------------------------------------------------------- */
static void demo_firmware_version(void)
{
    sh_write0("  [4.1] Firmware Version Embedding\n\n");

    sh_write0("    Version:     " FW_VERSION "\n");
    sh_write0("    Built:       " BUILD_DATE "\n");
    sh_write0("    Git commit:  " GIT_HASH "\n\n");

    sh_write0("    How to inject via CMake:\n");
    sh_write0("      execute_process(\n");
    sh_write0("        COMMAND git rev-parse --short HEAD\n");
    sh_write0("        OUTPUT_VARIABLE GIT_HASH\n");
    sh_write0("        OUTPUT_STRIP_TRAILING_WHITESPACE)\n");
    sh_write0("      target_compile_definitions(... PRIVATE\n");
    sh_write0("        -DGIT_HASH=\"${GIT_HASH}\")\n\n");

    sh_write0("    In code, use #ifndef to provide fallbacks so the code\n");
    sh_write0("    compiles even outside a git repository.\n\n");

    sh_write0("    Semver (MAJOR.MINOR.PATCH):\n");
    sh_write0("      - MAJOR: incompatible API changes\n");
    sh_write0("      - MINOR: backwards-compatible new features\n");
    sh_write0("      - PATCH: backwards-compatible bug fixes\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 2: 内存使用分析
 * -------------------------------------------------------------------------- */
static void demo_memory_usage(void)
{
    sh_write0("  [4.2] Memory Usage Report\n\n");

    /* 使用链接脚本中定义的符号计算各段大小 */
    uint32_t data_size = (uint32_t)(&__data_end - &__data_start);
    uint32_t bss_size  = (uint32_t)(&__bss_end - &__bss_start);
    uint32_t ram_used  = data_size + bss_size;

    /* Flash 使用量需要从 .map 文件读取, 这里做粗略估算 */
    /* __data_load_start = Flash 中 .data 初始值的起始地址 */
    (void)&__data_load_start; /* 符号存在性检查, 实际 Flash 用量从 .map 获取 */

    sh_write0("    RAM Usage (from linker symbols):\n");
    sh_write0("    ┌──────────────┬──────────┬─────────────────────────┐\n");
    sh_write0("    │ Section      │ Size     │ Location                │\n");
    sh_write0("    ├──────────────┼──────────┼─────────────────────────┤\n");
    sh_write0("    │ .data (init) │ ");
    sh_write_dec(data_size);
    sh_write0(" bytes │ RAM @ ");
    sh_write_hex((uint32_t)&__data_start);
    sh_write0("       │\n");
    sh_write0("    │ .bss  (zero) │ ");
    sh_write_dec(bss_size);
    sh_write0(" bytes │ RAM @ ");
    sh_write_hex((uint32_t)&__bss_start);
    sh_write0("       │\n");
    sh_write0("    │ Total RAM    │ ");
    sh_write_dec(ram_used);
    sh_write0(" bytes │ out of 16384 (16KB)      │\n");
    sh_write0("    │ Available    │ ");
    sh_write_dec(16384 - ram_used);
    sh_write0(" bytes │ (for stack + heap)       │\n");
    sh_write0("    └──────────────┴──────────┴─────────────────────────┘\n\n");

    sh_write0("    Flash Usage (use arm-none-eabi-size for exact values):\n");
    sh_write0("      $ arm-none-eabi-size lesson_08_advanced.elf\n");
    sh_write0("        text    data   bss    dec    hex   filename\n");
    sh_write0("        XXXX      XX    XX   XXXX   XXXX  lesson_08_advanced.elf\n\n");

    sh_write0("    .map file analysis:\n");
    sh_write0("      - Total Flash = text + data (initial values in Flash)\n");
    sh_write0("      - Total RAM   = data + bss (runtime RAM)\n");
    sh_write0("      - Check for .map in build directory\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 3: 栈水印 -- 测量实际栈使用量
 * --------------------------------------------------------------------------
 *
 * 原理:
 *   1. 启动时, 用已知 pattern 填充栈空间
 *   2. 运行程序 (执行最深调用路径)
 *   3. 检查 pattern 从栈顶开始被覆写了多少
 *   4. 覆写量 = 最大栈使用量
 *
 * 这是最可靠的栈使用量测量方法, 优于静态分析.
 * -------------------------------------------------------------------------- */

/* 栈水印 pattern */
#define STACK_WATERMARK 0xEEEEEEEE

static void fill_stack_watermark(void)
{
    /* 获取当前栈指针 -- 它以上的空间是未使用的 */
    uint32_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));

    /* 从 sp 到 stack_top 之间的区域填充 watermark
     * 注意: 不能覆盖当前栈帧! 从 sp 上方开始填充
     * 使用 < sp 因为栈向下增长 */
    uint32_t *start = (uint32_t *)((uint32_t)&__heap_start);
    uint32_t *end   = (uint32_t *)sp;

    /* 只在 start < end 时填充
     * 使用递减指针而非索引循环, 避免 GCC 优化为 memset 调用
     * (我们用的是 -nostdlib, 没有 memset 可用!) */
    if (start < end)
    {
        uint32_t count = end - start;
        uint32_t *p = start;
        /* 使用 do-while + 递减计数 — GCC 不会将其识别为 memset 模式 */
        do {
            *p++ = STACK_WATERMARK;
        } while (--count > 0);
    }
}

static uint32_t measure_stack_usage(void)
{
    volatile uint32_t *p = (volatile uint32_t *)((uint32_t)&__heap_start);
    uint32_t *end = (uint32_t *)((uint32_t)&__stack_top);

    uint32_t used = 0;
    while ((uint32_t *)p < end)
    {
        if (*p != STACK_WATERMARK)
        {
            used++;
        }
        p++;
    }
    return used * 4; /* 转换为字节 */
}

static void demo_stack_watermark(void)
{
    sh_write0("  [4.3] Stack Watermark — Measuring Actual Stack Usage\n\n");

    sh_write0("    Principle:\n");
    sh_write0("      1. Fill free stack with 0xEEEEEEEE (watermark)\n");
    sh_write0("      2. Run the deepest call path in your program\n");
    sh_write0("      3. Count how many words are NOT 0xEEEEEEEE\n");
    sh_write0("      4. That's your peak stack usage.\n\n");

    sh_write0("    RAM memory layout:\n");
    sh_write0("      ┌──────────────┐ __stack_top = 0x20004000\n");
    sh_write0("      │   (unused)   │\n");
    sh_write0("      │  [watermark] │ ← SP 当前在这个区域\n");
    sh_write0("      │  [in use]    │\n");
    sh_write0("      ├──────────────┤ __heap_start / end of .bss\n");
    sh_write0("      │  .bss + .data│\n");
    sh_write0("      └──────────────┘ 0x20000000\n\n");

    /* 填充 watermark */
    fill_stack_watermark();
    sh_write0("    Stack watermark filled with 0xEEEEEEEE.\n");

    /* 模拟一些栈使用 (故意做几次函数调用 + 局部变量)
     * 不用 = {0} 初始化, 这会触发 GCC 生成 memset 调用 (nostdlib 没有 memset!) */
    uint32_t dummy[32];
    for (int i = 0; i < 32; i++)
    {
        dummy[i] = i * 7; /* 直接赋值, 被覆写的 watermark 会在 measure 中检测到 */
    }

    uint32_t stack_used = measure_stack_usage();
    sh_write0("    Measured stack usage: ");
    sh_write_dec(stack_used);
    sh_write0(" bytes (includes dummy[] array)\n");

    /* 阻止编译器优化掉 dummy */
    volatile uint32_t check = 0;
    for (int i = 0; i < 32; i++) check += dummy[i];
    (void)check;

    sh_write0("    Tip: add +256 bytes margin for ISR nesting + library calls.\n");
    sh_write0("    Then ensure TOTAL (data + bss + stack_max + margin) < RAM_SIZE.\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 4: 固件输出格式 (ELF vs BIN vs HEX)
 * -------------------------------------------------------------------------- */
static void demo_output_formats(void)
{
    sh_write0("  [4.4] Firmware Output Formats\n\n");

    sh_write0("    After building, the lesson produces:\n\n");
    sh_write0("    ┌──────────┬──────────────────────────────────────────┐\n");
    sh_write0("    │ Format   │ Description                              │\n");
    sh_write0("    ├──────────┼──────────────────────────────────────────┤\n");
    sh_write0("    │ .elf     │ Full executable (debug info, symbols)    │\n");
    sh_write0("    │          │ Used by: QEMU, GDB, linker                  │\n");
    sh_write0("    │ .bin     │ Raw binary image (no metadata)           │\n");
    sh_write0("    │          │ Used by: flash programmers, QEMU -kernel │\n");
    sh_write0("    │          │ Create: objcopy -O binary elf.elf bin.bin │\n");
    sh_write0("    │ .hex     │ Intel HEX format (text, with addresses)  │\n");
    sh_write0("    │          │ Used by: bootloaders, real hardware flash │\n");
    sh_write0("    │          │ Create: objcopy -O ihex elf.elf hex.hex  │\n");
    sh_write0("    │ .disasm  │ Annotated disassembly (for analysis)     │\n");
    sh_write0("    │          │ Create: objdump -d -S elf.elf > out.dis  │\n");
    sh_write0("    │ .map     │ Memory map (functions + data layout)     │\n");
    sh_write0("    │          │ Create: -Wl,-Map=output.map linker flag  │\n");
    sh_write0("    └──────────┴──────────────────────────────────────────┘\n\n");

    sh_write0("    Conversion commands:\n");
    sh_write0("      arm-none-eabi-objcopy -O binary  in.elf  out.bin\n");
    sh_write0("      arm-none-eabi-objcopy -O ihex    in.elf  out.hex\n\n");

    sh_write0("    Binary inspection:\n");
    sh_write0("      arm-none-eabi-nm     in.elf    (symbol list)\n");
    sh_write0("      arm-none-eabi-size   in.elf    (section sizes)\n");
    sh_write0("      arm-none-eabi-objdump -h in.elf (section headers)\n");
    sh_write0("      arm-none-eabi-readelf -S in.elf (detailed sections)\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: 生产部署检查清单
 * -------------------------------------------------------------------------- */
static void demo_production_checklist(void)
{
    sh_write0("  [4.5] Production Deployment Checklist\n\n");

    sh_write0("    Before shipping firmware to production:\n\n");
    sh_write0("    ┌─────────────────────────────────────────────────────────┐\n");
    sh_write0("    │ [ ] Check .map file: no unexpected bloat               │\n");
    sh_write0("    │ [ ] Verify stack usage with watermark technique        │\n");
    sh_write0("    │ [ ] Enable WDT with appropriate timeout                │\n");
    sh_write0("    │ [ ] Implement fault logging (.noinit for crash dumps)  │\n");
    sh_write0("    │ [ ] Add CRC/checksum for firmware integrity            │\n");
    sh_write0("    │ [ ] Embed build ID + version in firmware image         │\n");
    sh_write0("    │ [ ] Test on all hardware variants (not just QEMU!)     │\n");
    sh_write0("    │ [ ] Compile with -Os -flto for minimal size            │\n");
    sh_write0("    │ [ ] Strip debug symbols from production image          │\n");
    sh_write0("    │ [ ] Verify boot time < required limit                  │\n");
    sh_write0("    │ [ ] Test power consumption in all operating modes      │\n");
    sh_write0("    │ [ ] Document known issues and hardware dependencies    │\n");
    sh_write0("    └─────────────────────────────────────────────────────────┘\n\n");
}

/* --------------------------------------------------------------------------
 * 演示 6: CPUID 与系统信息
 * -------------------------------------------------------------------------- */
static void demo_system_info(void)
{
    sh_write0("  [4.6] System Identification Registers\n\n");

    /* 读取 CPUID (CPUID Base Register @ 0xE000ED00) */
    uint32_t cpuid;
    __asm__ volatile("ldr %0, [%1, #0]" : "=r"(cpuid) : "r"(0xE000ED00));

    sh_write0("    CPUID Base Register: ");
    sh_write_hex(cpuid);
    sh_write0("\n");

    uint32_t implementer = (cpuid >> 24) & 0xFF;
    uint32_t variant     = (cpuid >> 20) & 0x0F;
    uint32_t constant    = (cpuid >> 16) & 0x0F;
    uint32_t partno      = (cpuid >> 4)  & 0xFFF;
    uint32_t revision    = cpuid & 0x0F;

    sh_write0("      Implementer: 0x");
    sh_write_hex(implementer);
    sh_write0(" (ARM Ltd.)\n");
    sh_write0("      Variant:     ");
    sh_write_dec(variant);
    sh_write0("\n");
    sh_write0("      Constant:    ");
    sh_write_dec(constant);
    sh_write0("\n");
    sh_write0("      Part Number: 0x");
    sh_write_hex(partno);
    sh_write0(" (Cortex-M0 = 0xC20)\n");
    sh_write0("      Revision:    r");
    sh_write_dec(revision);
    sh_write0("p?\n\n");

    sh_write0("    Compiler:       " __VERSION__ "\n");
    sh_write0("    Build type:     ");
#if defined(__OPTIMIZE_SIZE__)
    sh_write0("-Os (optimize for size)\n");
#elif defined(__OPTIMIZE__)
    sh_write0("-O2 (optimize for speed)\n");
#else
    sh_write0("-O0 (no optimization / debug)\n");
#endif
    sh_write0("\n");
}

/* ==========================================================================
 * 入口
 * ========================================================================== */
void production_demo_entry(void)
{
    sh_write0("\n");
    sh_write0("========================================================\n");
    sh_write0("  [Module 4] From Development to Production\n");
    sh_write0("========================================================\n\n");

    demo_firmware_version();
    demo_system_info();
    demo_memory_usage();
    demo_output_formats();
    demo_stack_watermark();
    demo_production_checklist();

    sh_write0("  Production demo complete.\n\n");
}
