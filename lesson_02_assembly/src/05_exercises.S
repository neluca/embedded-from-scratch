@ ==============================================================================
@ 05_exercises.S -- Cortex-M0 汇编综合练习
@ ==============================================================================
@
@ 综合运用前 4 课的知识, 实现常见算法:
@ 1. memcpy  -- 内存块复制
@ 2. strlen  -- 字符串长度
@ 3. delay_loop -- 软件延时循环
@ 4. fibonacci -- 斐波那契数列
@ 5. bubble_sort -- 冒泡排序
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
@ 练习 1: my_memcpy -- 内存块复制
@ 等效于 C: void *memcpy(void *dst, const void *src, size_t n)
@ ==============================================================================
@ 输入: R0=dst, R1=src, R2=byte_count
@ 输出: R0=dst (原样返回)
@
@ Thumb-1 关键约束:
@   - LDR/STR 的 Rt (数据寄存器) 必须是低寄存器 R0-R7
@   - CBZ/CBNZ 在某些 GAS 版本对 cortex-m0 不可用, 用 CMP+BEQ/BNE 替代
@   - 为了字复制使用 R5 作为临时寄存器
my_memcpy:
    push    {r4, r5, lr}
    movs    r4, r0              @ R4 = dst (保存返回值)

    @ 先按字复制 (4 字节一组), 使用 R5 做临时
    lsrs    r3, r2, #2          @ R3 = word_count = byte_count / 4
    cmp     r3, #0              @ if word_count == 0, skip word copy
    beq     2f

