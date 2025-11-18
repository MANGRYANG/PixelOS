[BITS 16]               ; 16 bit real mode
[ORG 0x8000]            ; 0x8000에서 Stage1 시작 (Stage0에서 로드됨)

start_stage1:
    ; 세그먼트 레지스터 초기화
    cli			        ; 초기화 중 안전을 위한 인터럽트 비활성화
    xor ax, ax		    ; ax = 0
    mov ss, ax		    ; ss = 0
    mov sp, 0x9000	    ; Stage1 전용 스택
    
    mov ax, 0x8000 >> 4 ; 0x8000 / 16 = 0x0800
    mov ds, ax          ; ds = 0x0800
    mov es, ax          ; es = 0x0800
    sti			        ; 인터럽트 활성화

    mov si, msg_stage1  ; 문자열 시작 주소 저장

.print_loop:
    lodsb               ; al <- [ds:si], si + 1 byte
    cmp al, 0           ; if al == 0
    je .done_stage1     ; done_stage1 레이블로 이동 

    cmp al, 0x0A        ; 줄바꿈 처리인지 확인
    jne .print_char     ; 줄바꿈 처리가 아닌 경우 문자 출력 레이블로 이동

    ; 줄바꿈 처리 로직
    mov ah, 0x0E        ; TTY 모드 출력
    mov al, 0x0D        ; CR(Carriage Return) : 커서 위치를 같은 줄의 맨 앞으로 이동
    int 0x10            ; BIOS Interrupt
    mov al, 0x0A        ; LF(Line Feed) : 커서를 다음 줄로 이동
    int 0x10            ; BIOS Interrupt
    jmp .print_loop     ; 루프

.print_char:
    mov ah, 0x0E        ; TTY 모드 출력
    int 0x10            ; BIOS Interrupt
    jmp .print_loop     ; 출력 루프 레이블로 이동

.done_stage1:
    hlt                 ; 문자열 출력 후 CPU 중지
    jmp .done_stage1    ; 무한 루프

; 0 : null terminator
msg_stage1 db 'Stage1 is loaded successfully!', 0x0A, 0
