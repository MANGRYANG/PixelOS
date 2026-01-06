#include <stdint.h>
#include "../font/font.h"
#include "../kernel/heap.h"
#include "../kernel/interrupts.h"
#include "../kernel/event_queue.h"
#include "../kernel/event.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../mouse/cursor.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../graphics/compositor.h"
#include "../window/window.h"

void kernel_main(void)
{   
    // 힙 초기화
    heap_init();

    // 화면 초기화
    gfx_clear(COLOR_LIGHT_GRAY);

    // 인터럽트 설정
    interrupts_init();

    irq_enable_timer();
    irq_enable_keyboard();
    irq_enable_mouse();

    keyboard_reset_state();
    mouse_init();

    event_queue_init();

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

    compositor_compose();

    int mx = get_mouse_x();
    int my = get_mouse_y();
    cursor_init(mx, my);

    compositor_compose();

    uint32_t last_tick = g_timer_ticks;

    while (1) {
        // tick이 바뀔 때까지 대기
        while (g_timer_ticks == last_tick)
        {
            asm volatile("hlt");
        }

        // 이벤트 처리
        Event ev;
        while (event_pop(&ev)) {
            switch (ev.type) {
            case EV_MOUSE_MOVE:
                cursor_set_pos(ev.mouse_move.x, ev.mouse_move.y);
                break;

            case EV_MOUSE_BUTTON:

                break;

            case EV_KEY:
                if (ev.key.pressed && ev.key.ascii) {
                    window_put_char(testwin, ev.key.ascii, COLOR_BLACK);
                }
                break;

            default:
                break;
            }
        }
        
        compositor_compose();
    }
}
