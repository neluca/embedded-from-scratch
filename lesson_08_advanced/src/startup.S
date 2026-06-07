@ Lesson 8 startup
    .syntax unified
    .cpu cortex-m0
    .thumb
    .section .vector_table,"a"; .align 2; .global vector_table; .type vector_table,%object
vector_table:
    .word __stack_top; .word reset_handler
    .word default_handler; .word default_handler
    .space (15-4+1)*4; .word default_handler; .space 32*4
    .section .text,"ax"; .align 2
    .global reset_handler; .thumb_func; .type reset_handler,%function
reset_handler:
    ldr r0,=__data_load_start; ldr r1,=__data_start
    ldr r2,=__data_end; subs r2,r2,r1; beq zero_bss
copy_data:
    ldrb r3,[r0,#0]; strb r3,[r1,#0]; adds r0,r0,#1
    adds r1,r1,#1; subs r2,r2,#1; bne copy_data
zero_bss:
    ldr r0,=__bss_start; ldr r1,=__bss_end
    subs r1,r1,r0; beq call_main
zero_loop:
    movs r2,#0; strb r2,[r0,#0]; adds r0,r0,#1
    subs r1,r1,#1; bne zero_loop
call_main:
    bl main; b .
    .weak default_handler; .thumb_func; .type default_handler,%function
default_handler: b .
    .end
