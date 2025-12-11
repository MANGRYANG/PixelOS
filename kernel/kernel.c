#include <stdint.h>
#include "../font/font.h"
#include "../kernel/interrupts.h"
#include "../keyboard/keyboard.h"
#include "../graphics/graphics.h"
#include "../window/window.h"

#define WIDTH 320
#define HEIGHT 200
#define VGA_ADDR 0xA0000
#define BLACK 0x00
#define WHITE 0X0F
#define BLUE 0x01

void kernel_main(void)
{   
    // 화면 초기화
    gfx_clear(0x07);

    // 인터럽트 설정
    interrupts_init();
    timer_init();
    keyboard_init();

    keyboard_reset_state();

    asm volatile ("sti");

    // 윈도우 매니저 초기화
    wm_init();

    // 테스트용 window 생성
    Window* testwin = wm_create_window(
        8, 8,                   // px, py
        200, 160,               // width, height
        WHITE,                  // window 배경색은 흰색으로 설정
        BLUE,                   // window 테두리색은 파란색으로 설정
        "New window"            // title
    );

    // 모든 window를 포함한 화면 재출력
    wm_draw_all();

    while (1) {
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c) {
                window_put_char(testwin, c, BLACK);
            }
        }
    }
}