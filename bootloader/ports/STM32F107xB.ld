MEMORY
{
    flash1 (rx) : org = 0x08000000, len = 2k
    flash (rx)  : org = 0x08000000 + 256k - 64k, len = 64k
    ram (!rx)   : org = 0x20000000, len = 64k
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* The bootloader occuipes the first page of flash (2K), then the remainder
     * goes at the end.
     */
    . = 0;
    _stub_start = .;
    .startup : ALIGN(512)
    {
        _stub_start = .;
        KEEP(*(isr_vector))
        . = 2k;
        _stub_end = .;
    } > flash1

    .data :
    {
        . = ALIGN(4);
        PROVIDE(_sdata = .);
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        PROVIDE(_edata = .);
    } > ram AT > flash
    _sidata = LOADADDR(.data);
    _text_start = _sidata;

    .text :
    {
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    } > flash
    _text_end = .;

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
