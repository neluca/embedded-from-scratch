@ ==============================================================================
@ 01_registers_and_mov.s -- Cortex-M0 寄存器与数据处理指令
@ ==============================================================================
@
@ ARMv6-M (Thumb-1) 关键限制 (与 ARMv7-M 的重要区别):
@ 1. 仅低寄存器 R0-R7 能用于数据处理指令 (MOVS/ADDS/ANDS 等)
@ 2. 高寄存器 R8-R12 仅用于 MOV/ADD/CMP/BX
@ 3. 数据处理指令是两操作数形式: OP Rd, Rm (Rd = Rd OP Rm)
@ 4. 移位操作必须通过寄存器或 5 位立即数
@
@ 学习目标: 掌握寄存器操作、算术、逻辑、移位、比较指令
@ ==============================================================================

    .syntax unified
    .cpu    cortex-m0
    .thumb

@ ==============================================================================
@ 中断向量表
@ ==============================================================================
    .section .vector_table, "a"
    .align  2
    .global vector_table
    .type   vector_table, %object

vector_table:
    .word   __stack_top             @ [0]  初始 SP
    .word   reset_handler           @ [1]  Reset
    .word   default_handler         @ [2]  NMI
    .word   default_handler         @ [3]  HardFault
    .space  (14 - 4 + 1) * 4        @ [4-14] 保留槽位
    .word   default_handler         @ [15] SysTick
    .space  32 * 4                  @ [16-47] 外设中断

@ ==============================================================================
@ Semihosting 辅助函数
@ ==============================================================================
    .section .text, "ax"
    .align  2

@ semihosting_write0 -- 通过 SYS_WRITE0 输出 NUL 结尾字符串
@ 输入: R0 = 字符串地址
semihosting_write0:
    movs    r1, r0          @ R1 = 参数指针 (字符串本身)
    movs    r0, #0x04       @ R0 = SYS_WRITE0
    bkpt    #0xAB
    bx      lr

