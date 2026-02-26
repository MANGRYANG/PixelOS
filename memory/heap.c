#include "heap.h"

// 힙 크기는 1MB로 설정
#define HEAP_SIZE (1024 * 1024)
#define HEAP_ALIGN 16
#define HEAP_MAGIC 0xC0FFEE01u

// 링커 스크립트에서 제공하는 심볼
extern uint8_t _kernel_end;

typedef struct BlockHeader
{
    uint32_t magic;             // 검증용 넘버
    uint32_t size;              // payload 크기
    uint32_t flags;             // bit0 = 0 : 할당됨
    struct BlockHeader* next;   // free-list 연결
} BlockHeader;

// 내부 힙을 관리하는 포인터
static uint8_t* g_heap_start = 0;
static uint8_t* g_heap_end = 0;
static uint8_t* g_heap_curr = 0;

// 해제된 힙 블록(BlockHeader)들을 연결할 explicit free-list의 시작 포인터
static BlockHeader* g_free_list = 0;

// 주소 값(address)을 align 경계로 올림(ceil) 정렬하여 정렬된 주소 값을 반환
static inline uintptr_t align_up(uintptr_t address, size_t align)
{
    return (address + (align - 1)) & ~(uintptr_t)(align - 1);
}

// 크기(bytes)를 align 단위로 올림(ceil) 정렬하여 정렬된 크기를 반환
static inline size_t align_up_size(size_t bytes, size_t align)
{
    return (bytes + (align - 1)) & ~(size_t)(align - 1);
}

// 블록을 free 상태로 표시하고 explicit free-list의 head에 연결
static inline void push_free(BlockHeader* block)
{
    block->flags = 1;
    block->next = g_free_list;
    g_free_list = block;
}

// explicit free-list에서 요청된 크기 이상의 블록을 찾아 반환
static BlockHeader* pop_fit(size_t size)
{
    BlockHeader* prev = 0;
    BlockHeader* current = g_free_list;

    while (current)
    {
        if ((current->magic == HEAP_MAGIC) &&
            (current->flags & 1) &&
            (current->size >= size))
        {
            if (prev)
            {
                prev->next = current->next;
            }

            else 
            {
                g_free_list = current->next;
            }

            current->next = 0;
            current->flags = 0;

            return current;
        }

        else
        {
            prev = current;
            current = current->next;
        }

    }
    return 0;
}

// 힙 범위를 설정하는 초기화 함수
void heap_init(void)
{
    // 커널 끝 주소 다음부터 힙 영역 시작
    uintptr_t start = (uintptr_t)&_kernel_end;
    // 힙 시작 주소를 16바이트 단위로 맞춤
    start = align_up(start, HEAP_ALIGN);

    // 힙 관리 포인터 초기화
    g_heap_start = (uint8_t*)start;
    g_heap_curr = (uint8_t*)start;
    g_heap_end = (uint8_t*)(start + HEAP_SIZE);

    // explicit free-list 초기화
    g_free_list = 0;
}

// bump allocator 방식으로 힙 끝에서 새 블록을 할당
static void* alloc_from_bump(size_t size)
{
    // 현재 힙 커서를 주소 값으로 가져와 HEAP_ALIGN 경계로 올림 정렬
    uintptr_t p = (uintptr_t)g_heap_curr;
    p = align_up(p, HEAP_ALIGN);

    // 이번 블록에 필요한 총 바이트 계산
    uintptr_t needed = sizeof(BlockHeader) + size;
    
    // 필요한 총 바이트가 힙 범위를 넘어서는 경우 실패
    if (p + needed > (uintptr_t)g_heap_end)
    {
        return 0;
    }
    
    // 정렬된 위치에 블록 헤더 생성
    BlockHeader* header = (BlockHeader*)p;
    header->magic = HEAP_MAGIC;
    header->size = (uint32_t)size;
    header->flags = 0;
    header->next = 0;

    // payload 주소 계산 (블록 헤더 바로 다음)
    void* payload = (void*)((uint8_t*)header + sizeof(BlockHeader));

    // bump 커서를 블록 끝으로 이동
    g_heap_curr = (uint8_t*)(p + needed);
    
    // payload 주소 반환
    return payload;
}

// 힙 영역에서 메모리를 할당하는 API
void* kmalloc(size_t size)
{
    // 0바이트 할당의 경우 무시
    if (size == 0)
    {
        return 0;
    }

    size = align_up_size(size, HEAP_ALIGN);

    // free-list에서 필요 사이즈보다 큰 블록 탐색
    BlockHeader* header = pop_fit(size);

    // 탐색 성공
    if (header)
    {
        uint32_t old_size = header->size;

        // 남는 공간이 header + 최소 payload 이상이면 쪼개서 free 블럭으로 분리
        if (old_size >= size + sizeof(BlockHeader) + HEAP_ALIGN)
        {
            uint8_t* payload = (uint8_t*)header + sizeof(BlockHeader);
            BlockHeader* newheader = (BlockHeader*)(payload + size);
            newheader->magic = HEAP_MAGIC;
            newheader->size = old_size - (uint32_t)size - (uint32_t)sizeof(BlockHeader);
            newheader->flags = 1;
            newheader->next = 0;

            header->size = (uint32_t)size;

            push_free(newheader);
        }

        // payload 주소 반환
        return (void*)((uint8_t*)header + sizeof(BlockHeader));
    }

    // 탐색 실패
    else
    {
        // bump allocator 방식으로 힙에서 할당
        return alloc_from_bump(size);
    }
}

// 힙 영역에서 메모리를 해제(free-list에 등록)하는 API
bool kfree(void* ptr)
{
    // NULL인 경우 실패
    if (!ptr)
    {
        return false;
    }
    
    // 바이트 단위 포인터 산술 연산을 위한 캐스팅
    uint8_t* p = (uint8_t*)ptr;

    // heap 범위를 벗어나는 경우 실패
    if (p < (g_heap_start + sizeof(BlockHeader)) || p >= g_heap_end)
    {
        return false;
    }
    
    // 블록 헤더 주소 계산
    BlockHeader* header = (BlockHeader*)(p - sizeof(BlockHeader));

    // 블록 헤더의 검증값 확인을 통한 잘못된 포인터 방지
    if (header->magic != HEAP_MAGIC)
    {
        return false;
    }

    // 이중 free 방지
    if (header->flags & 1)
    {
        return false;
    }

    // 블록을 free-list에 등록
    push_free(header);

    return true;
}

// 힙에서 메모리 할당 후 0으로 초기화하는 API
void* kmalloc_zero(size_t size)
{
    uint8_t* p = (uint8_t*)kmalloc(size);

    // 할당 실패
    if (!p)
    {
        return 0;
    }

    // payload 내부를 0으로 초기화
    for (size_t i = 0; i < size; ++i)
    {
        p[i] = 0;
    }

    return p;
}

// -- 디버깅용 --
// 현재 힙에서 사용되고 있는 바이트 수를 반환하는 함수
size_t heap_bytes_used(void)
{
    // bump로 진행된 만큼만 사용량으로 계산
    return (size_t)(g_heap_curr - g_heap_start);
}

// 현재 힙으로 관리되는 전체 바이트 수를 반환하는 함수
size_t heap_bytes_total(void)
{
    // 힙 전체 크기를 반환
    return (size_t)(g_heap_end - g_heap_start);
}