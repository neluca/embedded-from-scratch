@ ==============================================================================
@ startup.S -- ARM Cortex-M0 汇编启动代码
@ ==============================================================================
@
@ 这是嵌入式系统中最底层的代码, 系统上电后首先执行.
@ 相比 Lesson 1 的 startup.c, 汇编版本更贴近硬件, 更透明.
@
@ 启动流程:
@   1. 硬件从向量表[0]加载 SP, 从[1]加载 PC → 跳转到 reset_handler
@   2. reset_handler: 复制 .data 段, 清零 .bss, 调用 main()
@   3. 如果 main() 返回, 进入安全死循环
@
@ 与 C 版本的区别:
@   - C 版本: 编译器自动生成函数序言/尾声 (push/pop, 栈帧)
@   - 汇编版本: 完全控制每条指令, 但必须手动管理 AAPCS 约定
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
    .word   __stack_top             @ [0]  初始 SP (硬件自动加载)
    .word   reset_handler           @ [1]  Reset
    .word   default_handler         @ [2]  NMI
    .word   hardfault_handler       @ [3]  HardFault
    .space  (11 - 4 + 1) * 4        @ [4-11] 保留
    .word   svcall_handler          @ [11] SVCall (FreeRTOS 入口)
    .space  2 * 4                   @ [12-13] 保留
    .word   pendsv_handler          @ [14] PendSV (FreeRTOS 上下文切换)
    .word   systick_handler         @ [15] SysTick
    .space  32 * 4                  @ [16-47] 外设中断

@ ==============================================================================
@ 链接脚本中定义的符号 (导入)
@ ==============================================================================
@ 注意: 这些是符号地址, 不是变量
@ 使用方式: ldr r0, =__data_start (把地址值加载到 r0)

@ ==============================================================================
@ reset_handler -- 复位入口
@ ==============================================================================
    .section .text, "ax"
    .align  2
    .global reset_handler
    .type   reset_handler, %function

reset_handler:
    @ 硬件启动后 SP 已从向量表[0]自动加载, 无需手动设置

    @ -------------------------------------------------------------------
    @ 第 1 步: 复制 .data 段 (已初始化数据) 从 Flash 到 RAM
    @ .data 段的 LMA (加载地址) 在 Flash, VMA (运行地址) 在 RAM
    @ -------------------------------------------------------------------
    ldr     r0, =__data_load_start  @ R0 = Flash 源地址 (LMA)
    ldr     r1, =__data_start       @ R1 = RAM 目标地址 (VMA)
    ldr     r2, =__data_end         @ R2 = RAM 结束地址
    subs    r2, r2, r1              @ R2 = .data 段大小 (字节数)
    beq     zero_bss                @ 如果 .data 为空, 跳过复制

copy_data:
    ldrb    r3, [r0, #0]            @ 从 Flash 读 1 字节
    strb    r3, [r1, #0]            @ 写到 RAM
    adds    r0, r0, #1              @ src++
    adds    r1, r1, #1              @ dst++
    subs    r2, r2, #1              @ count--
    bne     copy_data

    @ -------------------------------------------------------------------
    @ 第 2 步: 清零 .bss 段 (未初始化数据)
    @ C 标准规定静态/全局未初始化变量必须为 0
    @ -------------------------------------------------------------------
zero_bss:
    ldr     r0, =__bss_start        @ R0 = BSS 起始
    ldr     r1, =__bss_end          @ R1 = BSS 结束
    subs    r1, r1, r0              @ R1 = BSS 大小
    beq     call_main               @ 如果 BSS 为空, 跳到 main

zero_loop:
    movs    r2, #0
    strb    r2, [r0, #0]            @ 写 0
    adds    r0, r0, #1              @ ptr++
    subs    r1, r1, #1              @ count--
    bne     zero_loop

    @ -------------------------------------------------------------------
    @ 第 3 步: 调用 C 语言 main() 函数
    @ 按照 AAPCS, main() 没有参数, 不需要在 R0-R3 中传参
    @ -------------------------------------------------------------------
call_main:
    bl      main                    @ 跳转到 C 世界的入口!

    @ -------------------------------------------------------------------
    @ 第 4 步: main() 不应该返回, 如果返回了进入死循环
    @ -------------------------------------------------------------------
    b       .

@ ==============================================================================
@ 异常处理函数
@ ==============================================================================
@ 使用 .weak 使这些函数可被 C 代码覆盖
@ .thumb_set 创建别名

    .weak   default_handler
    .type   default_handler, %function
default_handler:
    b       .                       @ 死循环

    .weak   hardfault_handler
    .thumb_set hardfault_handler, default_handler

    .weak   svcall_handler
    .thumb_set svcall_handler, default_handler

    .weak   pendsv_handler
    .thumb_set pendsv_handler, default_handler

    .weak   systick_handler
    .thumb_set systick_handler, default_handler

    .end
