/*
 * =============================================================================
 * stack_guard.h -- 栈溢出检测 (Stack Overflow Guard)
 * =============================================================================
 *
 * Cortex-M0 没有 MPU (Memory Protection Unit), 无法用硬件检测栈溢出.
 * 必须用软件方法:
 *
 * 方法 1: 栈哨兵 (Stack Sentinel / Stack Painting)
 *   在启动时用已知模式填充栈区域, 定期检查最远端的 pattern 是否被覆盖.
 *   优点: 简单, 几乎无开销
 *   缺点: 只能事后检测 (栈已经越过了哨兵区域)
 *
 * 方法 2: 栈边界检查
 *   在函数入口检查 SP 是否超出预留范围.
 *   优点: 更精确
 *   缺点: 每个函数都有开销 (通常只在关键函数中检查)
 *
 * 本模块实现这两种方法.
 *
 * microbit 的 16KB RAM 布局 (从低到高):
 *   .data + .bss  →  heap (向上)  →  ...  →  stack (向下)  →  __stack_top
 *
 * 哨兵区域放在 stack 的最底端 (heap 上面):
 *   哨兵大小 = 64 字节 (16 个字)
 *   如果在哨兵处检测到非 pattern 值 → 栈曾经到达过这里 → 有溢出风险
 */
#ifndef STACK_GUARD_H
#define STACK_GUARD_H

#include <stdint.h>

/* 栈哨兵大小: 64 字节 = 16 个 32 位字 */
#define STACK_SENTINEL_WORDS 16

/* 哨兵填充 pattern: 0xDEADBEEF (容易在内存 dump 中识别) */
#define STACK_SENTINEL_PATTERN 0xDEADBEEF

/* =========================================================================
 * API
 * ========================================================================= */

/* 初始化栈哨兵 — 在启动代码调用 main() 之前调用 */
void stack_sentinel_init(void);

/* 检查栈哨兵是否完整 — 返回被覆盖的字数 */
uint32_t stack_sentinel_check(void);

/* 获取当前 SP 值 (用于边界检查) */
uint32_t stack_guard_get_sp(void);

/* 检查 SP 是否在安全范围内 — 返回 1=安全, 0=危险 */
int stack_guard_check(void);

/* 获取栈哨兵区域的起始地址 */
uint32_t stack_sentinel_addr(void);

/* 打印栈哨兵区域内容 (用于调试) */
void stack_sentinel_dump(void);

/* 栈使用量估算: 写入 sentinel 后, 检查最远被覆盖的位置 */
uint32_t stack_usage_estimate(void);

#endif /* STACK_GUARD_H */
