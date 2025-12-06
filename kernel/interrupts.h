#pragma once

#include <stdint.h>
#include "../font/font.h"

void interrupts_init(void);     // IDT 및 PIC 설정
void timer_init(void);          // 타이머 IRQ 활성화
void keyboard_init(void);       // 키보드 IRQ 활성화