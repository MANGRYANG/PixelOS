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

void cursor_draw()
{
    gfx_fill_rect_front(cx, cy, CURSOR_W, CURSOR_H, COLOR_BLACK);
}

int cursor_get_x(void) { return cx; }
int cursor_get_y(void) { return cy; }