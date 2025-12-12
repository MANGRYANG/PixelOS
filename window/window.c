#include "window.h"
#include "../font/font.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"

// 최대로 허용되는 window 개수
#define MAX_WINDOWS 4

// window 정보를 저장할 정적 배열
static Window g_windows[MAX_WINDOWS];

// z-index 순서대로 그리기 위한 포인터 배열
static Window* g_order[MAX_WINDOWS];
static int g_order_count = 0;

int g_top_zindex = 0;

// window manager 초기화
void wm_init(void)
{
    // 모든 window를 비활성 상태로 설정
    for (int i = 0; i < MAX_WINDOWS; ++i)
    {
        g_windows[i].in_use = false;
        g_order[i] = 0;
    }
    g_order_count = 0;
}

// 새 window 생성
Window* wm_create_window(int px, int py, int width, int height, uint8_t bg_color, uint8_t border_color, const char* title)
{
    for (int i = 0; i < MAX_WINDOWS; ++i)
    {
        // 비활성 상태인 window가 존재하는 경우
        if (!g_windows[i].in_use)
        {
            // 새로운 window 구조체 생성
            Window* win = &g_windows[i];
            // window 정보 설정
            win->px = px;
            win->py = py;
            win->width = width;
            win->height = height;
            win->bg_color = bg_color;
            win->border_color = border_color;
            win->title = title;
            // 초기 커서 좌표는 (0,0)으로 설정
            win->cursor_x = 0;
            win->cursor_y = 0;
            // 새 window를 활성 상태로 변경
            win->in_use = true;

            // 새로 생성된 window를 가장 상위 레이어로 자동 배치
            win->z_index = g_top_zindex++;

            // 생성한 window 반환
            return win;
        }
    }
    // window 생성 실패
    return 0;
}

// window 그리기 함수
static void draw_window(Window* win)
{
    // 인자로 받은 window가 비활성 상태이면 그리지 않음
    if (!win || !win->in_use) return;

    // 배경 채우기
    gfx_fill_rect(win->px, win->py, win->width, win->height, win->bg_color);

    // 테두리 채우기
    gfx_draw_rect(win->px, win->py, win->width, win->height, win->border_color);

    // 제목 바 높이 설정 (위아래 패딩 각 1픽셀)
    int title_bar_height = FONT_HEIGHT + 2;
    // 제목 바 채우기
    gfx_fill_rect(win->px + 1, win->py + 1, win->width - 2, title_bar_height, win->border_color);

    // 제목 텍스트 출력
    if (win->title) {
        // 패딩 설정
        int tx = win->px + 4;
        int ty = win->py + 2;
        // 제목 텍스트 출력(흰색)
        put_string(tx, ty, win->title, 0x0F);
    }
}

// g_order 포인터 배열을 활성 상태인 window로 채우는 함수
static void wm_build_order_list(void)
{
    g_order_count = 0;

    for (int i = 0; i < MAX_WINDOWS; ++i)
    {
        if (g_windows[i].in_use)
        {
            g_order[g_order_count++] = &g_windows[i];
        }
    }

    // 나머지는 0으로 정리
    for (int i = g_order_count; i < MAX_WINDOWS; ++i)
    {
        g_order[i] = 0;
    }
}

// z-index의 오버플로우 방지를 위한 z-index 일반화 함수
static void wm_normalize_zindex()
{
    for (int i = 0; i < g_order_count; ++i)
    {
        g_order[i]->z_index = i;
    }

    g_top_zindex = g_order_count;
}

// z-index 기준으로 window를 정렬하기 위한 내부 함수 정의
// Insert sort 사용
static void wm_sort_by_zindex()
{
    for (int i = 1; i < g_order_count; ++i)
    {
        Window* key = g_order[i];
        int j = i - 1;

        while (j >= 0 && g_order[j]->z_index > key->z_index)
        {
            g_order[j + 1] = g_order[j];
            --j;
        }

        g_order[j + 1] = key;
    }
}

// 정렬/정규화 파이프라인
static void wm_refresh_order()
{
    wm_build_order_list();
    wm_sort_by_zindex();
    wm_normalize_zindex();
}

// 모든 window를 포함한 화면 재출력 함수
void wm_draw_all()
{
    // 전체 배경 색 초기화
    gfx_clear(COLOR_LIGHT_GRAY);

    // 출력 전 window 순서 갱신(포인터 배열 정렬)
    wm_refresh_order();

    // 활성 상태인 window 출력 (정렬된 포인터 배열 순서대로)
    for (int i = 0; i < g_order_count; ++i)
    {
        draw_window(g_order[i]);
    }
}

// window 내부 문자 출력 함수
void window_put_char(Window* win, char c, uint8_t color)
{
    // window가 비활성 상태인 경우 그리지 않음
    if (!win || !win->in_use) return;

    // 타이틀 바 아래부터 텍스트 영역 시작
    int client_x = win->px + 4;
    int client_y = win->py + FONT_HEIGHT + 4;

    // 개행 문자 처리
    if (c == '\n')
    {
        win->cursor_x = 0;
        win->cursor_y += 1;
        return;
    }

    // 백스페이스 문자 처리
    if (c == '\b')
    {
        // 같은 줄에서 지울 수 있는 경우
        if (win->cursor_x > 0)
        {
            win->cursor_x -= 1;

            // 실제 픽셀 좌표 계산
            int px = client_x + (win->cursor_x * FONT_WIDTH);
            int py = client_y + (win->cursor_y * FONT_HEIGHT);

            for (int row = 0; row < FONT_HEIGHT; ++row)
            {
                for (int column = 0; column < FONT_WIDTH; ++column)
                {
                    gfx_putpixel(px + column, py + row, win->bg_color);
                }
            }
        }
        // 줄의 맨 앞에서 백스페이스 키 입력 시 윗줄로 이동
        else if (win->cursor_y > 0)
        {
            // 한 줄의 마지막 위치 계산
            int max_cols = (win->width - 8) / FONT_WIDTH;
            // 줄의 마지막 위치로 강제 이동
            // 줄마다 문자열 버퍼를 가지고 있지 않아 마지막 글자 위치로는 자동 이동 불가
            win->cursor_x = max_cols;

            // 윗줄로 이동
            win->cursor_y -= 1;
        }

        return;
    }

    int max_cols = (win->width - 8) / FONT_WIDTH;
    int max_rows = (win->height - (FONT_HEIGHT + 8)) / FONT_HEIGHT;

    // 출력 가능한 범위인지 검사
    if (win->cursor_y >= max_rows)
    {
        return;
    }

    if (win->cursor_x >= max_cols)
    {
        win->cursor_x = 0;
        win->cursor_y++;

        // 줄바꿈 후 줄이 넘치는 경우 출력 불가
        if (win->cursor_y >= max_rows)
            return;
    }

    int px = client_x + (win->cursor_x * FONT_WIDTH);
    int py = client_y + (win->cursor_y * FONT_HEIGHT);

    put_char(px, py, c, color);

    // 출력 후 커서 이동
    win->cursor_x++;
}

// window 내부 문자열 출력 함수
void window_put_string(Window* win, const char* s, uint8_t color)
{
    for (; *s; ++s)
    {
        window_put_char(win, *s, color);
    }
}

// 창을 가장 위 레이어로 가져오는 함수
void wm_bring_to_front(Window* win)
{
    if (!win || !win->in_use) return;

    win->z_index = g_top_zindex++;
    // 포인터 배열만 갱신
    wm_refresh_order();
}
