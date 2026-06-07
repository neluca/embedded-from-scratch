@ ==============================================================================
@ asm_funcs.S -- 可从 C 调用的汇编函数库
@ ==============================================================================
@
@ 演示 C 语言如何调用汇编函数.
@ 所有函数遵循 AAPCS 调用约定:
@   - 参数通过 R0-R3 传递 (前 4 个 32 位参数)
@   - 返回值通过 R0 (和 R1, 如果需要 64 位) 返回
@   - 被调用者必须保留 R4-R11, SP
@   - 调用者可以自由使用 R0-R3, R12, LR
@
@ C 中声明:
@   uint32_t asm_add(uint32_t a, uint32_t b);
@   void asm_delay_cycles(uint32_t cycles);
@   uint32_t asm_clz(uint32_t value);       // count leading zeros
@ ==============================================================================

    .syntax unified
    .cpu    cortex-m0
    .thumb

    .section .text, "ax"
    .align  2

@ --------------------------------------------------------------------------
@ asm_add -- 简单加法 (演示基本参数传递)
@ C 声明: uint32_t asm_add(uint32_t a, uint32_t b);
@ 输入: R0 = a, R1 = b
@ 输出: R0 = a + b
@ --------------------------------------------------------------------------
    .global asm_add
    .type   asm_add, %function
asm_add:
    adds    r0, r0, r1          @ R0 = R0 + R1, 返回值在 R0
    bx      lr                  @ 返回 (叶函数, 无需保存 LR)

@ --------------------------------------------------------------------------
@ asm_mul -- 乘法 (演示 Thumb-1 中 mul 指令)
@ C 声明: uint32_t asm_mul(uint32_t a, uint32_t b);
@ 输入: R0 = a, R1 = b
@ 输出: R0 = a * b (低 32 位)
@ 注意: M0 使用 MULS 而不是 MUL, 结果总是 32 位
@ --------------------------------------------------------------------------
    .global asm_mul
    .type   asm_mul, %function
asm_mul:
    muls    r0, r1              @ R0 = R0 * R1, 设置 N/Z 标志
    bx      lr

@ --------------------------------------------------------------------------
@ asm_abs -- 绝对值 (演示条件分支在汇编中的实现)
@ C 声明: int32_t asm_abs(int32_t x);
@ 输入: R0 = x
@ 输出: R0 = |x|
@ --------------------------------------------------------------------------
    .global asm_abs
    .type   asm_abs, %function
asm_abs:
    cmp     r0, #0
    bge     1f                  @ 如果 >= 0, 跳过取反
    @ M0 没有 RSB 指令, 用 MVNS + ADDS 实现补码取反
    mvns    r0, r0              @ R0 = ~R0
    adds    r0, r0, #1          @ R0 = -R0
1:  bx      lr

@ --------------------------------------------------------------------------
@ asm_clz -- 计算前导零的数量 (Count Leading Zeros)
@ C 声明: uint32_t asm_clz(uint32_t value);
@ 输入: R0 = value
@ 输出: R0 = 前导零个数 (0-32)
@
@ 算法: 从 bit31 向下扫描, 第一个为 1 的位之前的零个数
@ 注意: M0 没有 CLZ 指令! (CLZ 是 ARMv7-M 的)
@ --------------------------------------------------------------------------
    .global asm_clz
    .type   asm_clz, %function
asm_clz:
    push    {r4}
    movs    r4, #0              @ clz 计数器从 0 开始
    ldr     r1, =0x80000000     @ mask = bit31

    @ 对 value==0 特判: 返回 32
    cmp     r0, #0
    beq     3f

1:  tst     r0, r1              @ 测试当前位
    bne     2f                  @ 找到第一个 1, 退出
    adds    r4, r4, #1          @ clz++ (这个位是 0)
    lsrs    r1, r1, #1          @ mask >>= 1 (移向下一位)
    b       1b

2:  movs    r0, r4              @ 返回 clz 计数
    pop     {r4}
    bx      lr

3:  movs    r0, #32             @ value==0: 所有 32 位都是前导零
    pop     {r4}
    bx      lr

@ --------------------------------------------------------------------------
@ asm_delay_cycles -- 精确的周期级延时 (软件循环)
@ C 声明: void asm_delay_cycles(uint32_t cycles);
@ 输入: R0 = 循环次数
@ --------------------------------------------------------------------------
    .global asm_delay_cycles
    .type   asm_delay_cycles, %function
