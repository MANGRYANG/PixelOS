#pragma once
#include <stdint.h>

// 이벤트의 타입을 결정하는 enum 정의
typedef enum {
    EV_NONE = 0,
    EV_KEY,
    EV_MOUSE_MOVE,
    EV_MOUSE_BUTTON,
} EventType;

// Event 구조체
typedef struct {
    // Event 타입을 저장하는 멤버 변수
    EventType type;
    // Event 타입에 따라 key/mouse_move/mouse_button 중 정확히 하나만 유효(나머지 접근 금지)
    union {
        struct {
            // make 및 break에서 공통으로 사용하는 스캔코드(0x00~0x7F)
            uint8_t scancode;
            // 1 = pressed(make), 0 = released(break)
            uint8_t pressed;
            // pressed가 1인 상태에서만 유효(없는 경우 0)
            char ascii;
        } key;

        struct {
            // 업데이트 후 절대 좌표
            int x, y;
            // 패킷 기준 상대적 이동량
            int dx, dy;
        } mouse_move;

        struct {
            // MOUSE_LEFT/RIGHT/MIDDLE 중 하나
            uint8_t button;
            // 1 = pressed, 0 = released
            uint8_t pressed;
            // 현재 버튼 상태 비트마스크
            uint8_t buttons;
            // 이벤트 시점 커서의 절대 좌표
            int x, y;
        } mouse_button;
    };
} Event;