/*
 * =============================================================================
 * error_handler.h -- 统一错误处理框架
 * =============================================================================
 *
 * 设计原则:
 *   1. 使用错误码而非魔法数字
 *   2. 每个模块定义自己的错误码范围
 *   3. 统一的错误记录和报告入口
 *   4. 支持在复位后保留错误信息 (noinit section)
 *
 * 错误码范围分配:
 *   0x0000:        成功 (ERR_OK)
 *   0x0001-0x0FFF: 通用错误
 *   0x1000-0x1FFF: WDT 模块
 *   0x2000-0x2FFF: UART 模块
 *   0x3000-0x3FFF: 存储模块
 *   0xF000-0xFFFF: 严重/致命错误
 */
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdint.h>

/* 错误码枚举 */
typedef enum
{
    /* 成功 */
    ERR_OK = 0x0000,

    /* 通用错误 (0x0001-0x0FFF) */
    ERR_UNKNOWN = 0x0001, /* 未知错误 */
    ERR_NULL_POINTER = 0x0002, /* 空指针 */
    ERR_INVALID_PARAM = 0x0003, /* 无效参数 */
    ERR_OUT_OF_RANGE = 0x0004, /* 值超出范围 */
    ERR_TIMEOUT = 0x0005, /* 操作超时 */
    ERR_NOT_INIT = 0x0006, /* 模块未初始化 */
    ERR_BUSY = 0x0007, /* 资源繁忙 */
    ERR_NO_MEMORY = 0x0008, /* 内存不足 */
    ERR_BUFFER_OVERFLOW = 0x0009, /* 缓冲区溢出 */
    ERR_CHECKSUM = 0x000A, /* 校验失败 */

    /* 严重错误 (0xF000-0xFFFF) */
    ERR_FATAL_STACK_OVERFLOW = 0xF001, /* 栈溢出 */
    ERR_FATAL_WDT_TIMEOUT = 0xF002, /* 看门狗复位 */
    ERR_FATAL_HARDFAULT = 0xF003, /* HardFault */
    ERR_FATAL_ASSERT = 0xF004, /* 断言失败 */
    ERR_FATAL_MEMORY_CORRUPT = 0xF005, /* 内存损坏 */
} error_code_t;

/* =========================================================================
 * 故障日志结构 (存储在 noinit RAM 中, 复位后保留)
 * =========================================================================
 * 配合 Lesson 10 的 RESETREAS 检测, 可以在复位后读取上次故障信息.
 */
typedef struct
{
    uint32_t magic;          /* 魔数: 0xDEADBEEF 表示日志有效 */
    uint32_t error_code;     /* 上次故障的错误码 */
    uint32_t fault_pc;       /* 故障时的 PC (来自 HardFault 栈帧) */
    uint32_t line_number;    /* 故障时的行号 (如果是断言失败) */
    uint32_t timestamp;      /* 故障时间戳 (SysTick 计数) */
    uint32_t crc;            /* 本结构的 CRC32 校验 */
} fault_log_t;

/* =========================================================================
 * API 函数声明
 * ========================================================================= */

/* 记录错误 (不触发处理) */
void error_record(error_code_t code);

/* 获取最近的错误码 */
error_code_t error_get_last(void);

/* 将错误码转为可读字符串 */
const char *error_to_string(error_code_t code);

/* 获取故障日志指针 (位于 noinit RAM, 复位后保留) */
const fault_log_t *fault_log_get(void);

/* 清除故障日志 */
void fault_log_clear(void);

/* 写入故障日志 */
void fault_log_write(error_code_t code, uint32_t pc, uint32_t line);

/* 打印故障日志到控制台 */
void fault_log_print(void);

/* 获取错误计数 */
uint32_t error_get_count(void);

/* 重置错误计数 */
void error_reset_count(void);

#endif /* ERROR_HANDLER_H */
