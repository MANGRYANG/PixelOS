[BITS 32]

global idt_load
global timer_isr
global keyboard_isr

extern keyboard_handler
extern timer_handler

;-----------------------------------------------------------------------------
; IDT 로드
;-----------------------------------------------------------------------------
idt_load:
    mov eax, [esp+4]            ; 함수로 전달된 첫 번째 인자 (idtp 주소)
    lidt [eax]                  ; CPU에 IDT 정보 등록
    ret

;-----------------------------------------------------------------------------
; 타이머 IRQ0 인터럽트 서비스 루틴
;-----------------------------------------------------------------------------
timer_isr:
    pusha
    call timer_handler          ; timer_handler 호출
    popa

    mov al, 0x20                ; PIC EOI(End of interrupt) 명령 코드 전송
    out 0x20, al
    
    iretd                       ; 인터럽트에서 복귀

;-----------------------------------------------------------------------------
; 키보드 IRQ1 인터럽트 서비스 루틴
;-----------------------------------------------------------------------------
keyboard_isr:
    pusha
    call keyboard_handler       ; keyboard_handler 호출
    popa

    mov al, 0x20
    out 0x20, al                ; PIC EOI(End of interrupt) 명령 코드 전송

    iretd                       ; 인터럽트에서 복귀