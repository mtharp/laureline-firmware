__main_stack_size__     = 0x0200;

MEMORY
{
    flash : org = 0x08000000, len = 128k
    ram : org = 0x20000000, len = 32k
}

__ram_start__           = ORIGIN(ram);
__ram_size__            = LENGTH(ram);
__ram_end__             = __ram_start__ + __ram_size__;

ENTRY(Reset_Handler)

SECTIONS
{
    . = 0;
    _text = .;

    /* First a stub vector table that just points to the real init vector. It
     * is stripped when outputting the .hex file for use with a bootloader.
     * This allows the main .elf file to be used when debugging without a
     * bootloader, and the .hex file (minus boot_stub) to be used with a
     * bootloader that resides in the first page.
     */
    boot_stub : ALIGN(16) SUBALIGN(4)
    {
        KEEP(*(boot_stub))
    } > flash

    /* Now the real vector table starts at a 2K offset. Must be aligned to the
     * size of the table rounded up to a power of 2.
     */
    . = 0x800;
    startup : ALIGN(512) SUBALIGN(4)
    {
        KEEP(*(isr_vector))
    } > flash

    constructors : ALIGN(4) SUBALIGN(4)
    {
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);
    } > flash

    destructors : ALIGN(4) SUBALIGN(4)
    {
        PROVIDE(__fini_array_start = .);
        KEEP(*(.fini_array))
        KEEP(*(SORT(.fini_array.*)))
        PROVIDE(__fini_array_end = .);
    } > flash

    .text : ALIGN(16) SUBALIGN(16)
    {
        *(.text.startup.*)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue_7t)
        *(.glue_7)
        *(.gcc*)
    } > flash

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > flash

    .ARM.exidx : {
        PROVIDE(__exidx_start = .);
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        PROVIDE(__exidx_end = .);
     } > flash

    .eh_frame_hdr :
    {
        *(.eh_frame_hdr)
    } > flash

    .eh_frame : ONLY_IF_RO
    {
        *(.eh_frame)
    } > flash

    _etext = .;
    _sidata = ALIGN(4);

    .stacks :
    {
        . = ALIGN(8);
        _sstack = .;
        . += __main_stack_size__;
        . = ALIGN(8);
        _estack = .;
    } > ram

    .data : AT ( _sidata )
    {
        . = ALIGN(4);
        PROVIDE(_sdata = .);
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        PROVIDE(_edata = .);
    } > ram

    .bss :
    {
        . = ALIGN(4);
        PROVIDE(_sbss = .);
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        PROVIDE(_ebss = .);
    } > ram
}

PROVIDE(end = .);
_end            = .;

__heap_base__   = _end;
__heap_end__    = __ram_end__;
