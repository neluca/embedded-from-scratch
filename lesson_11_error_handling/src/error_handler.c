/*
 * =============================================================================
 * error_handler.c -- 错误处理框架实现
 * =============================================================================
 */
#include "error_handler.h"
#include "semihosting.h"
#include "soft_crc.h"
#include <stddef.h>
#include <stdint.h>

/* 最近一次错误 (运行时可变) */
static error_code_t g_last_error = ERR_OK;

/* 错误计数 */
static uint32_t g_error_count = 0;

/* 故障日志 — 放在 .noinit section 中, 软复位后保留 */
/* 注意: 上电复位时值为随机, 需用 magic 验证有效性 */
__attribute__((section(".noinit"), aligned(4))) static fault_log_t g_fault_log;

void error_record(error_code_t code)
{
    g_last_error = code;
    g_error_count++;
}

error_code_t error_get_last(void)
{
    return g_last_error;
}

const char *error_to_string(error_code_t code)
{
    switch (code)
    {
    case ERR_OK:
        return "OK";
    case ERR_UNKNOWN:
        return "Unknown error";
    case ERR_NULL_POINTER:
        return "Null pointer";
    case ERR_INVALID_PARAM:
        return "Invalid parameter";
    case ERR_OUT_OF_RANGE:
        return "Out of range";
    case ERR_TIMEOUT:
        return "Timeout";
    case ERR_NOT_INIT:
        return "Not initialized";
    case ERR_BUSY:
        return "Resource busy";
    case ERR_NO_MEMORY:
        return "Out of memory";
    case ERR_BUFFER_OVERFLOW:
        return "Buffer overflow";
    case ERR_CHECKSUM:
        return "Checksum failure";
    case ERR_FATAL_STACK_OVERFLOW:
        return "FATAL: Stack overflow";
    case ERR_FATAL_WDT_TIMEOUT:
        return "FATAL: WDT timeout";
    case ERR_FATAL_HARDFAULT:
        return "FATAL: HardFault";
    case ERR_FATAL_ASSERT:
        return "FATAL: Assertion failed";
    case ERR_FATAL_MEMORY_CORRUPT:
        return "FATAL: Memory corruption";
    default:
        return "Unknown code";
    }
}

const fault_log_t *fault_log_get(void)
{
    /* 验证 magic */
    if (g_fault_log.magic != 0xDEADBEEF)
    {
        return NULL;
    }
    /* 验证 CRC */
    uint32_t crc = crc32(&g_fault_log, sizeof(fault_log_t) - 4);
    if (crc != g_fault_log.crc)
    {
        return NULL;
    }
    return &g_fault_log;
}

void fault_log_clear(void)
{
    g_fault_log.magic = 0;
    g_fault_log.error_code = 0;
    g_fault_log.fault_pc = 0;
    g_fault_log.line_number = 0;
    g_fault_log.timestamp = 0;
    g_fault_log.crc = 0;
}

void fault_log_write(error_code_t code, uint32_t pc, uint32_t line)
{
    g_fault_log.magic = 0xDEADBEEF;
    g_fault_log.error_code = (uint32_t)code;
    g_fault_log.fault_pc = pc;
    g_fault_log.line_number = line;
    g_fault_log.timestamp = 0; /* SysTick not available, placeholder */
    g_fault_log.crc = crc32(&g_fault_log, sizeof(fault_log_t) - 4);
}

void fault_log_print(void)
{
    const fault_log_t *log = fault_log_get();

    if (log == NULL)
    {
        sh_write0("No valid fault log (clean boot or corrupted).\n");
        return;
    }

    sh_write0("=== Fault Log (from previous reset) ===\n");
    sh_write0("  Error:  ");
    sh_write0(error_to_string((error_code_t)log->error_code));
    sh_write0(" (0x");
    sh_write_hex(log->error_code);
    sh_write0(")\n");
    sh_write0("  PC:     0x");
    sh_write_hex(log->fault_pc);
    sh_write0("\n");
    sh_write0("  Line:   ");
    sh_write_hex(log->line_number);
    sh_write0("\n");
    sh_write0("  CRC:    0x");
    sh_write_hex(log->crc);
    sh_write0(" (valid)\n");
    sh_write0("========================================\n");
}

uint32_t error_get_count(void)
{
    return g_error_count;
}

void error_reset_count(void)
{
    g_error_count = 0;
}
