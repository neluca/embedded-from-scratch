@ ==============================================================================
@ 03_branch_and_loop.S -- Cortex-M0 分支与循环
@ ==============================================================================
@
@ 学习目标:
@ 1. B <cond>: 条件分支 (依赖 APSR 标志位)
@ 2. BL: 带链接的分支 (函数调用)
@ 3. BX/BLX: 寄存器间接分支
@ 4. CBZ/CBNZ: 比较并分支 (零/非零)
@ 5. IT 块: If-Then 条件执行 (M0 唯一的条件执行方式!)
@ 6. 循环构造: for/while 的汇编实现
@
@ 条件码 (APSR flags: N-Z-C-V):
@   EQ (Z=1): 相等          NE (Z=0): 不等
@   CS/HS (C=1): 进位/无符号≥   CC/LO (C=0): 无进位/无符号<
@   MI (N=1): 负数           PL (N=0): 正数或零
@   VS (V=1): 溢出           VC (V=0): 无溢出
@   HI (C=1&Z=0): 无符号>   LS (C=0|Z=1): 无符号≤
@   GE (N=V): 有符号≥       LT (N!=V): 有符号<
@   GT (N=V&Z=0): 有符号>   LE (N!=V|Z=1): 有符号≤
@ ==============================================================================

    .syntax unified
    .cpu    cortex-m0
    .thumb

@ ==============================================================================
@ 向量表
@ ==============================================================================
    .section .vector_table, "a"
    .align  2
    .global vector_table
    .type   vector_table, %object
vector_table:
    .word   __stack_top
    .word   reset_handler
    .word   default_handler
    .word   default_handler
    .space  (15 - 4 + 1) * 4
    .word   default_handler
    .space  32 * 4

@ ==============================================================================
@ Semihosting 辅助
@ ==============================================================================
    .section .text, "ax"
    .align  2

semihosting_write0:
    movs    r1, r0
    movs    r0, #0x04
    bkpt    #0xAB
    bx      lr

