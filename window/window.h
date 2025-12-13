#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../graphics/layer.h"

extern int g_top_zindex;

// Window 구조체 타입 정의
typedef struct Window
{
    // window의 가장 왼쪽 위 픽셀의 화면 기준 좌표
    int px;
    int py;
    // window의 너비 및 높이
    int width;
    int height;
    // window 내부 배경 색
    uint8_t bg_color;
    // window 테두리 색
    uint8_t border_color;
    // window 제목
    const char* title;
    // 텍스트 출력용 window 내부 커서
    // 텍스트 단위 좌표
    int cursor_x;
    int cursor_y;

    // window의 활성 상태
    bool in_use;

    // window 전용 백 버퍼
    uint8_t* buffer;
    // 논리적인 window 너비
    int stride;

    // 레이어
    Layer layer;
} Window;

// window manager 초기화
void wm_init(void);

// window 생성 함수
Window* wm_create_window(int px, int py, int width, int height, uint8_t bg_color, uint8_t border_color, const char* title);
// window 삭제 함수
void wm_destroy_window(Window* win);

// window 버퍼에 텍스트를 출력하는 함수
void window_draw_char(Window* w, int x, int y, char c, uint8_t color);
void window_draw_string(Window* w, int x, int y, const char* s, uint8_t color);

// window 내부 텍스트 출력 함수
void window_put_char(Window* win, char c, uint8_t color);
void window_put_string(Window* win, const char* s, uint8_t color);

// window를 최상위 레이어로 이동시키는 함수
void wm_bring_to_front(Window* win);