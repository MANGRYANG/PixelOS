#include "cursor.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"

#define CURSOR_W 8
#define CURSOR_H 8

static int cx, cy;
static uint8_t under[CURSOR_W * CURSOR_H];
static int under_valid = 0;

static inline int idx(int x, int y)
{
    return y * CURSOR_W + x;
}

static void save_under(int x, int y)
{
    for (int j = 0; j < CURSOR_H; ++j)
        for (int i = 0; i < CURSOR_W; ++i)
            under[idx(i, j)] = gfx_getpixel(x + i, y + j);

    under_valid = 1;
}

static void restore_under(int x, int y)
{
    if (!under_valid) return;

    for (int j = 0; j < CURSOR_H; ++j)
        for (int i = 0; i < CURSOR_W; ++i)
            gfx_putpixel(x + i, y + j, under[idx(i, j)]);
}

static void draw_cursor(int x, int y)
{
    gfx_fill_rect(x, y, CURSOR_W, CURSOR_H, COLOR_BLACK);
}

void cursor_init(int x, int y)
{
    cx = x;
    cy = y;
    save_under(cx, cy);
    draw_cursor(cx, cy);
}

void cursor_move(int x, int y)
{
    if (x == cx && y == cy) return;

    restore_under(cx, cy);
    cx = x;
    cy = y;
    save_under(cx, cy);
    draw_cursor(cx, cy);
}

void cursor_refresh(void)
{
    restore_under(cx, cy);
    save_under(cx, cy);
    draw_cursor(cx, cy);
}