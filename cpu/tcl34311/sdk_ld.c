#ifdef __LD__

OUTPUT_FORMAT("elf32-littlepi32v2")
OUTPUT_ARCH(pi32v2)
ENTRY(_start)

SECTIONS
{
    . = 0x00000000;

    .text : {
        *(.text.start)
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
}

#endif
