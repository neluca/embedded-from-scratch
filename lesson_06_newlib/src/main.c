/*
 * =============================================================================
 * Lesson 6 主程序 -- C 标准库集成 (newlib-nano)
 * =============================================================================
 *
 * 学习目标:
 * 1. 理解 C 运行时系统调用 (syscalls) 的作用
 * 2. 掌握堆管理 (_sbrk) 和 malloc 的使用
 * 3. 使用 printf/scanf 等标准 I/O (通过 semihosting 后端)
 * 4. 使用数学函数 (M0 软件浮点, 无 FPU)
 * 5. 了解 newlib-nano 的配置和限制
 *
 * newlib-nano 是 ARM GCC 工具链自带的精简版 C 标准库:
 *   - 使用 -specs=nano.specs 链接
 *   - 代码体积小, 适合微控制器
 *   - 默认禁用 printf 浮点格式化 (%f) 以节省空间
 *   - 可通过 -u _printf_float 启用浮点格式化 (增加 ~12KB)
 *   - 系统调用接口: _sbrk, _write, _read, _exit 等
 *
 * 使用 newlib-nano 的内存开销:
 *   - printf 基本支持: ~3KB
 *   - malloc 基本实现: ~1KB
 *   - 数学函数 (sin/cos/sqrt): ~2KB (软件浮点)
 *   总计: ~6KB (在 16KB RAM 的 microbit 上可以接受)
 * =============================================================================
 */

#include "semihosting.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* --------------------------------------------------------------------------
 * 演示 1: printf 标准输出 (通过 _write syscall → semihosting)
 * -------------------------------------------------------------------------- */
static void demo_printf(void)
{
    sh_write0("[1] printf via semihosting-backed _write\n\n");

    /* printf 内部调用 _write(1, buf, len)
     * _write 通过 semihosting SYS_WRITEC 逐字符输出 */
    printf("    Hello from printf!\n");
    printf("    Integer:  %d (expect 42)\n", 42);
    printf("    Hex:      0x%08X (expect 0xDEADBEEF)\n", 0xDEADBEEF);
    printf("    String:   '%s'\n", "Cortex-M0");
    printf("    Char:     '%c' (expect 'A')\n", 'A');
    printf("    Negative: %d (expect -123)\n", -123);
    printf("    Pointer:  %p (expect 0x20000000-ish)\n", (void *)0x20000000);
    printf("\n");

    /* snprintf: 格式化到缓冲区 (不调用 _write) */
    char buf[64];
    snprintf(buf, sizeof(buf), "    snprintf: answer=%d hex=0x%X", 42, 0x42);
    printf("%s\n\n", buf);
}

/* --------------------------------------------------------------------------
 * 演示 2: malloc/free 堆管理 (通过 _sbrk syscall)
 * -------------------------------------------------------------------------- */
static void demo_malloc(void)
{
    sh_write0("[2] malloc / free via _sbrk heap\n\n");

    /* _sbrk 维护一个简单的线性堆
     * malloc 内部在堆上管理空闲块 */

    printf("    Allocating memory...\n");

    /* 分配一些内存块 */
    int *p1 = (int *)malloc(sizeof(int));
    int *p2 = (int *)malloc(sizeof(int));
    char *p3 = (char *)malloc(32);

    if (p1 && p2 && p3)
    {
        *p1 = 42;
        *p2 = 100;
        strcpy(p3, "Hello Heap!");

        printf("    p1 = %p, *p1 = %d\n", (void *)p1, *p1);
        printf("    p2 = %p, *p2 = %d\n", (void *)p2, *p2);
        printf("    p3 = %p,  str = '%s'\n", (void *)p3, p3);

        /* 注意 p1/p2/p3 的地址关系:
         *   malloc 在堆上分配, 地址应该递增
         *   (取决于 malloc 的分配策略) */

        printf("\n    Freeing p1 and p3...\n");
        free(p1);
        free(p3);

        /* 重新分配 p4 (可能重用已释放的空间) */
        int *p4 = (int *)malloc(sizeof(int));
        if (p4)
        {
            *p4 = 999;
            printf("    p4 = %p, *p4 = %d\n", (void *)p4, *p4);
            printf("    (p4 may reuse freed p1 space)\n");
            free(p4);
        }
        free(p2);
    }
    else
    {
        printf("    ERROR: malloc failed! (out of heap?)\n");
    }

    /* 边界测试: 尝试分配过多内存 */
    printf("\n    Testing heap exhaustion:\n");
    uint32_t huge_size = 16 * 1024; /* microbit only has 16KB total RAM! */
    void *huge = malloc(huge_size);
    if (huge == NULL)
    {
        printf("    malloc(%lu) = NULL (expected, heap too small)\n",
               (unsigned long)huge_size);
    }
    else
    {
        printf("    malloc(%lu) succeeded (unexpected!)\n",
               (unsigned long)huge_size);
        free(huge);
    }

    printf("\n");
}

/* --------------------------------------------------------------------------
 * 演示 3: 数学函数 (软件浮点, M0 无 FPU)
 * -------------------------------------------------------------------------- */
