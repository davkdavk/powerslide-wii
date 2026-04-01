/* Wii linker script for PowerPC */
OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc:750)
ENTRY(_start)

MEMORY
{
    MEM1 (rx) : ORIGIN = 0x80000000, LENGTH = 0x01800000
    MEM2 (rwx) : ORIGIN = 0x90000000, LENGTH = 0x04000000
}

SECTIONS
{
    .init :
    {
        _start = .;
        KEEP (*(.init))
    } >MEM1

    .text :
    {
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t*)
    } >MEM1

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r*)
    } >MEM1

    .data :
    {
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)
    } >MEM2

    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } >MEM2

    /DISCARD/ :
    {
        *(.comment)
        *(.gnu_version)
    }
}
