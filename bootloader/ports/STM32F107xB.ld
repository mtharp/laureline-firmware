MEMORY
{
    flash1 (rx) : org = 0x08000000, len = 2k
    flash (rx) : org = 0x0801c000, len = 16k
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
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    } > flash

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
