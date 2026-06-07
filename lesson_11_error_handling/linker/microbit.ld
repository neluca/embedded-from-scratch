/*
 * =============================================================================
 * ARM Cortex-M0 (microbit / nRF51822) 链接脚本 — 含 .noinit section
 * =============================================================================
 *
 * .noinit section: 在 RAM 中, 但启动代码不会清零它.
 * 用于存储需在软复位/WDT 复位后保留的数据 (如故障日志).
 *
 * 注意: 上电复位时 .noinit 的值为不确定 (随机值).
 * 应用层必须用 magic number + CRC 验证数据有效性.
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

    /* --- .bss: 启动时清零 --- */
    .bss :
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > ram

    /* --- .noinit: 不初始化! 软复位后保留 --- */
    .noinit (NOLOAD) :
    {
        __noinit_start = .;
        *(.noinit .noinit.*)
        . = ALIGN(4);
        __noinit_end = .;
    } > ram

    __heap_start = .;
    __heap_end   = __stack_top - 0x200;
}
