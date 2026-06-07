/*
 * =============================================================================
 * ring_buffer.c -- 环形缓冲区实现
 * =============================================================================
 */
#include "ring_buffer.h"

void ring_buf_init(ring_buf_t *rb, uint8_t *buffer, uint32_t size)
{
    rb->buffer = buffer;
    rb->size = size;
    rb->mask = size - 1; /* size 是 2 的幂, size-1 就是全 1 掩码 */
    rb->head = 0;
    rb->tail = 0;
}

int ring_buf_put(ring_buf_t *rb, uint8_t byte)
{
    uint32_t next_head = (rb->head + 1) & rb->mask;
    if (next_head == rb->tail)
    {
        return 0; /* 满: 下一个写位置会追上读位置 */
    }

    rb->buffer[rb->head] = byte;
    rb->head = next_head;
    return 1;
}

int ring_buf_get(ring_buf_t *rb, uint8_t *byte)
{
    if (rb->tail == rb->head)
    {
        return 0; /* 空 */
    }

    *byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) & rb->mask;
    return 1;
}

int ring_buf_peek(const ring_buf_t *rb, uint8_t *byte)
{
    if (rb->tail == rb->head)
    {
        return 0;
    }

    *byte = rb->buffer[rb->tail];
    return 1;
}

uint32_t ring_buf_available(const ring_buf_t *rb)
{
    return (rb->head - rb->tail) & rb->mask;
}

uint32_t ring_buf_free(const ring_buf_t *rb)
{
    return (rb->tail - rb->head - 1) & rb->mask;
}

void ring_buf_reset(ring_buf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

int ring_buf_is_empty(const ring_buf_t *rb)
{
    return rb->head == rb->tail;
}

int ring_buf_is_full(const ring_buf_t *rb)
{
    return ((rb->head + 1) & rb->mask) == rb->tail;
}

uint32_t ring_buf_write(ring_buf_t *rb, const uint8_t *data, uint32_t len)
{
    uint32_t written = 0;
    while (written < len && ring_buf_put(rb, data[written]))
    {
        written++;
    }
    return written;
}

uint32_t ring_buf_read(ring_buf_t *rb, uint8_t *data, uint32_t len)
{
    uint32_t read = 0;
    while (read < len && ring_buf_get(rb, &data[read]))
    {
        read++;
    }
    return read;
}
