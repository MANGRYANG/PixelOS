#include <stdint.h>

#include "game_window.h"
#include "../font/font.h"
#include "../graphics/color.h"
#include "../graphics/compositor.h"

static Window* g_gamewin = 0;

#define GAME_TITLE_HEIGHT (FONT_HEIGHT + 2)

// 게임 윈도우 생성 함수
void kernel_game_init(void)
{
    g_gamewin = wm_create_window(80, 30, 220, 150, COLOR_BLACK, COLOR_BLUE, "GAME");
}

void kernel_game_clear(uint8_t color)
{
    if (!g_gamewin || !g_gamewin->in_use)
    {
        return;
    }

    const int client_x0 = 1;
    const int client_y0 = GAME_TITLE_HEIGHT + 1;
    const int client_x1 = g_gamewin->width - 1;
    const int client_y1 = g_gamewin->height - 1;

    for (int y = client_y0; y < client_y1; ++y)
    {
        for (int x = client_x0; x < client_x1; ++x)
        {
            g_gamewin->buffer[y * g_gamewin->stride + x] = color;
        }
    }
}

void kernel_game_fill_rect(int x, int y, int w, int h, uint8_t color)
{
    if (!g_gamewin || !g_gamewin->in_use)
    {
        return;
    }

    if (w <= 0 || h <= 0)
    {
        return;
    }

    // 게임 렌더링 구역의 좌표로 변경
    const int origin_x = 1;
    const int origin_y = GAME_TITLE_HEIGHT + 1;

    int x0 = origin_x + x;
    int y0 = origin_y + y;
    int x1 = x0 + w;
    int y1 = y0 + h;

    const int clip_x0 = 1;
    const int clip_y0 = GAME_TITLE_HEIGHT + 1;
    const int clip_x1 = g_gamewin->width - 1;
    const int clip_y1 = g_gamewin->height - 1;

    if (x0 < clip_x0) x0 = clip_x0;
    if (y0 < clip_y0) y0 = clip_y0;
    if (x1 > clip_x1) x1 = clip_x1;
    if (y1 > clip_y1) y1 = clip_y1;

    if (x0 >= x1 || y0 >= y1)
    {
        return;
    }

    for (int py = y0; py < y1; ++py)
    {
        for (int px = x0; px < x1; ++px)
        {
            g_gamewin->buffer[py * g_gamewin->stride + px] = color;
        }
    }
}

void kernel_game_draw_text(int x, int y, const char* text, uint8_t color)
{
    if (!g_gamewin || !g_gamewin->in_use || !text)
    {
        return;
    }

    // 게임 렌더링 구역의 좌표로 변경
    const int origin_x = 1;
    const int origin_y = GAME_TITLE_HEIGHT + 1;

    const int clip_x0 = 1;
    const int clip_y0 = GAME_TITLE_HEIGHT + 1;
    const int clip_x1 = g_gamewin->width - 1;
    const int clip_y1 = g_gamewin->height - 1;

    int draw_x = origin_x + x;
    int draw_y = origin_y + y;

    if (draw_y < clip_y0 || draw_y + FONT_HEIGHT > clip_y1)
    {
        return;
    }

    // user pointer가 잘못되었을 때 무한히 읽지 않도록 10글자로 제한
    char buf[10];
    
    int i = 0;
    while (i < 10 && text[i] != '\0')
    {
        buf[i] = text[i];
        ++i;
    }
    buf[i] = '\0';

    window_draw_string(g_gamewin, draw_x, draw_y, buf, color);
}

void kernel_game_present(void)
{
    compositor_compose();
}

// 게임을 그릴 수 있는 영역의 크기를 가져오는 함수
uint32_t kernel_game_get_size(void)
{
    if (!g_gamewin || !g_gamewin->in_use)
    {
        return 0;
    }

    const int title_h = FONT_HEIGHT + 2;

    uint16_t client_w = 0;
    uint16_t client_h = 0;

    if (g_gamewin->width > 2)
    {
        client_w = (uint16_t)(g_gamewin->width - 2);
    }

    if (g_gamewin->height > title_h + 2)
    {
        client_h = (uint16_t)(g_gamewin->height - title_h - 2);
    }

    return ((uint32_t)client_h << 16) | client_w;
}