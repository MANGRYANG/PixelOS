#include <stdint.h>
#include "../font/font.h"
#include "../kernel/interrupts.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../window/window.h"

void draw_mouse_cursor(int x, int y)
{
    // 간단한 8x8 검정 네모 커서
    gfx_fill_rect(x, y, 8, 8, COLOR_BLACK);
}

void kernel_main(void)
{   
    // 화면 초기화
    gfx_clear(COLOR_LIGHT_GRAY);

    // 인터럽트 설정
    interrupts_init();

    irq_enable_timer();
    irq_enable_keyboard();
    irq_enable_mouse();

    keyboard_reset_state();
    mouse_init();

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

    int mx = 150, my = 100;
    draw_mouse_cursor(mx, my);

    while (1) {
        if (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (c) {
                window_put_char(testwin, c, COLOR_BLACK);
            }
        }

        int new_mx = get_mouse_x();
        int new_my = get_mouse_y();

        if (new_mx != mx || new_my != my) {
            // 일단 화면 초기화
            wm_draw_all();
            // 새 위치에 커서 다시 그림
            draw_mouse_cursor(new_mx, new_my);

            mx = new_mx;
            my = new_my;
        }
    }
}
