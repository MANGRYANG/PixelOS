#include <stdint.h>
#include "../font/font.h"
#include "../kernel/interrupts.h"
#include "../keyboard/keyboard.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../window/window.h"

void kernel_main(void)
{   
    // 화면 초기화
    gfx_clear(COLOR_LIGHT_GRAY);

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
        COLOR_WHITE,            // window 배경색은 흰색으로 설정
        COLOR_BLUE,             // window 테두리색은 파란색으로 설정
        "New window"            // title
    );

    // 테스트용 window 생성
    Window* testwin2 = wm_create_window(
        50, 50,
        200, 140,
        COLOR_WHITE,
        COLOR_BLUE,
        "New window2"
    );


    // 모든 window를 포함한 화면 재출력
    wm_draw_all();

    while (1) {
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c) {
                window_put_char(testwin, c, COLOR_BLACK);
            }
        }
    }
}