static void demo_math(void)
{
    sh_write0("[3] Math functions (software float on M0)\n\n");

    /* 重要: Cortex-M0 没有硬件 FPU!
     * 所有浮点运算都是软件模拟的 (通过 libgcc 的浮点模拟库).
     * 软件浮点比硬件浮点慢 100-1000 倍.
     *
     * 在嵌入式系统中:
     *   - 尽量避免浮点运算
     *   - 使用定点数代替 (如 Q15.16 格式)
     *   - 如果必须用浮点, 考虑使用 -mfloat-abi=soft
     */

    printf("    M0 has NO FPU (Floating Point Unit).\n");
    printf("    All float ops are software emulated.\n\n");

    /* 三角函数 */
    double pi = 3.141592653589793;
    printf("    sin(pi/6) = %f (expect 0.5)\n", sin(pi / 6.0));
    printf("    cos(pi/3) = %f (expect 0.5)\n", cos(pi / 3.0));

    /* 平方根 */
    printf("    sqrt(2.0) = %f (expect ~1.4142)\n", sqrt(2.0));
    printf("    sqrt(9.0) = %f (expect 3.0)\n", sqrt(9.0));

    /* 幂和对数 */
    printf("    pow(2, 10) = %f (expect 1024)\n", pow(2.0, 10.0));
    printf("    log(2.718) = %f (expect ~1.0)\n", log(2.718281828));

    /* 浮点运算的性能影响:
     *   一个 sin() 调用大约需要 500-2000 个周期 (取决于实现)
     *   在 16MHz 的 M0 上, 这意味着 30-125 微秒
     *   对于实时控制, 这个延迟可能不可接受
     */

    printf("\n    [Fixed-point alternative for embedded]\n");

    /* 定点数示例: Q15.16 格式 (15 位整数, 16 位小数) */
    int32_t fixed_val = (int32_t)(2.5 * 65536.0); /* 2.5 在 Q15.16 中 */
    printf("    2.5 in Q15.16 fixed-point = 0x%08lX (%ld)\n",
           (unsigned long)fixed_val, (long)fixed_val);
    printf("    Fixed-point is ~100x faster than software float on M0.\n");

    printf("\n");
}

/* Forward declaration for qsort/bsearch comparator */
static int compare_ints(const void *a, const void *b);

/* --------------------------------------------------------------------------
 * 演示 4: 字符串和内存操作 (stdlib.h, string.h)
 * -------------------------------------------------------------------------- */
static void demo_string(void)
{
    sh_write0("[4] String & memory utilities\n\n");

    /* strtol / strtoul */
    const char *num_str = "12345";
    long val = strtol(num_str, NULL, 10);
    printf("    strtol('%s') = %ld\n", num_str, val);

    const char *hex_str = "0xDEAD";
    long hex_val = strtol(hex_str, NULL, 16);
    printf("    strtol('%s', 16) = 0x%lX\n", hex_str, hex_val);

    /* qsort (compare_ints is defined at file scope below) */
    int arr[] = { 5, 2, 8, 1, 9, 3 };
    qsort(arr, 6, sizeof(int), compare_ints);

    printf("    qsort([5,2,8,1,9,3]) = [%d,%d,%d,%d,%d,%d]\n",
           arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);

    /* bsearch */
    int key = 5;
    int *found = (int *)bsearch(&key, arr, 6, sizeof(int), compare_ints);
    printf("    bsearch(5) = %s\n",
           found ? "found" : "not found");

    /* memcpy/memset/memcmp (通常编译器内建, 不调用函数) */
    printf("\n");
}

/* --------------------------------------------------------------------------
 * 演示 5: newlib-nano 配置说明
 * -------------------------------------------------------------------------- */
static void demo_newlib_info(void)
{
    sh_write0("[5] newlib-nano Configuration\n\n");

    printf(
        "    Currently using: newlib-nano (included with ARM GCC)\n"
        "\n"
        "    newlib-nano features:\n"
        "      - Included with ARM GCC toolchain (no extra build needed)\n"
        "      - Link with: -specs=nano.specs\n"
        "      - Mature and well-tested on Cortex-M\n"
        "      - System call interface: _sbrk, _write, _read, _exit, etc.\n"
        "      - Default: printf %%f disabled (saves ~12KB)\n"
        "      - Enable %%f: add -u _printf_float to linker flags\n"
        "\n"
        "    Controlling newlib-nano features via linker:\n"
        "      -u _printf_float    Enable %%f formatting (+~12KB flash)\n"
        "      -u _scanf_float     Enable %%f in scanf (+~8KB flash)\n"
        "      --specs=nano.specs  Use newlib-nano (smaller)\n"
        "      --specs=nosys.specs Use full newlib (larger, more features)\n"
        "\n"
        "    For most embedded projects, newlib-nano is the best choice.\n"
        "    It provides standard C with minimal overhead.\n"
        "\n");
}

/* --------------------------------------------------------------------------
 * main -- 主程序入口
 * -------------------------------------------------------------------------- */
int main(void)
{
    sh_write0("\n=== Lesson 6: C Standard Library Integration ===\n");
    sh_write0("Library: newlib-nano (via -specs=nano.specs)\n\n");

    demo_printf();
    demo_malloc();
    demo_math();
    demo_string();
    demo_newlib_info();

    sh_write0("=== Lesson 6 complete! ===\n\n");

    /* exit(0) → _exit(0) → SYS_EXIT semihosting */
    exit(0);

    while (1)
    {
    }
    return 0;
}

/* compare_ints 放在文件作用域 (C99 不支持嵌套函数) */
static int compare_ints(const void *a, const void *b)
{
    return (*(const int *)a - *(const int *)b);
}
