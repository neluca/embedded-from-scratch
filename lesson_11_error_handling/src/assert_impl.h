/*
 * =============================================================================
 * assert_impl.h -- 嵌入式轻量级断言系统
 * =============================================================================
 *
 * 标准 C 的 assert() 依赖 fprintf(stderr, ...) 和 abort().
 * 在裸机系统中, 我们需要自己实现.
 *
 * 本模块提供:
 *   - ASSERT(expr):            条件为假时触发 (Release 可禁用)
 *   - ASSERT_ALWAYS(expr):     条件为假时触发 (永远启用)
 *   - STATIC_ASSERT(expr, msg): 编译期断言 (不占代码空间)
 *
 * 断言的触发行为:
 *   1. 通过 semihosting 输出文件名 + 行号 + 条件
 *   2. 进入死循环 (等待看门狗复位或调试器介入)
 *
 * 与标准 assert 的区别:
 *   - 不调用 abort() — 裸机系统无法"退出"
 *   - 不依赖标准库 — 使用 semihosting 或 UART 输出
 *   - 可选记录到非易失存储 (结合 Lesson 10 的故障诊断)
 */
#ifndef ASSERT_IMPL_H
#define ASSERT_IMPL_H

/* --------------------------------------------------------------------------
 * 编译期静态断言 (不占任何代码/RAM 空间!)
 * --------------------------------------------------------------------------
 * 技巧: 使用负数数组大小触发编译错误
 * C11 的 _Static_assert 也可用, 但 typedef 方式兼容性更好
 */
#define STATIC_ASSERT(expr, msg) \
    typedef char static_assert_##msg[(expr) ? 1 : -1] __attribute__((unused))

/* --------------------------------------------------------------------------
 * 调试输出函数 (由 main.c 或 semihosting 提供)
 * -------------------------------------------------------------------------- */
void assert_failed(const char *file, int line, const char *expr);

/* --------------------------------------------------------------------------
 * 运行时断言宏
 * --------------------------------------------------------------------------
 * ASSERT: Debug 构建时检查; 定义 NDEBUG 后变为空操作 (与标准 assert 一致)
 * ASSERT_ALWAYS: 无论 Debug/Release 都会检查 (用于关键安全条件)
 */

#ifdef NDEBUG
#define ASSERT(expr) ((void)0)
#else
#define ASSERT(expr)                                  \
    do                                                \
    {                                                 \
        if (!(expr))                                  \
        {                                             \
            assert_failed(__FILE__, __LINE__, #expr); \
        }                                             \
    } while (0)
#endif

#define ASSERT_ALWAYS(expr)                           \
    do                                                \
    {                                                 \
        if (!(expr))                                  \
        {                                             \
            assert_failed(__FILE__, __LINE__, #expr); \
        }                                             \
    } while (0)

/* 便捷宏: 空指针检查 */
#define ASSERT_NOT_NULL(ptr) ASSERT_ALWAYS((ptr) != NULL)

/* 便捷宏: 范围检查 */
#define ASSERT_IN_RANGE(val, min, max) \
    ASSERT_ALWAYS((val) >= (min) && (val) <= (max))

#endif /* ASSERT_IMPL_H */
