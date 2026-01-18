#include <stdint.h>
#include "../font/font.h"
#include "../kernel/heap.h"
#include "../kernel/interrupts.h"
#include "../kernel/task.h"
#include "../kernel/event_queue.h"
#include "../kernel/event.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../mouse/cursor.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../graphics/compositor.h"
#include "../window/window.h"

static Window* g_testwin = 0;

// UI 작업을 위한 태스크
static void ui_task(void* arg)
{
    (void)arg;

    uint32_t last_tick = g_timer_ticks;

    for (;;) {
        // 프레임 경계 대기
        while (g_timer_ticks == last_tick) {
            asm volatile("hlt");
        }
        last_tick = g_timer_ticks;

        // 이벤트 처리
        Event ev;
        while (event_pop(&ev)) {
            if (ev.type == EV_MOUSE_MOVE) {
                cursor_set_pos(ev.mouse_move.x, ev.mouse_move.y);
            } else if (ev.type == EV_KEY) {
                if (ev.key.pressed && ev.key.ascii && g_testwin) {
                    window_put_char(g_testwin, ev.key.ascii, COLOR_BLACK);
                }
            }
        }

        compositor_compose();
        task_yield();
    }
}

static int gx = 10;
static int gvx = 1;

// 앱 작업을 위한 태스크
static void app_task(void* arg)
{
    (void)arg;

    for (;;) {
        if (g_testwin) {
            gx += gvx;
            if (gx < 4) { gx = 4; gvx = 1; }
            if (gx > g_testwin->width - 12) { gx = g_testwin->width - 12; gvx = -1; }

            window_put_char(g_testwin, '.', COLOR_RED);
        }

        task_yield();
    }
}

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
    wm_create_window(8, 8, 200, 160, COLOR_WHITE, COLOR_BLUE, "Window A");

    // 테스트용 window 생성
    g_testwin = wm_create_window(50, 50, 200, 140, COLOR_WHITE, COLOR_BLUE, "Window B");

    int mx = get_mouse_x();
    int my = get_mouse_y();
    cursor_init(mx, my);

    compositor_compose();

    task_init();
    task_create(ui_task, 0, 32768);
    task_create(app_task, 0, 32768);

    task_start();
}
