/*
 * =============================================================================
 * ARM Cortex-M0 (microbit / nRF51822) 链接脚本
 * =============================================================================
 *
 * 内存布局:
 *   Flash: 0x00000000 - 0x0003FFFF (256KB)  代码和只读数据
 *   RAM:   0x20000000 - 0x20003FFF (16KB)   读写数据、BSS、栈
 */

MEMORY
{
    flash (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    ram   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

__stack_top = ORIGIN(ram) + LENGTH(ram);
ENTRY(reset_handler)

SECTIONS
{
    .vector_table :
    {
        KEEP(*(.vector_table))
        . = ALIGN(4);
    } > flash

    .text :
    {
        *(.text .text.*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(4);
    } > flash

    .rodata :
    {
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > flash

    .data :
    {
        __data_start = .;
        *(.data .data.*)
        . = ALIGN(4);
        __data_end = .;
    } > ram AT > flash

    __data_load_start = LOADADDR(.data);

    .bss :
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > ram

    __heap_start = .;
    __heap_end   = __stack_top - 0x200;
}
