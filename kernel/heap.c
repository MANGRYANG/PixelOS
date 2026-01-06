#include "heap.h"

// 힙 크기는 1MB로 설정
#define HEAP_SIZE (1024 * 1024)

// 링커 스크립트에서 제공하는 심볼
extern uint8_t _kernel_end;

// 내부 힙을 관리하는 포인터
static uint8_t* g_heap_start = 0;
static uint8_t* g_heap_end = 0;
static uint8_t* g_heap_curr = 0;

// v를 align의 배수로 올림(ceil)하여 정렬된 주소를 생성하는 인라인 함수
static inline uintptr_t align_up(uintptr_t v, size_t align)
{
    return (v + (align - 1)) & ~(uintptr_t)(align - 1);
}

// 힙 범위를 설정하는 초기화 함수
void heap_init(void)
{
    // 커널 끝 주소 다음부터 힙 영역 시작
    uintptr_t start = (uintptr_t)&_kernel_end;
    // 힙 시작 주소를 16바이트 단위로 맞춤
    start = align_up(start, 16);

    // 힙 관리 포인터 초기화
    g_heap_start = (uint8_t*)start;
    g_heap_curr = (uint8_t*)start;
    g_heap_end = (uint8_t*)(start + HEAP_SIZE);
}

// 2의 거듭제곱인지 판별하는 인라인 함수
static inline int is_pow2(size_t x) {
    return x && ((x & (x - 1)) == 0);
}

// 주소 범위를 예약하는 함수
void* kmalloc_aligned(size_t size, size_t align)
{
    // 0바이트 할당의 경우 무시
    if (size == 0)
    {
        return 0;
    }
    
    // 정렬 값이 0이거나 2의 거듭제곱 꼴이 아닌 경우 16으로 기본 세팅
    if (align == 0 || !is_pow2(align))
    {
        align = 16;
    }

    uintptr_t curr = (uintptr_t)g_heap_curr;
    uintptr_t aligned = align_up(curr, align);

    uintptr_t next = aligned + size;
    // 힙 범위를 넘어서는 경우(메모리 부족) 실패
    if (next > (uintptr_t)g_heap_end)
    {
        return 0;
    }

    g_heap_curr = (uint8_t*)next;

    return (void*)aligned;
}

// 16바이트 단위로 정렬하여 할당하는 API
void* kmalloc(size_t size)
{
    return kmalloc_aligned(size, 16);
}