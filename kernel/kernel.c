#include <stdint.h>
#include "../font/font.h"
#include "../kernel/heap.h"
#include "../kernel/interrupts.h"
#include "../kernel/task.h"
#include "../kernel/event_queue.h"
#include "../kernel/event.h"
#include "../kernel/timer.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../mouse/cursor.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../graphics/compositor.h"
#include "../window/window.h"

static Window* g_testwin = 0;

// 드래그를 통해 이동할 윈도우
static Window* g_dragwin = 0;

// 마우스로 클릭한 부분과 창의 좌상단의 오프셋
static int g_drag_off_x = 0;
static int g_drag_off_y = 0;

// 창이 드래그 상태인지 나타내는 변수
static int g_dragging = 0;

// 유휴 상태를 위한 태스크
static void idle_task(void* arg)
{
    (void)arg;

    for (;;)
    {
        asm volatile("sti; hlt");   // 인터럽트로만 깨어남
        task_yield();               // 깨어나면 다른 태스크에게 CPU 양보
    }
}

// UI 작업을 위한 태스크
static void ui_task(void* arg)
{
    (void)arg;

    uint32_t last_tick = g_timer_ticks;

    for (;;)
    {
        // 프레임 경계 대기
        while (g_timer_ticks == last_tick)
        {
            task_sleep(1);
            continue;
        }
        last_tick = g_timer_ticks;

        // 이벤트 처리
        Event ev;
        while (event_pop(&ev))
        {
            switch (ev.type)
            {
                case EV_MOUSE_MOVE:
                {
                    int mx = ev.mouse_move.x;
                    int my = ev.mouse_move.y;

                    cursor_set_pos(mx, my);

                    // 드래그 상태이고 대상 창이 존재하는 경우
                    if (g_dragging && g_dragwin)
                    {
                        // 갱신할 좌표 계산
                        int new_x = mx - g_drag_off_x;
                        int new_y = my - g_drag_off_y;

                        // 클램프
                        if (new_x < 0) new_x = 0;
                        if (new_y < 0) new_y = 0;
                        if (new_x > WIDTH - g_dragwin->width) new_x = WIDTH - g_dragwin->width;
                        if (new_y > HEIGHT - g_dragwin->height) new_y = HEIGHT - g_dragwin->height;

                        wm_move_window(g_dragwin, new_x, new_y);
                    }

                    break;
                }

                case EV_MOUSE_BUTTON:
                {
                    // 프레스 및 버튼 상태 가져오기
                    int pressed = ev.mouse_button.pressed;
                    int button  = ev.mouse_button.button;

                    // 마우스 위치
                    int mx = ev.mouse_button.x;
                    int my = ev.mouse_button.y;

                    // 왼쪽 버튼을 누르는 경우
                    if (button == MOUSE_LEFT && pressed)
                    {
                        // 마우스 위치에서의 최상단 윈도우 가져오기 + 활성 창 등록
                        Window* w = wm_focus_at(mx, my);

                        // 윈도우가 존재하고 타이틀바 위에 있는 경우
                        if (w && wm_is_on_titlebar(w, mx, my))
                        {
                            // 드래그 대상 윈도우 설정
                            g_dragwin = w;
                            g_dragging = 1;
                            g_drag_off_x = mx - w->px;
                            g_drag_off_y = my - w->py;
                            
                            // 해당 윈도우를 최상단으로 이동
                            wm_bring_to_front(w);
                        }

                    }
                    
                    // 왼쪽 버튼을 떼는 경우
                    else if (button == MOUSE_LEFT && !pressed)
                    {
                        // 드래그 종료
                        g_dragging = 0;
                        g_dragwin = 0;
                    }

                    break;
                }

                case EV_KEY:
                {
                    wm_send_key((uint8_t)ev.key.ascii, ev.key.pressed);
                    break;
                }

                default:
                {
                    break;
                }
            }
        }

        compositor_compose();
        task_yield();
    }
}

// 앱 작업을 위한 태스크
static void app_task(void* arg)
{
    (void)arg;

    // 윈도우 준비
    while (!g_testwin)
    {
        task_sleep(1);
    }
    
    // 테스트 시작 텍스트 출력
    window_put_string(g_testwin, "[HEAP] test start\n", COLOR_BROWN);
    
    // kmalloc 1회 수행 (64바이트 payload)
    void* p = kmalloc(64);

    // kmalloc 실패
    if (!p)
    {
        window_put_string(g_testwin, "[FAIL] kmalloc(64)\n", COLOR_LIGHT_RED);
        for (;;)
        {
            task_sleep(100);
        }
    }

    // kmalloc 성공
    window_put_string(g_testwin, "[PASS] kmalloc(64)\n", COLOR_LIGHT_GREEN);

    // kfree 1회 수행
    if (!kfree(p))
    {
        // kfree 성공
        window_put_string(g_testwin, "[FAIL] kfree(ptr)\n", COLOR_LIGHT_RED);
        
        for (;;)
        {
            task_sleep(100);
        }
    }

    // kfree 실패
    else
    {
        window_put_string(g_testwin, "[PASS] kfree(ptr)\n", COLOR_LIGHT_GREEN);
    }

    window_put_string(g_testwin, "[HEAP] test done\n", COLOR_BROWN);

    for (;;)
    {
        task_sleep(100);
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

    // PIT 주파수 설정 (100Hz)
    pit_set_frequency(100);

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
    g_testwin = wm_create_window(50, 50, 200, 140, COLOR_WHITE, COLOR_BLUE, "TEST");

    wm_create_window(150, 70, 200, 140, COLOR_WHITE, COLOR_BLUE, "HIT");

    int mx = get_mouse_x();
    int my = get_mouse_y();
    cursor_init(mx, my);

    compositor_compose();

    task_init();
    task_create(idle_task, 0, 8192);
    task_create(ui_task, 0, 8192);
    task_create(app_task, 0, 8192);

    task_start();
}
