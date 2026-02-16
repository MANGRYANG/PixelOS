#pragma once
#include <stdint.h>

// PIT 채널 0을 원하는 주파수(Hz)로 설정하는 함수
void pit_set_frequency(uint32_t hz);