MEMORY
{
    flash1 (rx) : org = 0x08000000, len = 2k
    flash (rx)  : org = 0x08000000 + 256k - 64k, len = 64k
    ram (!rx)   : org = 0x20000000, len = 64k
}

ENTRY(Reset_Handler)

SECTIONS
{
    . = 0;

    /* The bootloader occupies the first page of flash (2K), then the remainder
     * goes at the end of flash.
     */
    .startup : ALIGN(512)
    {
        KEEP(*(isr_vector))
        . = 512;
        KEEP(*(*.info_table))
        . = 2k;
        _user_start = .;
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
    _user_end = _sidata;

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

    .uninit (NOLOAD) :
    {
        *(.uninit)
    } > ram
}
