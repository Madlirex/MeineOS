ENTRY(kernel_main)

SECTIONS {
    . = 0x100000;
    kernel_start = .;

    .text : ALIGN(4K) {
        *(.text.boot)
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(.bss*)
        *(COMMON)
    }

    . = ALIGN(4K);
    kernel_end = .;
}
