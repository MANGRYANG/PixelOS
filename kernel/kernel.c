#include <stdint.h>
#include "../font/font.h"
#include "../kernel/interrupts.h"
#include "../keyboard/keyboard.h"

#define WIDTH 320
#define HEIGHT 200
#define VGA_ADDR 0xA0000
#define BLACK 0x00
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

    keyboard_reset_state();

    asm volatile ("sti");

    int x = 0;
    int y = 0;

    while (1) {
        if(keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c)
            {
                if (c == '\n')
                {
                    x = 0;
                    y += 16;
                }
                
                else if (c == '\b')
                {
                    if (x > 8)
                    {
                        x -= 8;
                    }
                }
                
                else
                {
                    put_char(x, y, c, BLACK);
                    x += 8;
                }
            }
        }
    }
}