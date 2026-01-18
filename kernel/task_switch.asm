[BITS 32]
global task_switch

; void task_switch(uint32_t** old_esp, uint32_t* new_esp)
task_switch:
    cli

    mov eax, [esp+4]    ; old_esp (현재 태스크의 ESP를 저장할 변수의 주소)
    mov edx, [esp+8]    ; new_esp (다음 태스크의 ESP를 저장할 변수의 주소)

    ; 현재 스택의 레지스터 정보 보존을 위한 push
    push ebp
    push ebx
    push esi
    push edi

    ; 현재 스택 포인터 저장
    mov [eax], esp

    ; 태스크 전환
    mov esp, edx

    ; 전환된 태스크의 레지스터 정보 pop
    pop edi
    pop esi
    pop ebx
    pop ebp

    sti
    ret