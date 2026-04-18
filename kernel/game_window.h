#pragma once

#include <stdint.h>
#include "../window/window.h"

// 게임 윈도우 생성 함수
Window* kernel_game_init(void);

// 게임 렌더링 관련 헬퍼
void kernel_game_clear(uint8_t color);
void kernel_game_fill_rect(int x, int y, int w, int h, uint8_t color);
void kernel_game_present(void);

// 게임을 그릴 수 있는 영역의 크기를 가져오는 함수
uint32_t kernel_game_get_size(void);