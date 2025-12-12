#pragma once
#include <stdint.h>

void cursor_init(int x, int y);
void cursor_set_pos(int x, int y);
void cursor_draw(void);

int cursor_get_x(void);
int cursor_get_y(void);

// 현재 draw target에 커서를 렌더링하는 함수
void cursor_render(void);