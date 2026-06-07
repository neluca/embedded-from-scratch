/*
 * =============================================================================
 * Application 链接脚本 — 位于 Flash 0x00002000 (Bootloader 之后)
 * =============================================================================
 * 注意: Cortex-M0 无 VTOR，硬件异常仍通过 bootloader 的向量表 (0x00000000)
 * App 的向量表位于 0x00002000，bootloader 手动读取并跳转
 */
MEMORY
{
    flash (rx)  : ORIGIN = 0x00002000, LENGTH = 248K
    ram   (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

__stack_top = ORIGIN(ram) + LENGTH(ram);
ENTRY(app_reset_handler)

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
