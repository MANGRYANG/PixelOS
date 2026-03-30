[BITS 32]

section .text

global jump_usermode

; 현재 사용자 코드/데이터 세그먼트 셀렉터 (RPL = 3) 
%define USER_CODE_SEG 0x1B
%define USER_DATA_SEG 0x23

;-----------------------------------------------------------------------------
; void jump_usermode(uint32_t user_eip, uint32_t user_esp);
;-----------------------------------------------------------------------------
jump_usermode:
    cli

    ; 유저 데이터 세그먼트를 미리 로드 (CPL0에서 RPL=3 셀렉터 로드가 가능함)
    mov ax, USER_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 함수 인자 미리 확보
    mov ecx, [esp + 4] ; user_eip
    mov edx, [esp + 8] ; user_esp

    ; iretd를 위한 스택 프레임 구성
    ; SS, ESP, EFLAGS, CS, EIP 순서로 푸시
    
    push dword USER_DATA_SEG        ; SS

    push edx                        ; ESP (user_esp)

    pushfd
    pop eax
    or eax, 0x200 ; IF bit 설정
    push eax                        ; EFLAGS
    
    push dword USER_CODE_SEG        ; CS
    
    push ecx                        ; EIP (user_eip)
    
    iretd

section .usertext

global usermode_entry
extern user_test_main

;-----------------------------------------------------------------------------
; void usermode_entry(void);
;-----------------------------------------------------------------------------
usermode_entry:
    mov ax, USER_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call user_test_main             ; 실제 유저 모드 테스트 함수 호출

.hang:
    jmp .hang