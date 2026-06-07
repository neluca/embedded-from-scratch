/*
 * =============================================================================
 * soft_crc.h -- 软件 CRC32 校验 (用于数据完整性验证)
 * =============================================================================
 *
 * CRC (Cyclic Redundancy Check) 是嵌入式系统中最常用的数据完整性校验.
 *
 * 典型用途:
 *   - 配置数据完整性验证 (写入 Flash 前后)
 *   - 固件映像校验 (Bootloader 验证 App 镜像)
 *   - 通信协议校验 (Modbus, 自定义协议)
 *   - 关键 RAM 数据结构保护 (检测内存损坏)
 *
 * 本模块实现 CRC-32 (多项式 0xEDB88320, 与 zlib/gzip 兼容).
 * M0 无硬件 CRC, 这是纯软件查表实现.
 *
 * 查表法 vs 计算法:
 *   - 查表:  256 × 4 = 1KB 表, 每个字节 ~10 周期
 *   - 计算:  0 字节表, 每个字节 ~100 周期
 *   嵌入式通常选择查表法 (1KB 换速度)
 */
#ifndef SOFT_CRC_H
#define SOFT_CRC_H

#include <stdint.h>

/* 计算 CRC-32 */
uint32_t crc32(const void *data, uint32_t length);

/* 计算 CRC-32 (带初始值, 用于分段计算) */
uint32_t crc32_continue(const void *data, uint32_t length, uint32_t init_crc);

/* 快速 CRC-32 校验: data + appended_crc 应为 0 */
int crc32_check(const void *data, uint32_t length_with_crc);

/* 在数据末尾附加 CRC-32 (buffer 需要留 4 字节空间) */
void crc32_append(void *data, uint32_t length);

#endif /* SOFT_CRC_H */
