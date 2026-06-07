@ ==============================================================================
@ startup.S -- Cortex-M0 启动代码 (带真实中断处理函数)
@ ==============================================================================
@
@ 与 Lesson 3 的 startup.S 相比, 这个版本:
@   1. 为 SysTick, PendSV, SVCall, HardFault 提供了真实处理函数
@   2. 展示了异常入口/退出时硬件自动压栈/出栈的机制
@   3. HardFault 处理函数调用 C 函数来分析栈帧
@   4. 为 FreeRTOS (Lesson 7) 打下了基础
@
@ Cortex-M0 异常入口时的硬件行为 (EXC_ENTRY):
@   1. 自动压栈 8 个字: xPSR, PC, LR, R12, R3, R2, R1, R0
@   2. 从向量表读取异常处理函数地址 → PC
@   3. 更新 LR 为 EXC_RETURN 值 (0xFFFFFFF9 或 0xFFFFFFFD)
@
@ 异常退出时的硬件行为 (EXC_RETURN):
@   - 执行 BX LR (LR = EXC_RETURN 值)
@   - 硬件自动出栈 8 个字: R0, R1, R2, R3, R12, LR, PC, xPSR
@ ==============================================================================

    .syntax unified
    .cpu    cortex-m0
    .thumb

@ ==============================================================================
@ 中断向量表 (带真实处理函数)
@ ==============================================================================
    .section .vector_table, "a"
    .align  2
    .global vector_table
    .type   vector_table, %object

vector_table:
    .word   __stack_top             @ [0]  初始 SP
    .word   reset_handler           @ [1]  Reset
    .word   nmi_handler             @ [2]  NMI
    .word   hardfault_handler       @ [3]  HardFault (真实处理函数!)
    .space  (11 - 4 + 1) * 4        @ [4-11] 保留
    .word   svcall_handler          @ [11] SVCall (FreeRTOS 用它启动第一个任务)
    .space  2 * 4                   @ [12-13] 保留
    .word   pendsv_handler          @ [14] PendSV (FreeRTOS 上下文切换!)
    .word   systick_handler         @ [15] SysTick (系统节拍!)
    .space  32 * 4                  @ [16-47] 外设中断

@ ==============================================================================
@ reset_handler -- 系统复位入口
@ ==============================================================================
    .section .text, "ax"
    .align  2
    .global reset_handler
    .type   reset_handler, %function

reset_handler:
    @ 复制 .data 段 (已初始化数据)
    ldr     r0, =__data_load_start
    ldr     r1, =__data_start
    ldr     r2, =__data_end
    subs    r2, r2, r1
    beq     zero_bss

copy_data:
    ldrb    r3, [r0, #0]
    strb    r3, [r1, #0]
    adds    r0, r0, #1
    adds    r1, r1, #1
    subs    r2, r2, #1
    bne     copy_data

zero_bss:
    @ 清零 .bss 段
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    subs    r1, r1, r0
    beq     call_main

zero_loop:
    movs    r2, #0
    strb    r2, [r0, #0]
    adds    r0, r0, #1
    subs    r1, r1, #1
    bne     zero_loop

call_main:
    bl      main
    b       .

@ ==============================================================================
@ 默认异常处理函数 -- 死循环 (用 .weak 允许被 C 覆盖)
@ ==============================================================================
    .weak   default_handler
    .type   default_handler, %function
default_handler:
    b       .

    .weak   nmi_handler
    .thumb_set nmi_handler, default_handler

@ ==============================================================================
@ hardfault_handler -- HardFault 异常处理 (汇编部分)
@ ==============================================================================
@ 当发生 HardFault 时:
@   1. 硬件自动压栈 8 个字
@   2. SP 指向栈帧 (stack frame)
@   3. LR = EXC_RETURN (0xFFFFFFF9 = Thread+MSP, 0xFFFFFFFD = Thread+PSP)
@
@ 此函数将栈帧指针传给 C 函数 hardfault_handler_c(uint32_t *stack_frame)
@ stack_frame[0]=R0, [1]=R1, [2]=R2, [3]=R3,
@               [4]=R12, [5]=LR, [6]=PC(fault), [7]=xPSR
@ ==============================================================================
    .global hardfault_handler
    .thumb_func
    .type   hardfault_handler, %function
hardfault_handler:
    @ R0 = SP (栈帧指针) 作为 C 函数参数
    @ Thumb-1: MOV (not MOVS) for SP→low register transfer
    mov     r0, sp                  @ R0 = 栈帧地址
    bl      hardfault_handler_c     @ 调用 C 处理函数
    @ C 函数返回后, 如果无法恢复, 进入死循环
    b       .

@ ==============================================================================
@ svcall_handler -- SVC (Supervisor Call) 处理
@ ==============================================================================
@ FreeRTOS 使用 SVCall 启动第一个任务:
@   SVC 0 指令触发此处理函数
@   处理函数从异常栈帧中获取目标任务的 SP 和 PC
@   修改栈帧以切换到第一个任务
@ ==============================================================================
    .global svcall_handler
    .thumb_func
    .type   svcall_handler, %function
svcall_handler:
    @ 目前只是记录调用并返回
    push    {lr}
    bl      svcall_handler_c       @ 调用 C 处理函数
    pop     {pc}

@ ==============================================================================
@ pendsv_handler -- PendSV (可悬起系统调用) 处理
@ ==============================================================================
@ FreeRTOS 使用 PendSV 进行上下文切换
@
@ 注意: Handler 内部调用 semihosting 可能导致 QEMU 状态异常.
@ 为安全起见, 此 handler 只设置标志位, 不调用 semihosting.
@ 真实的 FreeRTOS PendSV handler 也不会有 I/O 调用.
@ ==============================================================================
    .global pendsv_handler
    .thumb_func
    .type   pendsv_handler, %function
pendsv_handler:
    @ 在 M0 中, 异常处理函数可以安全地使用 R0-R3, R12 (硬件已保存)
    @ 这里我们不调用 C 函数, 直接用汇编设置全局标志
    ldr     r0, =g_pendsv_triggered
    movs    r1, #1
    str     r1, [r0, #0]           @ g_pendsv_triggered = 1
    ldr     r0, =g_pendsv_count
    ldr     r1, [r0, #0]
    adds    r1, r1, #1
    str     r1, [r0, #0]           @ g_pendsv_count++
    bx      lr                     @ 异常返回 (LR = EXC_RETURN)

@ ==============================================================================
@ systick_handler -- SysTick 定时器中断处理
@ ==============================================================================
@ SysTick 是 Cortex-M 内建的 24 位递减定时器.
@ 在 FreeRTOS 中用作系统节拍 (tick interrupt).
@
@ 配置步骤:
@   1. 设置 SYST_RVR (重载值)
@   2. 清零 SYST_CVR (当前值)
@   3. 设置 SYST_CSR (使能, 中断使能, 时钟源)
@ ==============================================================================
    .global systick_handler
    .thumb_func
    .type   systick_handler, %function
systick_handler:
    push    {lr}
    bl      systick_handler_c      @ 调用 C 处理函数
    pop     {pc}

    .end
