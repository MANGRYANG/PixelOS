ENTRY(_start)           # 커널 실행 진입점 심볼 설정

SECTIONS {              # kernel.bin의 메모리 배치 규칙 정의
    . = 0x00100000;     # Location counter = 0x00100000 (커널 시작 주소)

    .text : ALIGN(4096)
    {
        *(.text*)       # 모든 실행 코드를 하나로 묶어 배치
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata*)     # 모든 읽기 전용 데이터를 하나로 묶어 배치
    }

    .data : ALIGN(4096)
    {
        *(.data*)       # 초기값이 존재하는 전역 및 정적 변수를 하나로 묶어 배치
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)       # 모든 COMMON 심볼들을 bss 블록에 배치
        *(.bss*)        # 초기값이 없는 전역 및 정적 변수를 하나로 묶어 배치
    }
}