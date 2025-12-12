#include <stdint.h>
#include "../font/font.h"
#include "../kernel/interrupts.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../mouse/cursor.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../window/window.h"

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

    wm_composite();

    int mx = get_mouse_x();
    int my = get_mouse_y();
    cursor_init(mx, my);
    gfx_present_with_cursor(mx, my, COLOR_LIGHT_RED);

    while (1) {
        int new_mx = get_mouse_x();
        int new_my = get_mouse_y();

        if (new_mx != mx || new_my != my) {
            cursor_set_pos(new_mx, new_my);
            mx = new_mx;
            my = new_my;
        }
        
        gfx_present_with_cursor(mx, my, COLOR_LIGHT_RED);
    }
}
