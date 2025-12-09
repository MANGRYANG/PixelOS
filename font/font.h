#pragma once

#include <stdint.h>

#define WIDTH 320
#define HEIGHT 200
#define VGA_ADDR 0xA0000
#define WHITE 0X0F

#define FONT_WIDTH  8
#define FONT_HEIGHT 16

// ASCII 0~127용 폰트 배열
extern uint8_t font8x16[128][FONT_HEIGHT];

// 글자 지우기 함수
void remove_char(int px, int py, uint8_t color);

// 글자 출력 함수
void put_char(int px, int py, char ascii, uint8_t color);

// 문자열 출력 함수
void put_string(int px, int py, const char* str, uint8_t color);