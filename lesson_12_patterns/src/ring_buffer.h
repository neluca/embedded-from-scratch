/*
 * =============================================================================
 * ring_buffer.h -- 环形缓冲区 (Circular Buffer / Ring Buffer)
 * =============================================================================
 *
 * 环形缓冲区是嵌入式系统中最通用的数据结构.
 * 跨架构、跨语言: ARM/RISC-V/x86, C/C++/Rust — 模式完全一致.
 *
 * 典型用途:
 *   - UART 收/发缓冲 (ISR → 主循环 数据传递)
 *   - ADC 采样缓冲 (DMA → 数据处理)
 *   - 音频流缓冲
 *   - 传感器数据暂存
 *
 * 本实现特性:
 *   - 大小必须是 2 的幂 (用位掩码代替取模, 在无除法器的 M0 上快 ~100 倍!)
 *   - 单生产者单消费者模式 → ISR 安全 (无需锁/临界区)
 *   - 始终保留一个空槽: 简化空/满判断
 *
 * 线程安全说明:
 *   - 一个 ISR (生产者) + 一个主循环 (消费者) → 无需锁
 *   - 多个生产者或多个消费者 → 需要临界区保护
 */
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>

typedef struct
{
    uint8_t *buffer;     /* 数据缓冲区指针 */
    uint32_t size;       /* 缓冲区大小 (2 的幂) */
    uint32_t mask;       /* size - 1 (用于快速取模) */
    uint32_t head;       /* 写指针 (生产者修改) */
    uint32_t tail;       /* 读指针 (消费者修改) */
} ring_buf_t;

/* 初始化环形缓冲区. size 必须是 2 的幂 (如 64, 128, 256, 1024) */
void ring_buf_init(ring_buf_t *rb, uint8_t *buffer, uint32_t size);

/* 写入一个字节. 返回 1=成功, 0=缓冲区满 */
int ring_buf_put(ring_buf_t *rb, uint8_t byte);

/* 读取一个字节. 返回 1=成功, 0=缓冲区空 */
int ring_buf_get(ring_buf_t *rb, uint8_t *byte);

/* 查看下一个可读字节但不移除. 返回 1=成功, 0=空 */
int ring_buf_peek(const ring_buf_t *rb, uint8_t *byte);

/* 可读字节数 */
uint32_t ring_buf_available(const ring_buf_t *rb);

/* 可写字节数 (空闲空间) */
uint32_t ring_buf_free(const ring_buf_t *rb);

/* 清空缓冲区 */
void ring_buf_reset(ring_buf_t *rb);

/* 是否为空 */
int ring_buf_is_empty(const ring_buf_t *rb);

/* 是否为满 */
int ring_buf_is_full(const ring_buf_t *rb);

/* 批量写入 (返回实际写入字节数) */
uint32_t ring_buf_write(ring_buf_t *rb, const uint8_t *data, uint32_t len);

/* 批量读取 (返回实际读取字节数) */
uint32_t ring_buf_read(ring_buf_t *rb, uint8_t *data, uint32_t len);

#endif /* RING_BUFFER_H */
