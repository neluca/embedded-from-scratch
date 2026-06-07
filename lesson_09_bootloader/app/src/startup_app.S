@ ==============================================================================
@ Application 启动代码 — 位于 Flash 0x00002000
@ ==============================================================================
    .syntax unified
    .cpu    cortex-m0
    .thumb

    .section .vector_table, "a"
    .align  2
    .global app_vector_table
    .type   app_vector_table, %object
app_vector_table:
    .word   __stack_top
    .word   app_reset_handler
    .word   default_handler
    .word   default_handler
    .space  (15 - 4 + 1) * 4
    .word   default_handler
    .space  32 * 4

    .section .text, "ax"
    .align  2
    .global app_reset_handler
    .thumb_func
    .type   app_reset_handler, %function
app_reset_handler:
    @ 复制 .data
    ldr     r0, =__data_load_start
    ldr     r1, =__data_start
    ldr     r2, =__data_end
    subs    r2, r2, r1
    beq     app_zero_bss
app_copy_data:
    ldrb    r3, [r0, #0]
    strb    r3, [r1, #0]
    adds    r0, r0, #1
    adds    r1, r1, #1
    subs    r2, r2, #1
    bne     app_copy_data
app_zero_bss:
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    subs    r1, r1, r0
    beq     app_call_main
app_zero_loop:
    movs    r2, #0
    strb    r2, [r0, #0]
    adds    r0, r0, #1
    subs    r1, r1, #1
    bne     app_zero_loop
app_call_main:
    bl      app_main
    b       .

    .weak   default_handler
    .thumb_func
    .type   default_handler, %function
default_handler:
    b       .

    .end
