#pragma once

#include <stdint.h>

// boolean 타입 정의
typedef uint8_t bool;
#define true  1
#define false 0

// 키보드 상태 초기화 함수
void keyboard_reset_state(void);

// 문자 입력 API
bool keyboard_has_char(void);    // 버퍼에 문자가 있는지 체크하는 함수
char keyboard_get_char(void);    // 버퍼에서 문자를 반환하는 함수

// 특정 스캔코드 눌림 여부 확인 함수
bool keyboard_is_key_down(uint8_t scancode);

// 인터럽트 핸들러에서 호출하는 키보드 스캔코드 전달 함수
void keyboard_on_scancode(uint8_t scancode);