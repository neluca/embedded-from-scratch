/*
 * =============================================================================
 * semihosting.h -- Semihosting 辅助函数声明
 * =============================================================================
 */
#ifndef SEMIHOSTING_H
#define SEMIHOSTING_H

#include <stdint.h>

#define SYS_OPEN   0x01
#define SYS_CLOSE  0x02
#define SYS_WRITEC 0x03
#define SYS_WRITE0 0x04
#define SYS_WRITE  0x05
#define SYS_READ   0x06
#define SYS_EXIT   0x18

void sh_write0(const char *str);
void sh_write_hex(uint32_t val);
void sh_write_dec(uint32_t val);
void sh_writec(char c);
void sh_exit(int code);

#endif /* SEMIHOSTING_H */
