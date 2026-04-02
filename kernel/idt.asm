[BITS 32]

global idt_load
global timer_isr
global keyboard_isr
global mouse_isr
global page_fault_isr
global syscall_isr

extern mouse_handler
extern keyboard_handler
extern timer_handler
extern page_fault_handler
extern syscall_handler

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

;-----------------------------------------------------------------------------
; 마우스 IRQ12 인터럽트 서비스 루틴
;-----------------------------------------------------------------------------
mouse_isr:
    pusha
    call mouse_handler          ; mouse_handler 호출    
    popa

    ; IRQ12는 슬레이브 PIC에 연결되어 있음
    ; 따라서 IRQ12는 슬레이브 PIC(0xA0) -> 마스터 PIC(0x20) 순으로 EOI 명령 코드 전송
    mov al, 0x20
    out 0xA0, al           ; 슬레이브 PIC EOI
    out 0x20, al           ; 마스터 PIC EOI

    iretd

;-----------------------------------------------------------------------------
; 페이지 폴트 인터럽트 서비스 루틴
;-----------------------------------------------------------------------------
page_fault_isr:
    pusha

    ; 두 번째 인자(error_code) 가져오기
    mov eax, [esp + 32]
    push eax

    ; 첫 번째 인자(fault_addr) 가져오기
    mov eax, cr2
    push eax

    call page_fault_handler     ; page_fault_handler 호출
    add esp, 8                  ; 인자 정리 (스택에서 8바이트 제거)

    popa

    add esp, 4                  ; CPU가 스택에 추가한 error_code 정리

    iretd

;-----------------------------------------------------------------------------
; 시스템 콜 인터럽트 서비스 루틴
;-----------------------------------------------------------------------------
syscall_isr:
    pusha

    mov esi, esp                ; Trap Frame 사작 주소 (frame)
    mov eax, [esp + 28]         ; 시스템 콜 번호 (syscall_num)
    mov ecx, [esp + 24]         ; 시스템 콜 인자 (arg0)
    mov edx, [esp + 20]         ; 시스템 콜 인자 (arg1)
    mov ebx, [esp + 16]         ; 시스템 콜 인자 (arg2)

    push ebx                    ; 5번째 인자: arg2
    push edx                    ; 4번째 인자: arg1
    push ecx                    ; 3번째 인자: arg0
    push eax                    ; 2번째 인자: syscall_num
    push esi                    ; 1번째 인자: TrapFrame* frame

    call syscall_handler        ; syscall_handler 호출

    add esp, 20                 ; 인자 정리 (스택에서 20바이트 제거)

    mov esp, eax

    popa

    iretd