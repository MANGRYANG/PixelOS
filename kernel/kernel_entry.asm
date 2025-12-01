[BITS 32]

global _start

extern kernel_main

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call kernel_main

    jmp $

DATA_SEG equ 0x10