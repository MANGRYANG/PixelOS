#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int x;
    int y;
    bool visible;
} Cursor;

void cursor_init(int x, int y);
void cursor_set_pos(int x, int y);

// 커서 합성 함수
void cursor_composite(uint8_t* backbuffer);