#pragma once

#include <stdint.h>

#define WIDTH  320
#define HEIGHT 200
#define VGA_ADDR 0xA0000

// 화면 지우기 함수
void gfx_clear(uint8_t color);
// 특정 좌표에 위치한 픽셀의 색을 설정하는 함수
void gfx_putpixel(int x, int y, uint8_t color);
// 특정 좌표에 위치한 픽셀의 색을 읽어오는 함수
uint8_t gfx_getpixel(int x, int y);
// 특정 좌표에서 w * h 크기의 사각형을 색으로 채워 그리는 함수
void gfx_fill_rect(int x, int y, int w, int h, uint8_t color);
// 특정 좌표에서 w * h 크기의 사각형을 테두리만 그리는 함수
void gfx_draw_rect(int x, int y, int w, int h, uint8_t color);

// 백 버퍼의 내용을 VGA 메모리로 출력하는 함수
void gfx_present(void);

// VSYNC 동기화 함수
void gfx_wait_vsync(void);