/*
 * =============================================================================
 * assert_impl.c -- 断言失败处理
 * =============================================================================
 */
#include "assert_impl.h"
#include "error_handler.h"
#include "semihosting.h"
#include <stdint.h>

void assert_failed(const char *file, int line, const char *expr)
{
    sh_write0("\n!!! ASSERTION FAILED !!!\n");
    sh_write0("  File: ");
    sh_write0(file);
    sh_write0("\n");
    sh_write0("  Line: ");
    /* line 是 int, 这里简化为 hex 输出 */
    sh_write_hex((uint32_t)line);
    sh_write0("\n");
    sh_write0("  Expr: ");
    sh_write0(expr);
    sh_write0("\n\n");

    /* 记录到故障日志 (可通过复位保留) */
    fault_log_write(ERR_FATAL_ASSERT, 0, (uint32_t)line);

    /* 记录错误 */
    error_record(ERR_FATAL_ASSERT);

    /* 死循环 — 等待看门狗复位或调试器介入
     * 在真实系统中, WDT 会在几秒后复位系统 */
    sh_write0("System halted. Waiting for WDT reset...\n");
    while (1)
    {
    }
}
