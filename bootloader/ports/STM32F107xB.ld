__main_stack_size__     = 0x0200;

MEMORY
{
    flash1 (rx) : org = 0x08000000, len = 2k
    flash2 (rx) : org = 0x0801c000, len = 16k
    ram (!rx) : org = 0x20000000, len = 32k
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* The bootloader occuipes the first page of flash (2K), then the remainder
     * goes at the end.
     */
    . = 0;
    .startup : ALIGN(512)
    {
        KEEP(*(isr_vector))
    } > flash1

    .stacks :
    {
        . = ALIGN(8);
        _sstack = .;
        . += __main_stack_size__;
        . = ALIGN(8);
        _estack = .;
    } > ram

    .data :
    {
        . = ALIGN(4);
        PROVIDE(_sdata = .);
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        PROVIDE(_edata = .);
    } > ram AT > flash1
    _sidata = LOADADDR(.data);

    .text :
    {
        *(.text)
    } > flash2

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
