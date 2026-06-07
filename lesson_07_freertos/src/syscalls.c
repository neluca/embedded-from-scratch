/*
 * syscalls.c -- FreeRTOS 需要的辅助函数
 *
 * 提供:
 *   memcpy  - 队列操作需要
 *   malloc  - FreeRTOS 内部不使用, 但 event_groups 可能间接调用
 *
 * FreeRTOS 使用自己的 pvPortMalloc (在 heap_4.c 中),
 * 此处的 malloc 是为标准库提供的.
 */
#include <stddef.h>
#include <stdint.h>

/* memcpy -- 字节复制 (FreeRTOS queue.c 需要) */
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dst;
}

/* memset -- 填充字节 (FreeRTOS 内部使用) */
void *memset(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t *)s;
    for (size_t i = 0; i < n; i++)
    {
        p[i] = (uint8_t)c;
    }
    return s;
}
