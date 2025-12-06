#include <stdint.h>
#include "../font/font.h"
#include "../kernel/interrupts.h"

#define WIDTH 320
#define HEIGHT 200
#define VGA_ADDR 0xA0000
#define WHITE 0X0F

void kernel_main(void)
{
    // VGA 그래픽 메모리 시작
    uint8_t* const vga = (uint8_t*)VGA_ADDR;

    for(int i = 0; i < WIDTH*HEIGHT; ++i)
    {
        vga[i] = WHITE;  // 흰색 픽셀
    }

    // 인터럽트 설정
    interrupts_init();
    timer_init();
    keyboard_init();
    asm volatile ("sti");

}