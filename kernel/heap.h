#pragma once
#include <stdint.h>
#include <stddef.h>

// 힙 범위를 설정하는 초기화 함수
void heap_init(void);

// 주소 범위를 예약하는 함수
void* kmalloc_aligned(size_t size, size_t align);

// 16바이트 단위로 정렬하여 할당하는 API (kmalloc_aligned 함수 이용)
void* kmalloc(size_t size);