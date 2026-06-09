/* semihosting helpers for FreeRTOS demo */
#ifndef SEMIHOSTING_H
#define SEMIHOSTING_H
#include <stdint.h>
void sh_write0(const char *str);
void sh_write_hex(uint32_t val);
void sh_write_dec(uint32_t val);
void sh_exit(int code);
#endif
