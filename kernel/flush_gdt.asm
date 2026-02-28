[BITS 32]

global flush_gdt
global flush_tss

; 현재 커널 코드/데이터 세그먼트 셀렉터
%define KERNEL_CODE_SEG 0x08
%define KERNEL_DATA_SEG 0x10

;----------------------------------------------------------------------------
; void flush_gdt(uint16_t gdt_descriptor);
;----------------------------------------------------------------------------

flush_gdt:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp KERNEL_CODE_SEG:.flush_cs

.flush_cs:
    ret

;----------------------------------------------------------------------------
; void flush_tss(uint16_t tss_selector);
;----------------------------------------------------------------------------

flush_tss:
    mov ax, [esp + 4]
    ltr ax
    ret