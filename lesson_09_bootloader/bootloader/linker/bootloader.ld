/*
 * =============================================================================
 * Bootloader 链接脚本 — 位于 Flash 起始 (0x00000000)
 * =============================================================================
 * Bootloader 占用 Flash 前 8KB (0x00000000 - 0x00001FFF)
 * 应用程序从 0x00002000 开始
 */

MEMORY
{
    flash (rx)  : ORIGIN = 0x00000000, LENGTH = 8K
    ram   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

__stack_top = ORIGIN(ram) + LENGTH(ram);
ENTRY(reset_handler)

SECTIONS
{
    .vector_table : { KEEP(*(.vector_table)) . = ALIGN(4); } > flash
    .text         : { *(.text .text.*) *(.glue_7) *(.glue_7t) . = ALIGN(4); } > flash
    .rodata       : { *(.rodata .rodata.*) . = ALIGN(4); } > flash
    .data : { __data_start = .; *(.data .data.*) . = ALIGN(4); __data_end = .; } > ram AT > flash
    __data_load_start = LOADADDR(.data);
    .bss : { __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(4); __bss_end = .; } > ram
    __heap_start = .;
    __heap_end   = __stack_top - 0x200;
}
