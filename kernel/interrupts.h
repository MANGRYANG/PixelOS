#pragma once

#include <stdint.h>

// CPU가 Ring3에서 Ring0으로 전환되는 시점의 레지스터 스냅샷
typedef struct TrapFrame
{
    // pusha 저장 순서에 맞춘 레이아웃
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t old_esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    // ring3 -> ring0 진입 시 CPU가 자동으로 저장하는 값
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t useresp;
    uint32_t ss;
} TrapFrame;

void interrupts_init(void);         // IDT 및 PIC 설정
void irq_enable_timer(void);        // 타이머 IRQ 활성화
void irq_enable_keyboard(void);     // 키보드 IRQ 활성화
void irq_enable_mouse(void);        // 마우스 IRQ 활성화

void timer_handler(void);           // 타이머 인터럽트 핸들러
void keyboard_handler(void);        // 키보드 인터럽트 핸들러
void mouse_handler(void);           // 마우스 인터럽트 핸들러

// 페이지 폴트 핸들러
void page_fault_handler(uint32_t fault_addr, uint32_t error_code);

// 시스템 콜 핸들러
TrapFrame* syscall_handler(TrapFrame* frame, uint32_t syscall_no, uint32_t arg0, uint32_t arg1, uint32_t arg2);

extern volatile uint32_t g_last_pf_addr;
extern volatile uint32_t g_last_pf_err;

extern volatile uint32_t g_timer_ticks;