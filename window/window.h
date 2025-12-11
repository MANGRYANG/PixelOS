#pragma once

#include <stdint.h>
#include <stdbool.h>

// z-index 관리를 위한 전역 변수 설정
static int g_top_zindex = 0;

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

    // 윈도우 순서 설정을 위한 z-index
    int z_index;

    // window의 활성 상태
    bool in_use;
} Window;

// window manager 초기화
void wm_init(void);

// window 생성 함수
Window* wm_create_window(int px, int py, int width, int height, uint8_t bg_color, uint8_t border_color, const char* title);

// 모든 window를 포함한 화면 재출력 함수
void wm_draw_all(void);

// window 내부 텍스트 출력 함수
void window_put_char(Window* win, char c, uint8_t color);
void window_put_string(Window* win, const char* s, uint8_t color);

// window를 최상위 레이어로 이동시키는 함수
void wm_bring_to_front(Window* win);
