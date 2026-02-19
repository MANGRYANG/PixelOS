#pragma once

#include <stdint.h>

void interrupts_init(void);         // IDT 및 PIC 설정
void irq_enable_timer(void);        // 타이머 IRQ 활성화
void irq_enable_keyboard(void);     // 키보드 IRQ 활성화
void irq_enable_mouse(void);        // 마우스 IRQ 활성화

void timer_handler(void);           // 타이머 인터럽트 핸들러
void keyboard_handler(void);        // 키보드 인터럽트 핸들러
void mouse_handler(void);           // 마우스 인터럽트 핸들러

// 페이지 폴트 핸들러
void page_fault_handler(uint32_t fault_addr, uint32_t error_code);

extern volatile uint32_t g_last_pf_addr;
extern volatile uint32_t g_last_pf_err;

extern volatile uint32_t g_timer_ticks;