/*
 * =============================================================================
 * semihosting.h -- ARM Semihosting helpers for QEMU debug output
 * =============================================================================
 *
 * Semihosting 利用 BKPT 0xAB 指令让 QEMU 宿主执行 I/O 操作.
 * Cortex-M0 没有 ITM/SWO, semihosting 是最简单的调试输出方式.
 *
 * 所有函数使用 register 变量 + inline asm, 零栈开销.
 * =============================================================================
 */

#ifndef SEMIHOSTING_H
#define SEMIHOSTING_H

#include <stdint.h>

/* 写入以 NUL 结尾的字符串 */
void sh_write0(const char *s);

/* 写入 32 位十六进制值 "0xXXXXXXXX" */
void sh_write_hex(uint32_t v);

/* 写入 32 位十进制值 */
void sh_write_dec(uint32_t v);

/* 写入单个字符 */
void sh_writec(char c);

/* 通过 semihosting 退出 QEMU (SYS_EXIT) */
void sh_exit(int code);

#endif /* SEMIHOSTING_H */
