@ ==============================================================================
@ startup.S — Lesson 13 启动代码 (带 SWI 中断处理)
@ ==============================================================================
    .syntax unified
    .cpu    cortex-m0
    .thumb

    .section .vector_table, "a"
    .align  2
    .global vector_table
    .type   vector_table, %object
vector_table:
    .word   __stack_top
    .word   reset_handler
    .word   default_handler     @ NMI
    .word   hardfault_handler   @ HardFault
    .space  (11 - 4 + 1) * 4
    .word   svcall_handler
    .space  2 * 4
    .word   pendsv_handler
    .word   systick_handler
    @ IRQ0-19: default
    .space  20 * 4
    @ IRQ20-23: SWI0-SWI3
    .word   swi0_handler
    .word   swi1_handler
    .word   swi2_handler
    .word   swi3_handler
    @ IRQ24-31: default
    .space  8 * 4

    .section .text, "ax"
    .align  2
    .global reset_handler
    .thumb_func
    .type   reset_handler, %function
reset_handler:
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

    .weak   default_handler
    .thumb_func
    .type   default_handler, %function
default_handler:
    b       .

    .weak   hardfault_handler
    .thumb_set hardfault_handler, default_handler
    .weak   svcall_handler
    .thumb_set svcall_handler, default_handler
    .weak   pendsv_handler
    .thumb_set pendsv_handler, default_handler
    .weak   systick_handler
    .thumb_set systick_handler, default_handler

    @ SWI handlers — jump to C functions
    .global swi0_handler
    .thumb_func
    .type   swi0_handler, %function
swi0_handler:
    push    {lr}
    bl      swi0_handler_c
    pop     {pc}

    .global swi1_handler
    .thumb_func
    .type   swi1_handler, %function
swi1_handler:
    push    {lr}
    bl      swi1_handler_c
    pop     {pc}

    .global swi2_handler
    .thumb_func
    .type   swi2_handler, %function
swi2_handler:
    push    {lr}
    bl      swi2_handler_c
    pop     {pc}

    .global swi3_handler
    .thumb_func
    .type   swi3_handler, %function
swi3_handler:
    push    {lr}
    bl      swi3_handler_c
    pop     {pc}

    .end
