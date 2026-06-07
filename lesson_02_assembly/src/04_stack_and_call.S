@ ==============================================================================
@ 04_stack_and_call.S -- Cortex-M0 栈与子程序调用
@ ==============================================================================
@
@ 学习目标:
@ 1. 栈帧 (stack frame) 的结构和用途
@ 2. AAPCS (ARM Architecture Procedure Call Standard) for M0
@ 3. 参数传递: R0-R3 用于参数, R0-R1 用于返回值
@ 4. 调用者保存 vs 被调用者保存寄存器
@ 5. 叶函数 (leaf function) 优化
@ 6. 递归函数在 M0 上的实现
@
@ AAPCS 寄存器角色:
@   R0-R3:  参数/返回值/scratch (调用者保存)
@   R4-R11: 被调用者保存 (函数必须保留它们的值)
@   R12/IP: 过程内调用暂存器 (调用者保存)
@   R13/SP: 栈指针 (8 字节对齐要求)
@   R14/LR: 链接寄存器
@   R15/PC: 程序计数器
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
@ 示例函数 -- 演示各种调用约定
@ ==============================================================================

@ --------------------------------------------------------------------------
@ leaf_add -- 叶函数: 两个数相加 (不需要保存 LR)
@ 叶函数 = 不调用其他函数的函数
@ 优化: 不需要 push {lr}, 因为 LR 未曾被修改
@ --------------------------------------------------------------------------
@ 输入: R0=a, R1=b  输出: R0=a+b
leaf_add:
    adds    r0, r0, r1      @ R0 = R0 + R1
    bx      lr              @ 直接返回 (LR 未变)

@ --------------------------------------------------------------------------
@ nonleaf_add3 -- 非叶函数: 三个数相加
@ 调用 leaf_add 两次, 所以需要用栈保存 LR 和中间结果
@ --------------------------------------------------------------------------
@ 输入: R0=a, R1=b, R2=c  输出: R0=a+b+c
nonleaf_add3:
    push    {r4, lr}        @ 保存 R4 (被调用者保存) 和 LR
    movs    r4, r2          @ R4 = c (保存第三个参数)

    @ 第一次调用: leaf_add(a, b) -> R0
    @ R0=a, R1=b 已经就绪
    bl      leaf_add        @ R0 = a + b, LR = 返回地址
    @ R0 现在有 a+b
    movs    r1, r4          @ R1 = c (第二个参数)
    bl      leaf_add        @ R0 = (a+b) + c

    pop     {r4, pc}        @ 恢复 R4, 通过 POP {pc} 返回
    @ POP {pc} 等价于: POP {r4}; BX LR (但更高效)

@ --------------------------------------------------------------------------
@ sum_array -- 计算数组元素和
@ 使用被调用者保存寄存器保存指针和计数
@ --------------------------------------------------------------------------
@ 输入: R0=数组地址, R1=元素个数  输出: R0=总和
sum_array:
    push    {r4, r5, lr}    @ 保存 R4, R5 (将用作循环变量)
    movs    r4, r0          @ R4 = 数组地址
    movs    r5, r1          @ R5 = 计数
    movs    r0, #0          @ R0 = sum = 0 (返回值)
