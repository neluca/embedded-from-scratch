/*
 * =============================================================================
 * stack_guard.c -- 栈溢出检测实现
 * =============================================================================
 *
 * 栈哨兵原理:
 *   1. 系统启动时, 在栈区域的最底端写入哨兵 pattern
 *   2. 运行过程中定期检查哨兵区域
 *   3. 如果哨兵 pattern 被修改 → 栈曾经扩展到此位置 → 有溢出风险
 *
 * RAM 布局 (16KB, 0x20000000-0x20004000):
 *   .data/.bss → heap → [哨兵 64B] → stack → __stack_top
 *
 * 哨兵位置: 在 __heap_end 和 __stack_top 之间
 *   我们把它放在 __heap_end 后面
 */
#include "semihosting.h"
#include "stack_guard.h"
#include <stddef.h>
#include <stdint.h>

/* 链接脚本定义的边界 (需要在链接脚本中定义) */
extern uint8_t __stack_top;
extern uint8_t __heap_end;

/* 哨兵区域: 紧接 heap 之后, stack 之前 */
static uint32_t *sentinel_start = NULL;

void stack_sentinel_init(void)
{
    /* 哨兵放在 heap_end 之后 */
    sentinel_start = (uint32_t *)(((uint32_t)&__heap_end + 3) & ~3); /* 4 字节对齐 */

    /* 用 pattern 填充哨兵区域 */
    for (uint32_t i = 0; i < STACK_SENTINEL_WORDS; i++)
    {
        sentinel_start[i] = STACK_SENTINEL_PATTERN;
    }
}

uint32_t stack_sentinel_check(void)
{
    if (sentinel_start == NULL)
    {
        return 0xFFFFFFFF; /* 未初始化 */
    }

    uint32_t corrupted = 0;
    for (uint32_t i = 0; i < STACK_SENTINEL_WORDS; i++)
    {
        if (sentinel_start[i] != STACK_SENTINEL_PATTERN)
        {
            corrupted++;
        }
    }
    return corrupted;
}

uint32_t stack_guard_get_sp(void)
{
    uint32_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));
    return sp;
}

int stack_guard_check(void)
{
    uint32_t sp = stack_guard_get_sp();

    /* SP 应该在安全范围内: sentinel 之上, __stack_top 之下 */
    if (sp < (uint32_t)sentinel_start)
    {
        return 0; /* SP 低于哨兵 → 栈溢出! */
    }
    if (sp > (uint32_t)&__stack_top)
    {
        return 0; /* SP 超出 RAM 顶部 */
    }
    return 1;
}

uint32_t stack_sentinel_addr(void)
{
    return (uint32_t)sentinel_start;
}

void stack_sentinel_dump(void)
{
    if (sentinel_start == NULL)
    {
        sh_write0("Sentinel not initialized.\n");
        return;
    }

    sh_write0("Stack sentinel @ ");
    sh_write_hex((uint32_t)sentinel_start);
    sh_write0("\n");

    for (uint32_t i = 0; i < STACK_SENTINEL_WORDS; i++)
    {
        sh_write0("  [");
        sh_write_dec(i);
        sh_write0("] = ");
        sh_write_hex(sentinel_start[i]);
        if (sentinel_start[i] != STACK_SENTINEL_PATTERN)
        {
            sh_write0(" <-- CORRUPTED!");
        }
        sh_write0("\n");
    }
}

uint32_t stack_usage_estimate(void)
{
    if (sentinel_start == NULL)
    {
        return 0;
    }

    /* 从哨兵末尾向起始扫描, 找到第一个被修改的字 */
    for (int32_t i = STACK_SENTINEL_WORDS - 1; i >= 0; i--)
    {
        if (sentinel_start[i] != STACK_SENTINEL_PATTERN)
        {
            /* 栈至少到达了这里 → 估算栈使用量 */
            uint32_t stack_area_start = (uint32_t)&sentinel_start[STACK_SENTINEL_WORDS];
            uint32_t stack_bottom = (uint32_t)&sentinel_start[i];
            return stack_area_start - stack_bottom + (STACK_SENTINEL_WORDS - i) * 4;
        }
    }

    /* 所有哨兵完整 → 栈未到达哨兵区域 */
    return 0;
}
