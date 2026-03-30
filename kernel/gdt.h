#pragma once

#include <stdint.h>

#define GDT_KERNEL_CODE_SEG 0x08
#define GDT_KERNEL_DATA_SEG 0x10

#define GDT_USER_CODE_SEG 0x18
#define GDT_USER_DATA_SEG 0x20

#define GDT_USER_CODE_SEG_R3 0x1B
#define GDT_USER_DATA_SEG_R3 0x23

#define GDT_TASK_STATE_SEG 0x28

// GDT 초기화 함수
void gdt_init(void);

// TSS의 esp0 값을 변경하는 함수
void gdt_set_kernel_stack(uint32_t esp0);