1:  cmp     r5, #0
    beq     2f              @ 如果计数 == 0, 返回
    ldr     r1, [r4, #0]    @ R1 = *array
    adds    r0, r0, r1      @ sum += *array
    adds    r4, r4, #4      @ array++ (指针+4 字节)
    subs    r5, r5, #1      @ count--
    b       1b
2:  pop     {r4, r5, pc}    @ 恢复并返回

@ --------------------------------------------------------------------------
@ factorial -- 递归阶乘 (演示递归调用和栈帧)
@ 注意: M0 栈空间有限 (microbit 仅 16KB), 递归深度受限!
@ --------------------------------------------------------------------------
@ 输入: R0=n  输出: R0=n!
factorial:
    push    {r4, lr}        @ 保存 R4 和 LR

    cmp     r0, #1
    bgt     1f              @ if n > 1, recurse
    @ 基本情况: n <= 1, 返回 1
    movs    r0, #1
    pop     {r4, pc}
1:  @ 递归: n! = n * (n-1)!
    movs    r4, r0          @ R4 = n (保存当前 n)
    subs    r0, r0, #1      @ R0 = n-1 (递归参数)
    bl      factorial       @ R0 = (n-1)!
    @ 注意: BL 修改了 LR, 但我们已经 push 了 LR
    movs    r1, r4
    muls    r0, r1          @ R0 = n * (n-1)!

    pop     {r4, pc}

@ --------------------------------------------------------------------------
@ many_params -- 演示超过 4 个参数的函数
@ AAPCS: R0-R3 传前 4 个 32 位参数, 其余通过栈传递
@ --------------------------------------------------------------------------
@ 输入: R0=p1, R1=p2, R2=p3, R3=p4, [SP+0]=p5, [SP+4]=p6
@ 输出: R0 = sum of all 6 params
many_params:
    push    {r4, lr}
    adds    r0, r0, r1      @ +p2
    adds    r0, r0, r2      @ +p3
    adds    r0, r0, r3      @ +p4
    @ 第 5, 6 个参数在栈上 (调用者的栈帧中)
    ldr     r1, [sp, #8]    @ p5 (sp+0 是 push 的 r4, sp+4 是 lr, sp+8 是 p5)
    adds    r0, r0, r1      @ +p5
    ldr     r1, [sp, #12]   @ p6 (sp+12)
    adds    r0, r0, r1      @ +p6
    pop     {r4, pc}

@ ==============================================================================
@ reset_handler
@ ==============================================================================
    .global reset_handler
    .type   reset_handler, %function
reset_handler:

    ldr     r0, =msg_title
    bl      semihosting_write0

@ ==============================================================================
@ 第一部分: 叶函数 (leaf function)
@ ==============================================================================
    ldr     r0, =msg_s1
    bl      semihosting_write0

    @ 叶函数是最简单的函数: 不调用其他函数, 不需要保存 LR
    @ 直接在 R0-R3 和 R12 上操作, 通过 BX LR 返回
    movs    r0, #30
    movs    r1, #12
    bl      leaf_add            @ R0 = 42
    bl      semihosting_write_hex

    @ 叶函数反汇编示例:
    @   leaf_add:
    @     adds r0, r0, r1       @ 单周期指令
    @     bx   lr               @ 返回

@ ==============================================================================
@ 第二部分: 非叶函数 (non-leaf function)
@ ==============================================================================
    ldr     r0, =msg_s2
    bl      semihosting_write0

    @ 非叶函数需要:
    @   1. PUSH {..., lr} 保存返回地址
    @   2. 保存跨调用的值到被调用者保存寄存器 (R4-R11)
    @   3. POP {..., pc} 恢复并返回

    movs    r0, #10
    movs    r1, #20
    movs    r2, #12
    bl      nonleaf_add3        @ R0 = 10 + 20 + 12 = 42
    bl      semihosting_write_hex

@ ==============================================================================
@ 第三部分: 被调用者保存寄存器
@ ==============================================================================
    ldr     r0, =msg_s3
    bl      semihosting_write0

    @ 调用者保存: R0-R3, R12, LR (函数可以自由修改)
    @ 被调用者保存: R4-R11, SP (函数必须保留原值)
    @
    @ 如果函数要使用 R4-R11, 必须:
    @   1. PUSH 旧值
    @   2. 使用寄存器
    @   3. POP 恢复

    @ 准备测试数据
    sub     sp, sp, #20         @ 在栈上分配 5 个 int
    mov     r4, sp              @ R4 = 数组地址
    movs    r0, #1
    str     r0, [r4, #0]
    movs    r0, #2
    str     r0, [r4, #4]
    movs    r0, #3
    str     r0, [r4, #8]
    movs    r0, #4
    str     r0, [r4, #12]
    movs    r0, #5
    str     r0, [r4, #16]

    movs    r0, r4              @ R0 = 数组地址
    movs    r1, #5              @ R1 = 5 个元素
    bl      sum_array           @ R0 = 1+2+3+4+5 = 15
    bl      semihosting_write_hex  @ 0x0000000F

    add     sp, sp, #20

@ ==============================================================================
@ 第四部分: 递归函数
@ ==============================================================================
    ldr     r0, =msg_s4
    bl      semihosting_write0

    @ 递归函数必须特别小心:
    @   1. 每次递归调用都需要独立的栈帧
    @   2. 深度递归可能栈溢出 (M0 栈仅几 KB!)
    @   3. 用 PUSH {..., lr} 保存返回地址

    movs    r0, #5              @ 计算 5!
    bl      factorial           @ R0 = 120
    bl      semihosting_write_hex  @ 0x00000078

@ ==============================================================================
@ 第五部分: 栈帧详解
@ ==============================================================================
    ldr     r0, =msg_s5
    bl      semihosting_write0

    @ 典型函数栈帧布局 (从高地址到低地址):
    @   [sp + N]:  局部变量
    @   [sp + 0]:  PUSH 保存的寄存器
    @   [sp - 4]:  调用的函数的参数 (第 5+ 个参数)
    @
    @ SP 在函数入口时:
    @   [sp + 0]:  返回地址 (由调用者的 BL 压入... 不对, BL 存在 LR 中)
    @   实际上: BL 把返回地址存入 LR, 而不是压栈
    @   只有非叶函数才 PUSH {lr} 保存返回地址

    @ 演示: 调用 many_params (6 个参数)
    @ 参数 1-4 在 R0-R3, 参数 5-6 压栈
    movs    r0, #1              @ p1
    movs    r1, #2              @ p2
    movs    r2, #3              @ p3
    movs    r3, #4              @ p4
    @ 参数 5 和 6 通过栈传递 (在调用 many_params 之前压栈)
    movs    r4, #5
    push    {r4}                @ p6 先压 (栈向下增长, 最右参数最高地址)
    movs    r4, #6
    push    {r4}                @ p5 后压
    bl      many_params         @ R0 = 1+2+3+4+5+6 = 21
    add     sp, sp, #8          @ 清理参数栈空间
    bl      semihosting_write_hex  @ 0x00000015

@ ==============================================================================
@ 第六部分: 栈对齐要求
@ ==============================================================================
    ldr     r0, =msg_s6
    bl      semihosting_write0

    @ AAPCS 要求: 在 "公共接口" (函数调用边界) SP 必须 8 字节对齐
    @ M0 硬件在异常入口自动确保 8 字节对齐
    @ 但在手工压栈/弹栈时必须注意

    @ 展示检查当前 SP 对齐:
    mov     r0, sp
    bl      semihosting_write_hex  @ 应该显示 8 字节对齐的地址

    @ Push 单寄存器 (4 字节) 后, SP 可能不再 8 字节对齐:
    push    {r0}
    mov     r0, sp
    bl      semihosting_write_hex  @ 可能不对齐 (x...4 或 x...C)
    pop     {r0}
    @ 恢复对齐

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
    .asciz  "\n=== Cortex-M0 Assembly: Stack & Subroutines ===\n"
msg_s1:
    .asciz  "\n[1] Leaf function -- no LR save needed, BX LR return\n"
msg_s2:
    .asciz  "\n[2] Non-leaf function -- PUSH {lr}/POP {pc} required\n"
msg_s3:
    .asciz  "\n[3] Callee-saved registers (R4-R11)\n"
msg_s4:
    .asciz  "\n[4] Recursive function -- factorial(5)\n"
msg_s5:
    .asciz  "\n[5] Stack frame layout & >4 params via stack\n"
msg_s6:
    .asciz  "\n[6] Stack alignment (8-byte at public interface)\n"
msg_done:
    .asciz  "\n=== Exercise 04 complete! ===\n"

    .end
