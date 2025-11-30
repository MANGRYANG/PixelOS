;--------------------------------------------------------------------------------
; 커널 로더
;--------------------------------------------------------------------------------
load_kernel:
    mov si, MSG_LOAD_KERNEL
    call print_loop

    ; 커널 임시 로드 (0x8000)
    mov ah, 0x42            ; 확장된 디스크 읽기 모드 설정
    mov dl, [boot_drive]    ; 드라이브 번호 (첫 번째 하드디스크)
    mov si, kernel_dap      ; DAP 구조체 주소
    int 0x13                ; BIOS Interrupt (디스크 I/O 서비스)
    jc .load_error          ; Carry flag 체크 -> 에러 발생 시 load_error로 점프

    call graphic_mode       ; 화면 모드 전환 (그래픽 모드)

    jmp entry_PM            ; 보호 모드 진입 - 이후 PROTECTEDMODE 레이블로 점프

.load_error:
    mov si, MSG_LOAD_KERNEL_TEMP_ERROR
    call print_loop

    jmp $

;--------------------------------------------------------------------------------
; DAP 구조체
;--------------------------------------------------------------------------------
kernel_dap:
    db 0x10                 ; DAP 구조체 크기 : 16바이트
    db 0                    ; 예약된 공간
    dw 100                  ; 읽을 섹터 수
    dw 0x8000               ; BX : 오프셋
    dw 0                    ; ES : 세그먼트
    dd 1                    ; LBA 하위 32비트
    dd 0                    ; LBA 상위 

;--------------------------------------------------------------------------------
; 외부 코드 포함
;--------------------------------------------------------------------------------
%include "boot/video_mode.asm"
%include "boot/switch_pm.asm"

;--------------------------------------------------------------------------------
; 데이터
;--------------------------------------------------------------------------------
MSG_LOAD_KERNEL db "Loading kernel into main memory....", 0x0A, 0
MSG_LOAD_KERNEL_TEMP_ERROR db "Failed to load kernel into temporary space", 0x0A, 0

boot_drive db 0x80          ; 첫 번째 하드디스크

;--------------------------------------------------------------------------------
; PM 작업 코드
;--------------------------------------------------------------------------------
[BITS 32]

PROTECTEDMODE:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x0009FC00

    ; 커널 재배치
    mov esi, 0x00008000     ; source (0x8000)
    mov edi, 0x00100000     ; destination (0x100000)
    mov ecx, 12800          ; dword 단위 카운트 (100 * 512 bytes / 4 bytes = 12800)
    cld
    rep movsd               ; 4바이트씩 복사

    jmp CODE_SEG:0x00100000 ; 커널 진입