@ semihosting_write_hex -- 输出 32 位值的 hex 表示
@ 输入: R0 = value
@ 栈帧: [sp+0..10] = "0xXXXXXXXX\0"
semihosting_write_hex:
    push    {r4, lr}
    sub     sp, sp, #12
    mov     r4, sp              @ R4 = 缓冲区基址

    @ 前缀 "0x"
    movs    r1, #'0'
    strb    r1, [r4, #0]
    movs    r1, #'x'
    strb    r1, [r4, #1]

    @ 从低到高填充 8 个 hex 数字
    @ R2 = 源值副本, R3 = 循环计数器
    movs    r3, #8              @ 8 个 nibble

1:  subs    r3, r3, #1          @ R3--
    @ 提取当前 nibble
    movs    r2, r0
    movs    r1, #0x0F
    ands    r2, r1              @ R2 = 低 4 位 (Thumb-1: ANDS Rd, Rm)
    @ 转 ASCII
    cmp     r2, #10
    blt     2f
    adds    r2, r2, #('A' - 10 - '0')
2:  adds    r2, r2, #'0'
    @ 存入 buf[2 + r3]
    adds    r1, r4, r3
    adds    r1, r1, #2
    strb    r2, [r1, #0]
    @ 右移 4 位处理下一个 nibble
    lsrs    r0, r0, #4
    cmp     r3, #0
    bne     1b

    @ NUL 终止 + 换行
    movs    r1, #0
    strb    r1, [r4, #10]
    movs    r1, #'\n'
    strb    r1, [r4, #11]

    @ 输出 (不含换行)
    movs    r1, #0
    strb    r1, [r4, #10]
    movs    r0, r4
    bl      semihosting_write0

    @ 输出换行
    movs    r1, #'\n'
    strb    r1, [r4, #0]
    movs    r1, #0
    strb    r1, [r4, #1]
    movs    r0, r4
    bl      semihosting_write0

    add     sp, sp, #12
    pop     {r4, pc}

@ default_handler -- 未配置的异常处理
default_handler:
    b       .

@ ==============================================================================
@ reset_handler -- 主程序
@ ==============================================================================
    .global reset_handler
    .type   reset_handler, %function
reset_handler:

    ldr     r0, =msg_title
    bl      semihosting_write0

@ ==============================================================================
@ 第一部分: 寄存器与 MOV 指令
@ ==============================================================================
@ Cortex-M0 寄存器组:
@   R0-R7:   低寄存器 (所有指令可用)
@   R8-R12:  高寄存器 (仅 MOV/ADD/CMP/BX 可用)
@   R13/SP:  栈指针 (banked: MSP/PSP)
@   R14/LR:  链接寄存器 (BL 自动保存返回地址)
@   R15/PC:  程序计数器 (bit[0] 指示 Thumb 状态)
@   xPSR:    程序状态寄存器 (N-Z-C-V-Q + 异常号)
@
@ MOVS Rd, #imm8: 加载 8 位立即数到低寄存器
@ MVNS Rd, Rm:    加载 Rm 按位取反的值

    ldr     r0, =msg_s1
    bl      semihosting_write0

    movs    r0, #42         @ R0 = 42, 标志位: N=0 Z=0
    bl      semihosting_write_hex

    movs    r0, #0          @ R0 = 0, 标志位: N=0 Z=1
    bl      semihosting_write_hex

    @ MVNS: 按位取反
    movs    r1, #0
    mvns    r1, r1          @ R1 = 0xFFFFFFFF (M0 用 MVNS 而非 MOV.W #-1)
    movs    r0, r1
    bl      semihosting_write_hex

@ ==============================================================================
@ 第二部分: 算术运算
@ ==============================================================================
    ldr     r0, =msg_s2
    bl      semihosting_write0

    @ ADD/ADDS:  加法 (M0 中 ADD 和 ADDS 是同一指令)
    @ SUB/SUBS:  减法
    movs    r0, #100
    adds    r0, r0, #50     @ R0 = 150, Thumb-1: ADDS Rd, #imm8
    bl      semihosting_write_hex

    movs    r0, #200
    subs    r0, r0, #75     @ R0 = 125
    bl      semihosting_write_hex

    @ ADC: 带进位加法 (用于 64 位运算)
    @ 计算 0x12345678 + 0x87654321:
    @ Thumb-1: ADCS Rd, Rm  (Rd = Rd + Rm + C)
    ldr     r0, =0x12345678
    ldr     r1, =0x87654321
    movs    r2, r0
    adds    r2, r1          @ R2 = R0 + R1 = 0x99999999 (无进位)
    @ 演示 64 位加法: 0xFFFFFFFF00000000 + 0x0000000200000000
    ldr     r0, =0x00000000     @ 低 32 位
    ldr     r1, =0xFFFFFFFF     @ 高 32 位
    ldr     r2, =0x00000000     @ 加数低 32 位
    ldr     r3, =0x00000002     @ 加数高 32 位
    adds    r0, r0, r2          @ R0 = 低32位和, 更新 C
    movs    r4, r1
    adcs    r4, r3              @ R4 = 高32位和 + C (Thumb-1: ADCS Rd, Rm)
    @ 结果: 高32位=R4, 低32位=R0

    @ MULS: 32×32→32 乘法 (仅保留低 32 位)
    @ 注意: M0 无硬件除法! UDIV/SDIV 是 ARMv7-M 指令
    movs    r0, #6
    movs    r1, #7
    muls    r0, r1              @ R0 = 6 * 7 = 42
    bl      semihosting_write_hex

@ ==============================================================================
@ 第三部分: 逻辑运算
@ ==============================================================================
    ldr     r0, =msg_s3
    bl      semihosting_write0

    @ Thumb-1 逻辑指令格式: OP Rd, Rm  (Rd = Rd OP Rm)
    @ 所有操作数必须是低寄存器 (R0-R7)

    ldr     r0, =0xFF00FF00
    ldr     r1, =0xF0F0F0F0

    @ ANDS: 按位与
    movs    r2, r0          @ R2 = R0
    ands    r2, r1          @ R2 = R2 & R1 = 0xF000F000 (Thumb-1: ANDS Rd,Rm)

    @ ORRS: 按位或
    movs    r3, r0
    orrs    r3, r1          @ R3 = R3 | R1 = 0xFFF0FFF0

    @ EORS: 按位异或
    movs    r4, r0
    eors    r4, r1          @ R4 = R4 ^ R1 = 0x0F100F10

    @ BICS: 按位清零 (Rd = Rd & ~Rm)
    @ 在 M0 中用于快速掩码操作
    ldr     r0, =0x0FF0
    ldr     r1, =0x00F0
    movs    r2, r0
    bics    r2, r1          @ R2 = 0x0F00

@ ==============================================================================
@ 第四部分: 移位操作
@ ==============================================================================
    ldr     r0, =msg_s4
    bl      semihosting_write0

    @ LSLS Rd, Rm, #imm5: 逻辑左移 (低位补 0)
    movs    r0, #1
    lsls    r0, r0, #4      @ R0 = 16 (0x10)

    @ LSRS Rd, Rm, #imm5: 逻辑右移 (高位补 0)
    movs    r0, #0x80
    lsrs    r0, r0, #3      @ R0 = 0x10

    @ ASRS Rd, Rm, #imm5: 算术右移 (高位补符号位)
    @ 正数等价于 LSRS, 负数保持符号
    ldr     r0, =0xFFFFFF00 @ R0 = -256
    asrs    r0, r0, #4      @ R0 = -16 (0xFFFFFFF0)

    @ RORS: 循环右移
    @ Thumb-1 没有 RORS Rd, Rm, #imm !
    @ 必须使用 RORS Rd, Rs (Rs 含移位量) 或 MOV+ror 编码
    ldr     r0, =0x80000001
    movs    r1, #1          @ 移位量 = 1
    rors    r0, r1          @ R0 循环右移 1 位 (Thumb-1: RORS Rd,Rs)

@ ==============================================================================
@ 第五部分: 比较与测试
@ ==============================================================================
    ldr     r0, =msg_s5
    bl      semihosting_write0

    @ CMP Rn, Rm: 比较 (计算 Rn - Rm, 只更新标志位, 丢弃结果)
    movs    r0, #10
    movs    r1, #20
    cmp     r0, r1          @ 10 - 20: N=1(负), Z=0(非零), C=0(借位)

    @ CMN Rn, Rm: 比较负数 (计算 Rn + Rm, 只更新标志位)
    movs    r0, #42
    cmn     r0, r0          @ 42 + 42 = 84, 非零, Z=0

    @ TST Rn, Rm: 位测试 (计算 Rn & Rm, 只更新标志位)
    @ 常用于测试特定位
    movs    r0, #0x55       @ 0101 0101
    movs    r1, #0x04       @ 0000 0100
    tst     r0, r1          @ 测试 R0 的 bit2: 0x55 & 0x04 = 0x04, Z=0

    @ 通过 CMP 和条件分支实现 if-then-else
    movs    r0, #5
    movs    r1, #10
    cmp     r0, r1
    blt     1f              @ 如果 R0 < R1, 跳转
    @ 否则 (R0 >= R1) 到此处
1:  @ R0 < R1 到此处

@ ==============================================================================
@ 演示结束
@ ==============================================================================
    ldr     r0, =msg_done
    bl      semihosting_write0

    @ SYS_EXIT
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
    .asciz  "\n=== Cortex-M0 Assembly: Registers & Data Processing ===\n"
msg_s1:
    .asciz  "\n[1] MOV/MOVS/MVNS -- Register operations\n"
msg_s2:
    .asciz  "\n[2] ADD/SUB/ADC/SBC/MULS -- Arithmetic\n"
msg_s3:
    .asciz  "\n[3] AND/ORR/EOR/BIC -- Logical\n"
msg_s4:
    .asciz  "\n[4] LSL/LSR/ASR/ROR -- Shift\n"
msg_s5:
    .asciz  "\n[5] CMP/CMN/TST -- Compare & Test\n"
msg_done:
    .asciz  "\n=== Exercise 01 complete! ===\n"

    .end
