#include "cursor.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"

#define CURSOR_W 8
#define CURSOR_H 8

static int cx = 0, cy = 0;

void cursor_init(int x, int y)
{
    cursor_set_pos(x, y);
}

void cursor_set_pos(int x, int y)
{
    cx = x;
    cy = y;
}

void cursor_render(void)
{
    for (int y = 0; y < CURSOR_H; ++y)
    {
        for (int x = 0; x < CURSOR_W; ++x)
        {
            gfx_putpixel(cx + x, cy + y, COLOR_LIGHT_MAGENTA);
        }
    }
}

int cursor_get_x(void) { return cx; }
int cursor_get_y(void) { return cy; }