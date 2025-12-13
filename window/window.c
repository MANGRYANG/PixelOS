#include "window.h"
#include "../font/font.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../graphics/layer_manager.h"
#include "../mouse/cursor.h"

// 최대로 허용되는 window 개수
#define MAX_WINDOWS 4

// window가 가질 수 있는 최대 너비 및 높이
#define MAX_W 320
#define MAX_H 200

// window 정보를 저장할 정적 배열
static Window g_windows[MAX_WINDOWS];

// z-index 순서대로 그리기 위한 포인터 배열
static Window* g_order[MAX_WINDOWS];
static int g_order_count = 0;

int g_top_zindex = 0;

// 윈도우 버퍼
static uint8_t g_window_buffers[MAX_WINDOWS][MAX_W * MAX_H];

// window manager 초기화
void wm_init(void)
{
    // 모든 window를 비활성 상태로 설정
    for (int i = 0; i < MAX_WINDOWS; ++i)
    {
        g_windows[i].in_use = false;
        g_order[i] = 0;
        g_windows[i].buffer = 0;
    }
    g_order_count = 0;
}

// 윈도우 백 버퍼에 픽셀을 그리는 내부 인라인 함수
static inline void win_putpixel(Window* w, int x, int y, uint8_t c)
{
    if (x < 0 || x >= w->width || y < 0 || y >= w->height)
        return;

    w->buffer[y * w->stride + x] = c;
}

// 윈도우 백 버퍼에 프레임을 그리는 함수
static void window_draw_frame(Window* w)
{
    // 배경
    for (int y = 0; y < w->height; y++)
        for (int x = 0; x < w->width; x++)
            w->buffer[y * w->stride + x] = w->bg_color;

    // 테두리
    for (int x = 0; x < w->width; x++) {
        win_putpixel(w, x, 0, w->border_color);
        win_putpixel(w, x, w->height - 1, w->border_color);
    }

    for (int y = 0; y < w->height; y++) {
        win_putpixel(w, 0, y, w->border_color);
        win_putpixel(w, w->width - 1, y, w->border_color);
    }

    // 제목 바
    int title_h = FONT_HEIGHT + 2;
    for (int y = 1; y <= title_h; y++)
        for (int x = 1; x < w->width - 1; x++)
            win_putpixel(w, x, y, w->border_color);

    // 제목 텍스트
    if (w->title) {
        window_draw_string(w, 4, 2, w->title, COLOR_WHITE);
    }

}

// 새 window 생성
Window* wm_create_window(int px, int py, int width, int height,
                         uint8_t bg_color, uint8_t border_color,
                         const char* title)
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

            // 윈도우 백 버퍼 연결
            win->buffer = g_window_buffers[i];
            win->stride = width;

            // 레이어 설정
            win->layer.x = px;
            win->layer.y = py;
            win->layer.w = width;
            win->layer.h = height;
            win->layer.buffer = win->buffer;
            win->layer.z = win->z_index;
            win->layer.type = LAYER_WINDOW;
            win->layer.visible = 1;

            layer_add(&win->layer);

            // 프레임 초기 렌더링
            window_draw_frame(win);

            // 생성한 window 반환
            return win;
        }
    }
    // window 생성 실패
    return 0;
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

// 윈도우 버퍼에 문자 출력 함수
void window_draw_char(Window* w, int x, int y, char c, uint8_t color)
{
    if (!w || !w->in_use)
    {
        return;
    }

    // ASCII 범위 체크
    if ((unsigned char)c >= 128)
    {
        return;
    }

    for (int row = 0; row < FONT_HEIGHT; ++row)
    {
        int py = y + row;
        if (py < 0 || py >= w->height)
        {
            continue;
        }

        uint8_t bits = font8x16[(unsigned char)c][row];

        for (int col = 0; col < FONT_WIDTH; ++col)
        {
            if (bits & (1 << (7 - col)))
            {
                int px = x + col;
                if (px < 0 || px >= w->width)
                {
                    continue;
                }

                w->buffer[py * w->stride + px] = color;
            }
        }
    }
}

void window_draw_string(Window* w, int x, int y, const char* str, uint8_t color)
{
    if (!w || !w->in_use || !str)
    {
        return;
    }

    int px = x;

    for (const char* s = str; *s; ++s)
    {
        window_draw_char(w, px, y, *s, color);
        px += FONT_WIDTH;
    }
}

// window 내부 문자 출력 함수
void window_put_char(Window* win, char c, uint8_t color)
{
    // window가 비활성 상태인 경우 그리지 않음
    if (!win || !win->in_use) return;

    // 타이틀 바 아래부터 텍스트 영역 시작
    int client_x = 4;
    int client_y = FONT_HEIGHT + 4;

    // 개행 문자 처리
    if (c == '\n')
    {
        win->cursor_x = 0;
        win->cursor_y += 1;
        return;
    }

    int max_cols = (win->width - 8) / FONT_WIDTH;
    int max_rows = (win->height - (FONT_HEIGHT + 8)) / FONT_HEIGHT;

    if (win->cursor_y >= max_rows)
    {
        return;
    }

    if (win->cursor_x >= max_cols)
    {
        win->cursor_x = 0;
        win->cursor_y++;
        if (win->cursor_y >= max_rows)
        {
            return;
        }
    }

    int px = client_x + (win->cursor_x * FONT_WIDTH);
    int py = client_y + (win->cursor_y * FONT_HEIGHT);

    for (int row = 0; row < FONT_HEIGHT; ++row)
    {
        for (int col = 0; col < FONT_WIDTH; ++col)
        {
            if ((font8x16[c][row] >> (7 - col)) & 1)
            {
                win_putpixel(win, px + col, py + row, color);
            }
        }
    }

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
    if (!win || !win->in_use)
    {
        return;
    }

    win->z_index = g_top_zindex++;
    wm_refresh_order();
}

void wm_composite(void)
{
    // desktop 배경
    gfx_clear(COLOR_LIGHT_GRAY);

    int count;
    // 레이어 매니저에서 관리 중인 모든 레이어를 z-index 순서로 읽기
    Layer** layers = layer_get_all(&count);

    for (int i = 0; i < count; ++i)
    {
        Layer* layer = layers[i];
        // 레이어가 비가시 설정인 경우 다음 레이어로 이동
        if (!layer->visible) continue;

        for (int y = 0; y < layer->h; ++y)
        {
            int sy = layer->y + y;
            // y축 범위가 화면 범위를 넘어서는 경우 무시
            if (sy < 0 || sy >= HEIGHT) continue;

            for (int x = 0; x < layer->w; ++x)
            {
                int sx = layer->x + x;
                // x축 범위가 화면 범위를 넘어서는 경우 무시
                if (sx < 0 || sx >= WIDTH) continue;

                // 레이어의 (x, y) 픽셀을 화면 백 버퍼에 합성
                uint8_t c = layer->buffer[y * layer->w + x];
                gfx_putpixel(sx, sy, c);
            }
        }
    }
}