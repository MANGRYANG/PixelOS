;--------------------------------------------------------------------------------
; 문자열 출력 루프
;--------------------------------------------------------------------------------
%define CR 0x0D         ; CR(Carriage Return)
%define LF 0x0A         ; LF(Line Feed)

print_loop:
    mov ah, 0x0E        ; TTY 모드 출력

.next:
    lodsb               ; al = [ds:si++]
    or al, al           
    jz .print_done      ; null terminator → exit

    cmp al, LF          ; 줄바꿈 처리인지 확인
    jne .print_char     ; 줄바꿈 처리가 아닌 경우 문자 출력 로직으로 이동

    ; 줄바꿈 처리 로직
    mov al, CR          ; 커서 위치를 같은 줄의 맨 앞으로 이동
    int 0x10            ; BIOS Interrupt
    
    mov al, LF          ; 커서를 다음 줄로 이동
    int 0x10            ; BIOS Interrupt
    jmp .next           ; 루프

.print_char:
    int 0x10            ; BIOS Interrupt
    jmp .next           ; 출력 루프 레이블로 이동

.print_done:
    ret                 ; 출력 완료 시 리턴