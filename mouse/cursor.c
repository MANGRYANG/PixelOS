#include "cursor.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"

#define CURSOR_W 8
#define CURSOR_H 8

#define CURSOR_COLOR COLOR_LIGHT_MAGENTA

// 커서 전용 버퍼
static uint8_t cursor_buffer[CURSOR_W * CURSOR_H];
// 커서 레이어 선언
static Layer cursor_layer;

void cursor_init(int x, int y)
{
    for (int i = 0; i < CURSOR_W * CURSOR_H; ++i)
    {
        cursor_buffer[i] = CURSOR_COLOR;
    }
    cursor_layer.x = x;
    cursor_layer.y = y;
    cursor_layer.w = CURSOR_W;
    cursor_layer.h = CURSOR_H;
    cursor_layer.buffer = cursor_buffer;
    cursor_layer.z = 100;   // 항상 최상위 레이어로 사용
    cursor_layer.type = LAYER_CURSOR;
    cursor_layer.visible = 1;
}

// 커서 레이어 반환 함수
Layer* cursor_get_layer(void)
{
    return &cursor_layer;
}

// 커서 위치 설정 함수
void cursor_set_pos(int x, int y)
{
    cursor_layer.x = x;
    cursor_layer.y = y;
}