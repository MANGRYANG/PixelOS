#pragma once

#include <stdint.h>

// 버튼 비트 플래그
#define MOUSE_LEFT    0x01
#define MOUSE_RIGHT   0x02
#define MOUSE_MIDDLE  0x04

// 마우스 초기화 함수
void mouse_init(void);

// 인터럽트 핸들러에서 호출하는 마우스 데이터 전달 함수
void mouse_on_data(uint8_t data);

// 패킷(3 바이트) 완성 시 처리 함수
void mouse_on_packet(uint8_t packet[3]);

// 상태 조회
int get_mouse_x(void);
int get_mouse_y(void);
uint8_t get_mouse_buttons(void);