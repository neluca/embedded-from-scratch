/*
 * =============================================================================
 * cobs_framer.h -- COBS (Consistent Overhead Byte Stuffing) 数据帧编解码
 * =============================================================================
 *
 * 问题: UART/SPI 传递的是字节流, 如何知道一个"数据包"从哪里开始、到哪里结束?
 *
 * 解决方案: 数据帧 (Framing) — 在字节流中标记数据包边界.
 *
 * COBS 是一种高效的帧编码:
 *   - 用 0x00 作为包分隔符 (帧边界)
 *   - 编码: 将数据中的 0x00 替换为 (距离下一个 0x00 的偏移)
 *   - 开销: 固定 1 字节 / 254 数据字节 (最坏情况)
 *   - 最大扩展: N + ceil(N/254) 字节 (非常确定!)
 *
 * COBS 对比其他方案:
 *   SLIP:      最坏 2x 扩展, 简单但低效
 *   COBS:      最坏 +0.4% 开销, 确定性强
 *   Length-prefix: 0% 开销, 但需要可靠的长度字段
 *
 * COBS 编码示例:
 *   输入:  [45 00 00 55 00 00 04 00]
 *   输出:  [02 45 01 01 55 02 01 04 01 00]
 *          ↑代码块1  ↑代码块2      ↑代码块3  ↑帧尾
 *
 * 跨平台: 此算法与 CPU 架构无关, 可在任何平台上编译运行.
 */
#ifndef COBS_FRAMER_H
#define COBS_FRAMER_H

#include <stdint.h>

/* COBS 编码: 将数据编码为不含 0x00 的字节流
 * src:      输入数据
 * src_len:  输入长度
 * dst:      输出缓冲区 (至少 src_len + src_len/254 + 2 字节)
 * 返回:     编码后的字节数 (不含帧尾 0x00)
 */
uint32_t cobs_encode(const uint8_t *src, uint32_t src_len, uint8_t *dst);

/* COBS 解码: 将编码字节流还原为原始数据
 * src:      编码数据 (不含帧尾 0x00)
 * src_len:  编码数据长度
 * dst:      输出缓冲区 (至少 src_len 字节)
 * 返回:     解码后的字节数, 0=解码失败
 */
uint32_t cobs_decode(const uint8_t *src, uint32_t src_len, uint8_t *dst);

/* 计算 COBS 编码后的最大长度 */
uint32_t cobs_max_encoded_len(uint32_t src_len);

#endif /* COBS_FRAMER_H */
