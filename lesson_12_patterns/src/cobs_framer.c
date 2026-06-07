/*
 * =============================================================================
 * cobs_framer.c -- COBS 数据帧编解码实现
 * =============================================================================
 *
 * COBS (Consistent Overhead Byte Stuffing):
 *   将任意字节流编码为不含 0x00 的字节流.
 *   0x00 用作帧分隔符.
 *
 * 编码算法:
 *   1. 扫描输入, 找到下一个 0x00 (或剩余字节数 < 255)
 *   2. 输出: [距离下一个0 或 代码块长度] [非零数据...]
 *   3. 重复直到所有数据处理完成
 *   4. 追加 0x00 作为帧尾
 *
 * 解码算法:
 *   1. 读取代码字节 N
 *   2. 如果 N == 0: 帧结束 (或错误)
 *   3. 拷贝 N-1 个字节, 然后插入一个 0x00
 *   4. 重复
 */
#include "cobs_framer.h"

uint32_t cobs_encode(const uint8_t *src, uint32_t src_len, uint8_t *dst)
{
    uint32_t src_idx = 0;
    uint32_t dst_idx = 0;
    uint32_t code_idx = 0; /* 当前代码字节在 dst 中的位置 */
    uint8_t code_val = 1; /* 当前代码字节的值 (到下一个 0x00 的距离) */

    /* 初始代码字节位置 (稍后填写) */
    code_idx = dst_idx;
    dst[dst_idx] = 0; /* 占位 */
    dst_idx++;

    while (src_idx < src_len)
    {
        if (src[src_idx] == 0x00)
        {
            /* 找到了零: 完成当前代码块 */
            dst[code_idx] = code_val;
            code_val = 1;
            code_idx = dst_idx;
            dst[dst_idx] = 0; /* 占位 */
            dst_idx++;
            src_idx++;
        }
        else
        {
            /* 非零数据: 追加 */
            dst[dst_idx] = src[src_idx];
            dst_idx++;
            src_idx++;
            code_val++;

            /* 代码块满了 (最大 255 字节: 1 个代码字节 + 最多 254 个数据字节) */
            if (code_val == 0xFF)
            {
                dst[code_idx] = code_val;
                code_val = 1;
                code_idx = dst_idx;
                dst[dst_idx] = 0; /* 占位 */
                dst_idx++;
            }
        }
    }

    /* 完成最后一个代码块 */
    dst[code_idx] = code_val;

    return dst_idx; /* 不包含帧尾 0x00, 由调用者追加 */
}

uint32_t cobs_decode(const uint8_t *src, uint32_t src_len, uint8_t *dst)
{
    uint32_t src_idx = 0;
    uint32_t dst_idx = 0;

    while (src_idx < src_len)
    {
        uint8_t code = src[src_idx];
        src_idx++;

        if (code == 0x00)
        {
            /* 帧结束 (或提前结束) */
            break;
        }

        /* 拷贝 code-1 个非零字节 */
        uint8_t copy_len = code - 1;
        for (uint8_t i = 0; i < copy_len && src_idx < src_len; i++)
        {
            dst[dst_idx] = src[src_idx];
            dst_idx++;
            src_idx++;
        }

        /* 如果 code < 0xFF, 说明原数据这里有一个 0x00, 插入它 */
        if (code < 0xFF && src_idx < src_len)
        {
            dst[dst_idx] = 0x00;
            dst_idx++;
        }
    }

    return dst_idx;
}

uint32_t cobs_max_encoded_len(uint32_t src_len)
{
    /* 最坏情况: 每个 254 字节需要一个额外的代码字节 + 帧尾 0x00 */
    return src_len + (src_len / 254) + 2;
}
