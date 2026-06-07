/*
 * =============================================================================
 * semihosting.h -- Semihosting helper
 * =============================================================================
 */
#ifndef SEMIHOSTING_H
#define SEMIHOSTING_H
#include <stdint.h>
void sh_write0(const char *str);
void sh_write_hex(uint32_t val);
#endif
