@ ==============================================================================
@ 02_memory_access.S -- Cortex-M0 内存访问指令
@ ==============================================================================
@
@ 学习目标:
@ 1. LDR/STR: 字 (32-bit) 加载和存储, 各种寻址模式
@ 2. LDRB/STRB, LDRH/STRH: 字节和半字操作
@ 3. LDM/STM: 多寄存器加载/存储
@ 4. PUSH/POP: 栈操作
@ 5. PC-relative 加载和 ADR 伪指令
@
@ Thumb-1 限制:
@   - LDR/STR 的立即数偏移仅 5 位 (0-124, 字对齐)
@   - LDRB/STRB 偏移 5 位 (0-31)
@   - LDRH/STRH 偏移 5 位 (0-62, 半字对齐)
@   - LDM/STM 仅支持低寄存器 (R0-R7)
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
    .word   default_handler         @ NMI
    .word   default_handler         @ HardFault
    .space  (15 - 4 + 1) * 4
    .word   default_handler         @ SysTick
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

@ semihosting_write_hex -- 输出 R0 的 hex 值 + 换行
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
@ reset_handler
@ ==============================================================================
    .global reset_handler
    .type   reset_handler, %function
reset_handler:

    ldr     r0, =msg_title
    bl      semihosting_write0

@ ==============================================================================
@ 第一部分: LDR/STR 字操作
@ ==============================================================================
    ldr     r0, =msg_s1
    bl      semihosting_write0

    @ 在栈上分配 16 字节工作区 (4 个字)
    sub     sp, sp, #16
    mov     r5, sp                  @ R5 = 工作区基址

    @ LDR Rd, [Rn, #imm5]: 立即数偏移 (0-124, 字对齐)
    @ 先把一些数据存入 RAM
    movs    r0, #0xAA
    str     r0, [r5, #0]            @ [R5+0]  = 0xAA
    movs    r0, #0xBB
    str     r0, [r5, #4]            @ [R5+4]  = 0xBB
    movs    r0, #0xCC
    str     r0, [r5, #8]            @ [R5+8]  = 0xCC
    movs    r0, #0xDD
    str     r0, [r5, #12]           @ [R5+12] = 0xDD

    @ 现在用各种寻址模式读取:

    @ 1. LDR Rd, [Rn, #imm]: 基址 + 立即数偏移
    ldr     r0, [r5, #0]            @ R0 = [R5+0] = 0xAA
    bl      semihosting_write_hex

    ldr     r0, [r5, #8]            @ R0 = [R5+8] = 0xCC
    bl      semihosting_write_hex

    @ 2. LDR Rd, [Rn, Rm]: 基址 + 寄存器偏移
    movs    r1, #4                  @ offset = 4
    ldr     r0, [r5, r1]            @ R0 = [R5+4] = 0xBB
    bl      semihosting_write_hex

    @ 3. STR Rd, [Rn, Rm]: 寄存器偏移存储
    movs    r0, #0x42
    movs    r1, #12                 @ 偏移 12
    str     r0, [r5, r1]            @ [R5+12] = 0x42

    @ 4. LDR Rd, [Rn, #imm]! : 前索引 (先加偏移, 再加载, 更新基址)
    @    在 Thumb-1 中 LDR/STR 不支持 ! (前索引) 格式!
    @    需要手动 ADD 实现:
    adds    r5, r5, #4              @ R5 += 4 (先更新基址)
    ldr     r0, [r5, #0]            @ R0 = [R5] (再加载)
    bl      semihosting_write_hex

@ ==============================================================================
@ 第二部分: 字节和半字操作
@ ==============================================================================
    ldr     r0, =msg_s2
    bl      semihosting_write0

    @ 重新设置 R5 指向栈工作区
    add     r5, sp, #0

    @ LDRB/STRB: 字节操作
    movs    r0, #0x41               @ 'A'
    strb    r0, [r5, #0]
    movs    r0, #0x42               @ 'B'
    strb    r0, [r5, #1]
    movs    r0, #0x43               @ 'C'
    strb    r0, [r5, #2]
    movs    r0, #0                  @ NUL
    strb    r0, [r5, #3]

    @ 输出字符串 (通过 semihosting)
    movs    r0, r5
    bl      semihosting_write0

    @ LDRB 读取单个字节 (结果为 0 扩展到 32 位)
    ldrb    r0, [r5, #0]            @ R0 = 'A' = 0x41
    bl      semihosting_write_hex

    @ LDRH/STRH: 半字操作 (16 位)
    movs    r0, #0xDE
    lsls    r1, r0, #8
    adds    r0, r0, r1              @ R0 = 0xDEDE (注意: 这只在简单情况工作)
    @ 实际上: ldr r0, =0xDEAD
    ldr     r0, =0xDEAD
    strh    r0, [r5, #4]
    ldrh    r0, [r5, #4]            @ R0 = 0x0000DEAD (0 扩展)
    bl      semihosting_write_hex

@ ==============================================================================
@ 第三部分: LDM/STM 多寄存器操作
@ ==============================================================================
    ldr     r0, =msg_s3
    bl      semihosting_write0

    @ STMIA: Store Multiple Increment After (存储后地址递增)
    @ 将 R0-R3 的值存入内存, 每次存完后地址+4
    @ Thumb-1 格式: STMIA Rn!, {reg_list}
    add     r5, sp, #0
    ldr     r0, =0xDEADBEEF
    ldr     r1, =0xCAFEBABE
    ldr     r2, =0x12345678
    ldr     r3, =0x87654321
    stmia   r5!, {r0, r1, r2, r3}   @ 存储 4 个字, R5 指向末尾

    @ LDMIA: Load Multiple Increment After (加载后地址递增)
    @ 从内存读取多个值
    add     r5, sp, #0              @ 重置 R5
    movs    r0, #0
    movs    r1, #0
    movs    r2, #0
    movs    r3, #0
    ldmia   r5!, {r0, r1, r2, r3}   @ 加载 4 个字

    @ 验证: R0 应该是 0xDEADBEEF
    bl      semihosting_write_hex  @ R0 = 0xDEADBEEF

    movs    r0, r3                  @ R3 = 0x87654321
    bl      semihosting_write_hex

    @ 重要! STM/LDM 在 Thumb-1 中:
    @   1. 寄存器列表只能包含低寄存器 (R0-R7)
    @   2. 基址寄存器 Rn 必须在列表中(或不在列表中均可)
    @   3. 寄存器按编号顺序存储 (R0 在最低地址)
    @   4. 总是使用 ! (写回基址)

@ ==============================================================================
@ 第四部分: PUSH/POP 栈操作
@ ==============================================================================
    ldr     r0, =msg_s4
    bl      semihosting_write0

    @ PUSH: 等价于 STMDB SP!, {regs} (先减 SP, 再存储)
    @   SP 总是 4 字节对齐, PUSH 后向低地址增长
    @ POP: 等价于 LDMIA SP!, {regs} (先加载, 再增 SP)

    @ 演示: 保存和恢复寄存器
    ldr     r0, =msg_push_test
    bl      semihosting_write0

    movs    r0, #111
    movs    r1, #222
    ldr     r2, =333            @ 333 > 255, 不能用 MOVS 立即数
    push    {r0, r1, r2}            @ SP -= 12, 存 R0,R1,R2 到栈
    @ R0-R2 现在在栈上, 可以随意修改
    movs    r0, #0
    movs    r1, #0
    movs    r2, #0
    pop     {r0, r1, r2}            @ 恢复, SP += 12
    @ R0=111, R1=222, R2=333

    bl      semihosting_write_hex  @ 应显示 0x0000006F (111)

    @ PUSH {lr}; ... POP {pc}: 标准子程序调用/返回模式
    ldr     r0, =msg_push_lr
    bl      semihosting_write0
    @ 这个模式在 04_stack_and_call.S 中详细讲解

@ ==============================================================================
@ 第五部分: PC-relative 加载和地址获取
@ ==============================================================================
    ldr     r0, =msg_s5
    bl      semihosting_write0

    @ LDR Rd, =expr: 伪指令, 汇编器生成 PC-relative 加载
    @ 汇编器在代码附近创建 "literal pool" 存储 32 位值
    ldr     r0, =0x12345678         @ 伪指令: 加载 32 位立即数
    bl      semihosting_write_hex  @ 0x12345678

    @ ADR Rd, label: 获取 label 的地址到 Rd
    @ 汇编器生成 ADD Rd, PC, #offset
    @ ADR 只能引用同一 section 的符号, 跨 section 用 LDR =
    ldr     r0, =vector_table       @ R0 = vector_table 的地址 (= 0)
    bl      semihosting_write_hex  @ 0x00000000

    @ 实际读取常量: 用 LDR Rd, =label 加载地址,
    @ 然后 LDR Rd, [Rd] 读取值
    ldr     r0, =test_value         @ R0 = test_value 的地址
    ldr     r0, [r0, #0]            @ R0 = *test_value = 0xBADC0DE
    bl      semihosting_write_hex

@ ==============================================================================
@ 清理和退出
@ ==============================================================================
    add     sp, sp, #16             @ 释放工作区

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
    .asciz  "\n=== Cortex-M0 Assembly: Memory Access ===\n"
msg_s1:
    .asciz  "\n[1] LDR/STR -- Word (32-bit) load/store\n"
msg_s2:
    .asciz  "\n[2] LDRB/STRB, LDRH/STRH -- Byte & Halfword\n"
msg_s3:
    .asciz  "\n[3] LDM/STM -- Multiple register load/store\n"
msg_s4:
    .asciz  "\n[4] PUSH/POP -- Stack operations\n"
msg_s5:
    .asciz  "\n[5] PC-relative load & ADR -- Address computation\n"
msg_push_test:
    .asciz  "  PUSH {r0,r1,r2} / POP {r0,r1,r2} test\n"
msg_push_lr:
    .asciz  "  Pattern: PUSH {lr} / POP {pc} for subroutines\n"
msg_done:
    .asciz  "\n=== Exercise 02 complete! ===\n"

    .align  4
test_value:
    .word   0xBADC0DE

    .end