semihosting_write_hex:
    push    {r4, lr}
    sub     sp, sp, #12
    mov     r4, sp
    movs    r1, #'0'
    strb    r1, [r4, #0]
    movs    r1, #'x'
    strb    r1, [r4, #1]
    movs    r3, #8
1:  subs    r3, r3, #1
    movs    r2, r0
    movs    r1, #0x0F
    ands    r2, r1
    cmp     r2, #10
    blt     2f
    adds    r2, r2, #('A' - 10 - '0')
2:  adds    r2, r2, #'0'
    adds    r1, r4, r3
    adds    r1, r1, #2
    strb    r2, [r1, #0]
    lsrs    r0, r0, #4
    cmp     r3, #0
    bne     1b
    movs    r1, #0
    strb    r1, [r4, #10]
    movs    r0, r4
    bl      semihosting_write0
    movs    r1, #'\n'
    strb    r1, [r4, #0]
    movs    r1, #0
    strb    r1, [r4, #1]
    movs    r0, r4
    bl      semihosting_write0
    add     sp, sp, #12
    pop     {r4, pc}

default_handler:
    b       .

@ ==============================================================================
@ 辅助函数
@ ==============================================================================

@ double_it -- 将 R0 的值加倍 (演示 BL 调用)
@ 输入: R0 = 输入值
@ 输出: R0 = 输入值 * 2
double_it:
    lsls    r0, r0, #1
    bx      lr

@ abs_value -- 返回 R0 的绝对值
@ 输入: R0 = 有符号整数
@ 输出: R0 = |R0|
abs_value:
    cmp     r0, #0
    bge     1f                  @ 如果 R0 >= 0, 直接返回
    @ 否则取反 (M0 没有 RSB, 用 MVNS+ADDS)
    mvns    r0, r0
    adds    r0, r0, #1          @ 补码取反: -x = ~x + 1
1:  bx      lr

@ sum_to_n -- 计算 1+2+...+n
@ 输入: R0 = n
@ 输出: R0 = sum
sum_to_n:
    push    {r4}
    movs    r4, #0              @ sum = 0
1:  cmp     r0, #0
    beq     2f                  @ if n == 0, done
    adds    r4, r4, r0          @ sum += n
    subs    r0, r0, #1          @ n--
    b       1b
2:  movs    r0, r4
    pop     {r4}
    bx      lr

@ ==============================================================================
@ reset_handler
@ ==============================================================================
    .global reset_handler
    .type   reset_handler, %function
reset_handler:

    ldr     r0, =msg_title
    bl      semihosting_write0

@ ==============================================================================
@ 第一部分: B 无条件分支和基本 BL 调用
@ ==============================================================================
    ldr     r0, =msg_s1
    bl      semihosting_write0

    @ B label: 无条件跳转 (Thumb-1 范围: ±2KB 用于 16 位编码)
    @ BL label: 带链接跳转 (LR = 返回地址, 范围: ±16MB, 32 位编码)
    @   用于函数调用

    @ 演示 BL 调用 double_it
    movs    r0, #21
    bl      double_it           @ R0 = 42
    bl      semihosting_write_hex

    @ 演示 abs_value
    ldr     r0, =0xFFFFFFF6     @ R0 = -10
    bl      abs_value           @ R0 = 10
    bl      semihosting_write_hex  @ 0x0000000A

    @ 演示 sum_to_n
    movs    r0, #10
    bl      sum_to_n            @ R0 = 55 (1+2+...+10)
    bl      semihosting_write_hex  @ 0x00000037

@ ==============================================================================
@ 第二部分: 条件分支
@ ==============================================================================
    ldr     r0, =msg_s2
    bl      semihosting_write0

    @ CMP + B<cond>: 比较 + 条件跳转
    @ 这是 M0 上实现 if-then-else 的标准方式

    movs    r0, #5
    movs    r1, #10

    @ if (R0 < R1)
    cmp     r0, r1
    bge     1f                  @ if R0 >= R1, skip (反向条件)
    @ then: R0 < R1
    ldr     r0, =msg_less
    bl      semihosting_write0
    b       2f
1:  @ else: R0 >= R1
    ldr     r0, =msg_ge
    bl      semihosting_write0
2:  @ endif

    @ 多分支: if-else if-else
    movs    r0, #42

    @ if (R0 < 0)
    cmp     r0, #0
    bge     1f
    ldr     r0, =msg_neg
    bl      semihosting_write0
    b       3f
1:  @ else if (R0 == 0)
    cmp     r0, #0
    bne     2f
    ldr     r0, =msg_zero
    bl      semihosting_write0
    b       3f
2:  @ else
    ldr     r0, =msg_pos
    bl      semihosting_write0
3:

@ ==============================================================================
@ 第三部分: CBZ/CBNZ 零值跳转
@ ==============================================================================
    ldr     r0, =msg_s3
    bl      semihosting_write0

    @ CBZ Rn, label: 如果 Rn == 0, 跳转
    @ CBNZ Rn, label: 如果 Rn != 0, 跳转
    @ 范围: 前向 0-126 字节, 后向 0-128 字节
    @ 优势: 单条 16 位指令, 不修改标志位!

    @ 用 CMP+BEQ 实现 strlen 式循环
    @ (GAS 对 cortex-m0 的 CBZ/CBNZ 支持有限, 用 CMP+BEQ 代替)
    ldr     r0, =test_string       @ R0 = 字符串地址
    movs    r1, #0                  @ count = 0
1:  ldrb    r2, [r0, r1]            @ R2 = str[count]
    cmp     r2, #0                  @ 用 CMP+BEQ 代替 CBZ
    beq     2f                      @ if '\0', done
    adds    r1, r1, #1              @ count++
    b       1b
2:  @ R1 = 字符串长度

    movs    r0, r1
    bl      semihosting_write_hex  @ 应显示 0x00000005 ("Hello" = 5)

@ ==============================================================================
@ 第四部分: IT 块 -- 条件执行 (M0/ARMv6-M 支持)
@ ==============================================================================
@ 注意: GAS 2.36+ 对 cortex-m0 的 IT 块支持可能受限.
@ 在生产代码中, IT 块用于减少分支, 避免流水线刷新.
@ 此处展示 IT 块的概念, 并用分支作为后备实现.
    ldr     r0, =msg_s4
    bl      semihosting_write0

    @ IT 块是 ARMv6-M 支持的轻量级条件执行机制.
    @ 格式: IT<x><y><z> <cond> ; 后跟 1-4 条 T(Then)/E(Else) 指令
    @ M0 的 IT 块最多包含 4 条指令, 每个都是 16 位.
    @
    @ 如果汇编器不支持 IT 块, 等效实现是用条件分支:
    @   CMP + B<cond> + (fall-through 或 branch over)

    @ --- 示例 1: if (R0 == 0) R1 = 1; else R1 = 2; ---
    movs    r0, #0
    cmp     r0, #0
    bne     1f                  @ if != 0, goto else
    @ then:
    movs    r1, #1
    b       2f
1:  @ else:
    movs    r1, #2
2:  @ endif
    @ R1 = 1 (R0 == 0)
    movs    r0, r1
    bl      semihosting_write_hex  @ 0x00000001

    @ --- 示例 2: if (R0 >= 10) { R1=0xAA; R2=0xBB; } else { R1=0xCC; } ---
    movs    r0, #15
    cmp     r0, #10
    blt     1f                  @ if < 10, goto else
    @ then:
    movs    r1, #0xAA
    movs    r2, #0xBB
    b       2f
1:  @ else:
    movs    r1, #0xCC
2:  @ endif
    @ R1=0xAA, R2=0xBB

    @ --- IT 块理论 ---
    @ 在支持 IT 的汇编器上, 等效代码为:
    @   cmp r0, #0
    @   ite eq
    @   moveq r1, #1
    @   movne r1, #2
    @
    @ IT 块 vs 分支的权衡:
    @   IT 块: 无需分支预测, 但占用更多代码空间 (每个条件指令 16 位)
    @   分支:  简单直观, 但可能引起流水线刷新 (M0 的 3 级流水线代价小)

@ ==============================================================================
@ 第五部分: 循环构造
@ ==============================================================================
    ldr     r0, =msg_s5
    bl      semihosting_write0

    @ for 循环: for (i=0; i<5; i++) { ... }
    @ 汇编实现:
    movs    r4, #0              @ i = 0
1:  cmp     r4, #5              @ i < 5 ?
    bge     2f                  @ if i >= 5, exit loop
    @ loop body (用 semihosting 输出循环计数)
    adds    r4, r4, #1          @ i++
    b       1b
2:

    @ while 循环: while (n > 0) { n--; }
    movs    r0, #5
1:  cmp     r0, #0
    beq     2f                  @ n == 0, exit
    subs    r0, r0, #1          @ n--
    b       1b
2:
    @ R0 = 0

    @ do-while 循环: do { x++; } while (x < 10);
    movs    r0, #0
1:  adds    r0, r0, #1          @ x++
    cmp     r0, #10
    blt     1b                  @ x < 10, continue
    @ R0 = 10

    bl      semihosting_write_hex  @ 0x0000000A

    @ 带 CBZ 的紧凑循环 (适合 M0)
    @ while (*p++ != 0) { count++; }
    @ (count 某值的循环)

@ ==============================================================================
@ 第六部分: BX/BLX 寄存器分支
@ ==============================================================================
    ldr     r0, =msg_s6
    bl      semihosting_write0

    @ BX Rm: 跳转到 Rm 中的地址
    @ BLX Rm: 跳转到 Rm, LR = 返回地址
    @
    @ 重要: Rm 的 bit[0] 决定处理器状态
    @   bit[0] = 1: Thumb 状态
    @   bit[0] = 0: ARM 状态 (M0 不支持! 会导致 UsageFault/HardFault)
    @   在 M0 上, bit[0] 必须总是 1!
    @
    @ 所有函数地址的 bit[0] 自动为 1 (汇编器和链接器处理)

    @ 用函数指针调用 double_it
    ldr     r0, =double_it       @ R0 = double_it 地址 (bit[0]=1)
    movs    r1, #5               @ 参数
    movs    r0, r1               @ R0 = 5
    @ 注意: BLX Rm 在 Thumb-1 M0 中: Rm 不能是 LR 或 PC
    blx     r0                   @ 调用 double_it, R0=10, LR=返回地址
    @ R0 = 10 (5*2)
    bl      semihosting_write_hex  @ 0x0000000A

@ ==============================================================================
@ 退出
@ ==============================================================================
    ldr     r0, =msg_done
    bl      semihosting_write0

    movs    r0, #0x18
    movs    r1, #0
    push    {r1}
    mov     r1, sp
    bkpt    #0xAB
    b       .

@ ==============================================================================
@ 只读数据
@ ==============================================================================
    .section .rodata, "a"
    .align  2

msg_title:
    .asciz  "\n=== Cortex-M0 Assembly: Branch & Loop ===\n"
msg_s1:
    .asciz  "\n[1] B/BL -- Unconditional branch & function call\n"
msg_s2:
    .asciz  "\n[2] B<cond> -- Conditional branch (CMP + branch)\n"
msg_s3:
    .asciz  "\n[3] CBZ/CBNZ -- Compare and branch if zero/nonzero\n"
msg_s4:
    .asciz  "\n[4] IT -- If-Then conditional execution block\n"
msg_s5:
    .asciz  "\n[5] Loop patterns -- for/while/do-while in assembly\n"
msg_s6:
    .asciz  "\n[6] BX/BLX -- Register indirect branch\n"
msg_less:
    .asciz  "  5 < 10: branch taken (BLT)\n"
msg_ge:
    .asciz  "  Not less\n"
msg_neg:
    .asciz  "  Value is negative\n"
msg_zero:
    .asciz  "  Value is zero\n"
msg_pos:
    .asciz  "  Value is positive\n"
msg_done:
    .asciz  "\n=== Exercise 03 complete! ===\n"

test_string:
    .asciz  "Hello"

    .end
