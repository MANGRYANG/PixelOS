[BITS 16]               ; 16 bit real mode
[ORG 0x7C00]            ; 0x7C00부터 부트 섹터 시작

start:
    ; 세그먼트 레지스터 초기화
    cli			        ; 초기화 중 안전을 위한 인터럽트 비활성화
    xor ax, ax		    ; ax = 0
    mov ds, ax		    ; ds = 0
    mov es, ax		    ; es = 0
    mov ss, ax		    ; ss = 0
    mov sp, 0x7C00	    ; 스택 포인터를 부트 섹터 시작점으로 설정
    sti			        ; 인터럽트 활성화

    mov si, msg         ; 문자열 시작 주소 저장

.print_loop:
    lodsb               ; al <- [ds:si], si + 1 byte
    cmp al, 0           ; if al == 0
    je .load_stage1     ; Stage1 로드 레이블로 점프

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

.load_stage1:
    xor ax, ax          ; ax = 0
    mov es, ax          ; es = 0
    mov bx, 0x8000      ; Stage1 로드 주소
    mov ah, 0x42        ; 확장된 디스크 읽기 모드 설정
    mov dl, [boot_drive]; 드라이브 번호 (첫 번째 하드디스크)
    mov si, stage1_dap  ; DAP 구조체 주소
    int 0x13            ; BIOS Interrupt (디스크 I/O 서비스)
    jc .done_stage0     ; Carry flag 체크 -> 에러 발생 시 done_stage0으로 점프
    jmp 0x0000:0x8000   ; Stage1이 로드된 0x0000:0x8000으로 점프

.done_stage0:
    hlt                 ; 문자열 출력 후 CPU 중지
    jmp .done_stage0    ; 무한 루프

; 0 : null terminator
msg db 'Hello, PixelOS Booted from HDD!', 0x0A, 0
boot_drive db 0x80      ; 첫 번째 하드디스크

stage1_dap:
    db 0x10             ; DAP 구조체 크기 : 16바이트
    db 0                ; 예약된 공간
    dw 1                ; 읽을 섹터 수
    dw 0x8000           ; BX : 오프셋
    dw 0                ; ES : 세그먼트
    dd 1                ; LBA 하위 32비트
    dd 0                ; LBA 상위 32비트
    
times 510-($-$$) db 0   ; 부트섹터 크기를 512 byte로 맞춤 설정
dw 0xAA55               ; BIOS 부트 시그니처 : 마지막 2 byte
