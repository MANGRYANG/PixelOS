[BITS 32]
[ORG 0x00100000]

clear_screen_white:
    pushad

    mov edi, 0xA0000      ; VGA 그래픽 메모리 시작
    mov al, 0x0F          ; 흰색 픽셀
    mov ecx, 320*200      ; 화면 전체 픽셀 수
    cld
    rep stosb             ; 픽셀 단위로 반복

    popad

    jmp .done

.done:
    jmp $

times 2048-($-$$) db 0