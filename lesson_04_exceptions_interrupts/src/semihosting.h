/*
 * =============================================================================
 * semihosting.h -- Semihosting 调试辅助函数声明
 * =============================================================================
 */
#ifndef SEMIHOSTING_H
#define SEMIHOSTING_H

#include <stdint.h>

/* Semihosting 操作码 */
#define SYS_OPEN   0x01
#define SYS_CLOSE  0x02
#define SYS_WRITEC 0x03
#define SYS_WRITE0 0x04
#define SYS_WRITE  0x05
#define SYS_READ   0x06
#define SYS_EXIT   0x18

/* 输出 NUL 结尾字符串 */
void sh_write0(const char *str);

/* 输出 32 位 hex 值 + 空格 */
void sh_write_hex(uint32_t val);

/* 输出 32 位十进制值 */
void sh_write_dec(uint32_t val);

/* 输出单个字符 */
void sh_writec(char c);

/* 程序退出 */
void sh_exit(int code);

#endif /* SEMIHOSTING_H */