1:  ldr     r5, [r1, #0]       @ 读一个字 (R5 = *src)
    str     r5, [r0, #0]       @ 写一个字 (*dst = R5)
    adds    r0, r0, #4          @ dst += 4
    adds    r1, r1, #4          @ src += 4
    subs    r3, r3, #1          @ word_count--
    cmp     r3, #0
    bne     1b

2:  @ 处理剩余字节 (0-3 字节): byte_count & 3
    movs    r3, #3
    ands    r2, r3              @ R2 = byte_count & 3
    cmp     r2, #0
    beq     4f

3:  ldrb    r3, [r1, #0]        @ 读 1 字节
    strb    r3, [r0, #0]        @ 写 1 字节
    adds    r0, r0, #1
    adds    r1, r1, #1
    subs    r2, r2, #1
    cmp     r2, #0
    bne     3b

4:  movs    r0, r4              @ 返回 dst
    pop     {r4, r5, pc}

@ ==============================================================================
@ 练习 2: my_strlen -- 计算字符串长度
@ 等效于 C: size_t strlen(const char *s)
@ ==============================================================================
@ 输入: R0 = 字符串地址
@ 输出: R0 = 长度 (不含 NUL)
my_strlen:
    movs    r1, r0              @ R1 = 字符串指针
    movs    r0, #0              @ count = 0

1:  ldrb    r2, [r1, #0]
    cmp     r2, #0              @ Thumb-1: CMP+BEQ 代替 CBZ
    beq     2f                  @ 如果 '\0', 返回
    adds    r0, r0, #1          @ count++
    adds    r1, r1, #1          @ ptr++
    b       1b
2:  bx      lr

@ ==============================================================================
@ 练习 3: delay_loop -- 软件延时循环
@ ==============================================================================
@ 输入: R0 = 延时计数 (越大延时越长)
@ 注意: 在真实系统中应使用硬件定时器, 此处仅为教学演示
@ M0 @ 16MHz: ~3 cycles per inner loop -> 约 5.3us/count
delay_loop:
    push    {r4}
1:  movs    r4, #100            @ 内部循环 100 次 (微调延时精度)
2:  subs    r4, r4, #1
    bne     2b
    subs    r0, r0, #1
    bne     1b
    pop     {r4}
    bx      lr

@ ==============================================================================
@ 练习 4: fibonacci -- 计算第 n 个斐波那契数
@ F(0)=0, F(1)=1, F(n)=F(n-1)+F(n-2)
@ ==============================================================================
@ 输入: R0 = n (n >= 0)
@ 输出: R0 = F(n)
fibonacci:
    cmp     r0, #0
    bne     1f
    @ F(0) = 0
    bx      lr                  @ R0 already 0

1:  cmp     r0, #1
    bne     2f
    @ F(1) = 1
    @ R0 already 1
    bx      lr

2:  push    {r4, r5, lr}
    movs    r4, r0              @ R4 = n
    movs    r5, #0              @ F(0) = 0
    movs    r0, #1              @ F(1) = 1

    @ 迭代: for i=2 to n: next = a + b; a = b; b = next
    @ 保持 R5 = F(i-2), R0 = F(i-1)
3:  subs    r4, r4, #1          @ i++
    cmp     r4, #1
    beq     4f                  @ done

    @ R0 = F(i-1), R5 = F(i-2)
    adds    r1, r0, r5          @ next = F(i-1) + F(i-2)
    movs    r5, r0              @ F(i-2) = F(i-1)
    movs    r0, r1              @ F(i-1) = next
    b       3b

4:  pop     {r4, r5, pc}

@ ==============================================================================
@ 练习 5: bubble_sort -- 冒泡排序
@ ==============================================================================
@ 输入: R0 = 数组地址, R1 = 元素个数
@ 输出: 数组原地排序
bubble_sort:
    push    {r4, r5, r6, r7, lr}    @ R7 代替 R12 做临时变量

    @ 外层循环: for i = 0; i < n-1; i++
    subs    r1, r1, #1          @ n = n-1
    beq     4f                  @ 只有 1 个元素, 无需排序

    movs    r4, #0              @ i = 0
1:  @ 内层循环: for j = 0; j < n-1-i; j++
    subs    r6, r1, r4          @ R6 = n-1-i (内层循环次数)
    beq     3f                  @ 无元素需要比较

    movs    r5, #0              @ j = 0
    mov     r2, r0              @ ptr = array base (重置)

2:  ldr     r3, [r2, #0]        @ a = arr[j]
    ldr     r7, [r2, #4]        @ b = arr[j+1]
    cmp     r3, r7
    ble     2f                  @ if a <= b, skip swap
    @ swap arr[j] and arr[j+1]
    str     r7, [r2, #0]
    str     r3, [r2, #4]
2:  adds    r2, r2, #4          @ ptr++ (move to next element)
    adds    r5, r5, #1          @ j++
    cmp     r5, r6
    blt     2b                  @ continue inner loop

3:  adds    r4, r4, #1          @ i++
    cmp     r4, r1
    blt     1b                  @ continue outer loop

4:  pop     {r4, r5, r6, r7, pc}

@ ==============================================================================
@ reset_handler
@ ==============================================================================
    .global reset_handler
    .type   reset_handler, %function
reset_handler:

    ldr     r0, =msg_title
    bl      semihosting_write0

@ ==============================================================================
@ 测试: my_memcpy
@ ==============================================================================
    ldr     r0, =msg_ex1
    bl      semihosting_write0

    @ 分配源和目标缓冲区 (在栈上)
    sub     sp, sp, #32
    mov     r5, sp              @ R5 = 栈工作区

    @ 填充源数据
    ldr     r0, =0xDEADBEEF
    str     r0, [r5, #0]
    ldr     r0, =0xCAFEBABE
    str     r0, [r5, #4]
    movs    r0, #0x41           @ 'A'
    strb    r0, [r5, #8]
    movs    r0, #0x42           @ 'B'
    strb    r0, [r5, #9]
    movs    r0, #0
    strb    r0, [r5, #10]

    @ 复制 8 字节到偏移 16 处
    movs    r0, r5
    adds    r0, r0, #16         @ dst = buf+16
    movs    r1, r5              @ src = buf+0
    movs    r2, #8              @ 8 bytes
    bl      my_memcpy

    @ 验证: 读取目标并输出
    ldr     r0, [r5, #16]       @ 第一个字: 0xDEADBEEF
    bl      semihosting_write_hex
    ldr     r0, [r5, #20]       @ 第二个字: 0xCAFEBABE
    bl      semihosting_write_hex
    ldrb    r0, [r5, #24]       @ 'A' = 0x41
    bl      semihosting_write_hex

@ ==============================================================================
@ 测试: my_strlen
@ ==============================================================================
    ldr     r0, =msg_ex2
    bl      semihosting_write0

    ldr     r0, =test_str
    bl      my_strlen           @ R0 = 13 ("Hello, World!")
    bl      semihosting_write_hex  @ 0x0000000D (13)

@ ==============================================================================
@ 测试: fibonacci
@ ==============================================================================
    ldr     r0, =msg_ex3
    bl      semihosting_write0

    @ F(10) = 55
    movs    r0, #10
    bl      fibonacci
    bl      semihosting_write_hex  @ 0x00000037 (55)

    @ F(0) = 0
    movs    r0, #0
    bl      fibonacci
    bl      semihosting_write_hex  @ 0x00000000

    @ F(20) = 6765
    movs    r0, #20
    bl      fibonacci
    bl      semihosting_write_hex  @ 0x00001A6D (6765)

@ ==============================================================================
@ 测试: bubble_sort
@ ==============================================================================
    ldr     r0, =msg_ex4
    bl      semihosting_write0

    @ 在栈上创建测试数组: [5, 2, 8, 1, 9, 3]
    sub     sp, sp, #24
    mov     r6, sp

    movs    r0, #5
    str     r0, [r6, #0]
    movs    r0, #2
    str     r0, [r6, #4]
    movs    r0, #8
    str     r0, [r6, #8]
    movs    r0, #1
    str     r0, [r6, #12]
    movs    r0, #9
    str     r0, [r6, #16]
    movs    r0, #3
    str     r0, [r6, #20]

    @ 输出排序前
    ldr     r0, =msg_before
    bl      semihosting_write0

    @ 排序
    movs    r0, r6
    movs    r1, #6
    bl      bubble_sort

    @ 输出排序后
    ldr     r0, =msg_after
    bl      semihosting_write0

    @ 逐一输出排序结果
    ldr     r0, [r6, #0]
    bl      semihosting_write_hex  @ 1
    ldr     r0, [r6, #4]
    bl      semihosting_write_hex  @ 2
    ldr     r0, [r6, #8]
    bl      semihosting_write_hex  @ 3
    ldr     r0, [r6, #12]
    bl      semihosting_write_hex  @ 5
    ldr     r0, [r6, #16]
    bl      semihosting_write_hex  @ 8
    ldr     r0, [r6, #20]
    bl      semihosting_write_hex  @ 9

    add     sp, sp, #56         @ 释放所有栈空间

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
    .asciz  "\n=== Cortex-M0 Assembly: Exercises ===\n"
msg_ex1:
    .asciz  "\n[1] my_memcpy -- Memory copy (word + byte)\n"
msg_ex2:
    .asciz  "\n[2] my_strlen -- String length\n"
msg_ex3:
    .asciz  "\n[3] fibonacci -- Fibonacci sequence\n"
msg_ex4:
    .asciz  "\n[4] bubble_sort -- Array sorting\n"
msg_before:
    .asciz  "  Before: [5, 2, 8, 1, 9, 3]\n"
msg_after:
    .asciz  "  After: "
msg_done:
    .asciz  "\n=== Exercise 05 complete! ===\n"

test_str:
    .asciz  "Hello, World!"

    .end
