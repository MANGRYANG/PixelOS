#include "cursor.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"

#define CURSOR_W 8
#define CURSOR_H 8

#define CURSOR_COLOR COLOR_LIGHT_MAGENTA

static Cursor g_cursor;

void cursor_init(int x, int y)
{
    cursor_set_pos(x, y);
    g_cursor.visible = true;
}

void cursor_set_pos(int x, int y)
{
    g_cursor.x = x;
    g_cursor.y = y;
}

void cursor_composite(uint8_t* backbuffer)
{
    if (!g_cursor.visible)
        return;

    for (int y = 0; y < CURSOR_H; ++y)
    {
        int sy = g_cursor.y + y;
        if (sy < 0 || sy >= HEIGHT) continue;

        for (int x = 0; x < CURSOR_W; ++x)
        {
            int sx = g_cursor.x + x;
            if (sx < 0 || sx >= WIDTH) continue;

            backbuffer[sy * WIDTH + sx] = CURSOR_COLOR;
        }
    }
}