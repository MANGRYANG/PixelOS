#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../graphics/layer.h"

void cursor_init(int x, int y);

// 커서 레이어 반환 함수
Layer* cursor_get_layer(void);
// 커서 위치 설정 함수
void cursor_set_pos(int x, int y);