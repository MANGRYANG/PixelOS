;-----------------------------------------------------------------------------
; GDT 정의
;-----------------------------------------------------------------------------
gdt_start:
    dq 0x0000000000000000       ; Null Descriptor
    dq 0x00CF9A000000FFFF       ; Code segment
    dq 0x00CF92000000FFFF       ; Data segment
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT 크기
    dd gdt_start                ; GDT 시작 주소

CODE_SEG equ 0x08
DATA_SEG equ 0x10