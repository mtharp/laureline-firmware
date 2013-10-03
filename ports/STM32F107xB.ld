MEMORY
{
    flash (rx) : org = 0x08000000, len = 128k
    ram (!rx) : org = 0x20000000, len = 32k
}

ENTRY(Reset_Handler)

SECTIONS
{
    . = 0;

    /* First a stub vector table that just points to the real init vector. It
     * is stripped when outputting the .hex file for use with a bootloader.
     * This allows the main .elf file to be used when debugging without a
     * bootloader, and the .hex file (minus boot_stub) to be used with a
     * bootloader that resides in the first page.
     */
    boot_stub : ALIGN(16)
    {
        KEEP(*(boot_stub))
    } > flash

    /* Now the real vector table starts at a 2K offset. Must be aligned to the
     * size of the table rounded up to a power of 2.
     */
    . = 0x800;
    startup : ALIGN(512)
    {
        KEEP(*(isr_vector))
    } > flash

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