asm_delay_cycles:
    @ 每个循环耗时 3 个周期: subs(1) + bne(2, 分支预测命中)
    @ 实际延时 ≈ cycles * 3
1:  subs    r0, r0, #1
    bne     1b
    bx      lr

@ --------------------------------------------------------------------------
@ asm_swap_bytes -- 字节序反转 (32 位)
@ C 声明: uint32_t asm_swap_bytes(uint32_t value);
@ 输入: R0 = 0x12345678 (A B C D, 从高位到低位)
@ 输出: R0 = 0x78563412 (D C B A)
@
@ 注意: M0 没有 REV 指令! (REV 是 ARMv7-M 的)
@ --------------------------------------------------------------------------
    .global asm_swap_bytes
    .type   asm_swap_bytes, %function
asm_swap_bytes:
    push    {r4, r5, r6}

    @ 提取各字节: R0 = [A][B][C][D] (A=bits 24-31, D=bits 0-7)
    movs    r1, #0xFF

    @ R6 = byte A (bits 24-31, 最终去 bits 0-7)
    movs    r6, r0
    lsrs    r6, r6, #24         @ R6 = A = 0x12

    @ R5 = byte B (bits 16-23, 最终去 bits 8-15)
    movs    r4, #0xFF
    lsls    r4, r4, #16         @ mask = 0x00FF0000
    movs    r5, r0
    ands    r5, r4              @ R5 = B in place
    lsrs    r5, r5, #16         @ R5 = B = 0x34

    @ R3 = byte C (bits 8-15, 最终去 bits 16-23)
    movs    r4, #0xFF
    lsls    r4, r4, #8          @ mask = 0x0000FF00
    movs    r3, r0
    ands    r3, r4              @ R3 = C in place
    lsrs    r3, r3, #8          @ R3 = C = 0x56

    @ R1 = byte D (bits 0-7, 最终去 bits 24-31)
    ands    r1, r0              @ R1 = D = 0x78

    @ 重新组合: D(78)<<24 | C(56)<<16 | B(34)<<8 | A(12)
    lsls    r1, r1, #24         @ D → 0x78000000
    lsls    r3, r3, #16         @ C → 0x00560000
    lsls    r5, r5, #8          @ B → 0x00003400
    @ R6 = A = 0x12, 已在低 8 位

    movs    r0, r1              @ R0 = 0x78000000
    adds    r0, r0, r3          @ +0x00560000 = 0x78560000
    adds    r0, r0, r5          @ +0x00003400 = 0x78563400
    adds    r0, r0, r6          @ +0x00000012 = 0x78563412

    pop     {r4, r5, r6}
    bx      lr

@ --------------------------------------------------------------------------
@ asm_memcpy32 -- 优化的 32 位对齐 memcpy (字复制 + 字节残余)
@ C 声明: void *asm_memcpy32(void *dst, const void *src, size_t n);
@ 输入: R0=dst, R1=src, R2=n
@ 输出: R0=dst
@
@ 这是嵌入式库中 memcpy 的典型实现:
@   先按 4 字节块复制, 再处理剩余 0-3 字节
@ --------------------------------------------------------------------------
    .global asm_memcpy32
    .type   asm_memcpy32, %function
asm_memcpy32:
    push    {r4, r5}
    movs    r4, r0              @ R4 = dst (保存返回值)

    @ 先按字复制
    lsrs    r3, r2, #2          @ R3 = n / 4 (字数)
    beq     2f                  @ 无完整字, 跳字节复制

1:  ldr     r5, [r1, #0]        @ load word
    str     r5, [r0, #0]        @ store word
    adds    r0, r0, #4
    adds    r1, r1, #4
    subs    r3, r3, #1
    bne     1b

2:  @ 处理剩余字节
    movs    r3, #3
    ands    r2, r3              @ R2 = n & 3
    beq     4f

3:  ldrb    r3, [r1, #0]
    strb    r3, [r0, #0]
    adds    r0, r0, #1
    adds    r1, r1, #1
    subs    r2, r2, #1
    bne     3b

4:  movs    r0, r4              @ 返回 dst
    pop     {r4, r5}
    bx      lr

    .end
