#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 힙 범위를 설정하는 초기화 함수
void heap_init(void);

// 힙 영역에서 메모리를 할당하는 API
void* kmalloc(size_t size);
// 힙 영역에서 메모리를 해제하는 API
bool kfree(void* ptr);

// 힙 영역에서 메모리를 할당하고, 0으로 초기화하는 API
void* kmalloc_zero(size_t size);

// -- 디버깅용 --
// 현재 힙에서 사용되고 있는 바이트 수를 반환하는 함수
size_t heap_bytes_used(void);
// 현재 힙으로 관리되는 전체 바이트 수를 반환하는 함수
size_t heap_bytes_total(void);