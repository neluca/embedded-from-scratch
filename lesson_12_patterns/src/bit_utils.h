/*
 * =============================================================================
 * bit_utils.h -- 位操作工具箱 (Bit Manipulation)
 * =============================================================================
 *
 * 位操作是嵌入式编程的基本功.
 * 无论你用什么芯片 (ARM/RISC-V/MIPS/x86), 位操作逻辑完全一致.
 *
 * 本模块涵盖:
 *   1. 单比特操作 (set/clear/toggle/test)
 *   2. 多比特位域操作 (extract/insert)
 *   3. 端序 (endianness) 检测与转换
 *   4. 结构体对齐与打包
 *   5. 常见的寄存器操作模式
 *
 * M0 注意事项:
 *   - 无 BFI/UBFX/SBFX 位域指令 (ARMv7-M 才有)
 *   - 所有位域操作依赖 AND/ORR/LSL/LSR 组合
 *   - 这意味着你学到的"手动"方法在所有平台上都适用!
 */
#ifndef BIT_UTILS_H
#define BIT_UTILS_H

#include <stdint.h>

/* =========================================================================
 * 单比特操作 — 可用于任何 volatile 寄存器
 * ========================================================================= */

/* 置位: reg |= (1U << bit) */
#define BIT_SET(reg, bit) ((reg) |= (1U << (bit)))

/* 清零: reg &= ~(1U << bit) */
#define BIT_CLEAR(reg, bit) ((reg) &= ~(1U << (bit)))

/* 翻转: reg ^= (1U << bit) */
#define BIT_TOGGLE(reg, bit) ((reg) ^= (1U << (bit)))

/* 测试: (reg >> bit) & 1 */
#define BIT_TEST(reg, bit) (((reg) >> (bit)) & 1U)

/* 写值: 先清零再置位 (用于写整个寄存器时) */
#define BIT_WRITE(reg, bit, val) \
    ((reg) = ((reg) & ~(1U << (bit))) | (((val) & 1U) << (bit)))

/* =========================================================================
 * 多比特位域操作 — 手动实现 (M0 无 BFI/UBFX!)
 * =========================================================================
 *
 * 提取位域:
 *   value = (reg >> SHIFT) & MASK
 *
 * 写入位域:
 *   reg = (reg & ~MASK) | ((value << SHIFT) & MASK)
 */

/* 从 reg 提取 [msb:lsb] 位域 (包含边界) */
#define BF_EXTRACT(reg, lsb, msb) \
    (((reg) >> (lsb)) & ((1U << ((msb) - (lsb) + 1)) - 1U))

/* 生成 [msb:lsb] 位域 mask */
#define BF_MASK(lsb, msb) \
    (((1U << ((msb) - (lsb) + 1)) - 1U) << (lsb))

/* 向 reg 的 [msb:lsb] 写入 value */
#define BF_INSERT(reg, lsb, msb, value)     \
    ((reg) = ((reg) & ~BF_MASK(lsb, msb)) | \
             (((value) << (lsb)) & BF_MASK(lsb, msb)))

/* =========================================================================
 * 端序 (Endianness) — 数据在内存中的字节顺序
 * =========================================================================
 *
 * 小端 (Little-Endian): 低字节在低地址 (ARM 默认, x86)
 *   0x12345678 在内存中: [78][56][34][12]
 *
 * 大端 (Big-Endian): 高字节在低地址 (网络字节序, 某些 DSP)
 *   0x12345678 在内存中: [12][34][56][78]
 *
 * Cortex-M0 是小端 (硬件固定), 但与其他系统通信时需要转换.
 */

/* 运行时检测当前系统的端序 */
int is_little_endian(void);

/* 字节序反转 16/32 位 (M0 无 REV 指令, 纯软件实现!) */
uint16_t swap16(uint16_t val);
uint32_t swap32(uint32_t val);

/* 主机序 → 网络序 (大端). 在小端系统上就是 swap */
#define hton16(x) (is_little_endian() ? swap16(x) : (x))
#define hton32(x) (is_little_endian() ? swap32(x) : (x))
#define ntoh16(x) hton16(x) /* 对称操作 */
#define ntoh32(x) hton32(x)

/* =========================================================================
 * 结构体打包与对齐
 * =========================================================================
 *
 * Cortex-M0 对齐要求:
 *   uint8_t:  1 字节对齐
 *   uint16_t: 2 字节对齐
 *   uint32_t: 4 字节对齐
 *
 * 编译器会自动插入 padding 以满足对齐要求.
 * __attribute__((packed)) 可禁用 padding (以性能换空间).
 *
 * 警告: M0 不支持非对齐访问!
 *   非对齐的 uint32_t* 解引用 → HardFault
 *   如果必须访问非对齐数据, 使用 memcpy() 或逐字节读取
 */

/* 示例: 对比对齐和 packed 结构体 */
typedef struct
{
    uint8_t a;  /* offset 0 */
    /* padding 3 bytes */
    uint32_t b;  /* offset 4 */
    uint8_t c;  /* offset 8 */
    /* padding 3 bytes */
} aligned_struct_t; /* sizeof = 12 */

typedef struct __attribute__((packed))
{
    uint8_t a;  /* offset 0 */
    uint32_t b;  /* offset 1 (非对齐! 访问危险!) */
    uint8_t c;  /* offset 5 */
} packed_struct_t; /* sizeof = 6 */

/* 安全访问 packed 成员的辅助函数 */
uint32_t packed_read_u32(const uint8_t *addr);
void packed_write_u32(uint8_t *addr, uint32_t val);

#endif /* BIT_UTILS_H */
