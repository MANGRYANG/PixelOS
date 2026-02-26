#include "paging.h"

// 메모리 채우기를 위한 내부 헬퍼
static void* k_memset(void* dest, uint8_t value, uint32_t size)
{
    uint8_t* d = (uint8_t*)dest;
    for (uint32_t i = 0; i < size; ++i) d[i] = value;
    return dest;
}

// 주소를 4KiB 경계로 내림 정렬하는 내부 헬퍼
static inline uint32_t align_down_4k(uint32_t x)
{
    return x & ~0xFFFu;
}

// 가상주소에서 PDE 인덱스를 추출하는 내부 헬퍼 (상위 10비트)
static inline uint32_t pde_index(uint32_t v)
{
    return v >> 22;
}

// 가상주소에서 PTE 인덱스를 추출하는 내부 헬퍼 (중간 10비트)
static inline uint32_t pte_index(uint32_t v)
{
    return (v >> 12) & 0x3FFu;
}

// CR3 레지스터에 값을 쓰는 내부 헬퍼
static inline void write_cr3(uint32_t value)
{
    // 내부 어셈블리 실행 및 삭제 금지 선언
    __asm__ volatile ("mov %0, %%cr3" :: "r"(value) : "memory");
}

// CR0 레지스터 값을 읽어오는 내부 헬퍼
static inline uint32_t read_cr0(void)
{
    uint32_t value;
    // 내부 어셈블리 실행 및 삭제 금지 선언
    __asm__ volatile ("mov %%cr0, %0" : "=r"(value));

    return value;
}

// CR0 레지스터에 값을 쓰는 내부 헬퍼
static inline void write_cr0(uint32_t value)
{
    // 내부 어셈블리 실행 및 삭제 금지 선언
    __asm__ volatile ("mov %0, %%cr0" :: "r"(value) : "memory");
}

// 전역 페이지 디렉터리
static uint32_t g_page_directory[PDE_COUNT] __attribute__((aligned(PAGE_SIZE)));

// 전역 페이지 테이블 (첫 번째)
static uint32_t g_page_table_0[PTE_COUNT] __attribute__((aligned(PAGE_SIZE)));

// 페이징 자료구조 초기화 함수
void paging_init(void)
{
    // 페이지 디렉터리/테이블 내부 데이터 초기화
    k_memset(g_page_directory, 0, sizeof(g_page_directory));
    k_memset(g_page_table_0, 0, sizeof(g_page_table_0));

    // 첫 번째 4MiB 페이지 테이블 (0x00000000 ~ 0x003FFFFF)
    for (uint32_t i = 0; i < PTE_COUNT; ++i)
    {
        // PTE 구조 : 물리 페이지 프레임 주소(상위 20비트) - 플래그 및 상태 비트(하위 12비트)
        uint32_t phys = i * PAGE_SIZE;
        // Present 및 R/W 비트 활성화
        g_page_table_0[i] = phys | P_P | P_RW;
    }

    // PDE[0]에 Page Table 0의 주소 등록
    g_page_directory[0] = ((uint32_t)g_page_table_0) | P_P | P_RW;
}

// 페이징 활성화 함수 (CR3 로드 후 CR0.PE 및 CR0.PG 세팅)
// 유효한 페이지 테이블 보장을 위해 paging_init() 선행 필요
void paging_enable(void)
{
    // 페이지 디렉토리 주소를 CR3 레지스터에 전달
    uint32_t pd_phys = paging_get_directory_phys();
    write_cr3(pd_phys);

    // CR0의 페이징 비트(PG) 및 보호 비트(PE) 설정
    uint32_t cr0 = read_cr0();
    cr0 |= 0x80000001u;
    write_cr0(cr0);
}

// 현재 페이지 디렉토리 물리 주소를 반환하는 함수
uint32_t paging_get_directory_phys(void)
{
    return (uint32_t)g_page_directory;
}

// 주어진 가상 주소의 페이지 테이블 포인터를 반환하는 내부 헬퍼
static uint32_t* get_pt_for(uint32_t virt)
{
    uint32_t pde = g_page_directory[pde_index(virt)];
    // PDE 유효성 검사
    if ((pde & P_P) == 0) return 0;

    // PDE 상위 20비트(페이지 테이블 주소) 반환
    return (uint32_t*)(pde & 0xFFFFF000u);
}

// 단일 4KiB 페이지 매핑을 위한 함수
int map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    // 4KiB 경계로 내림 정렬
    virt = align_down_4k(virt);
    phys = align_down_4k(phys);

    // 페이지 테이블 포인터 탐색
    uint32_t* pt = get_pt_for(virt);

    // 탐색에 실패한 경우
    if (!pt)
    {
        return -1;
    }

    // 페이지 테이블의 PTE에 물리 프레임 주소 매핑
    pt[pte_index(virt)] = (phys & 0xFFFFF000u) | (flags & 0xFFFu) | P_P;
    
    return 0;
}

// 단일 4KiB 페이지 매핑 해제를 위한 함수
int unmap_page(uint32_t virt)
{
    // 4KiB 경계로 내림 정렬
    virt = align_down_4k(virt);

    // 페이지 테이블 포인터 탐색
    uint32_t* pt = get_pt_for(virt);

    // 탐색에 실패한 경우
    if (!pt)
    {
        return -1;
    }

    // 매핑 해제
    pt[pte_index(virt)] = 0;
    
    return 0;
}

// 가상 주소를 물리 주소로 변환하는 함수
uint32_t virt_to_phys(uint32_t virt)
{
    // 페이지 테이블 포인터 탐색
    uint32_t* pt = get_pt_for(virt);

    // 탐색에 실패한 경우
    if (!pt)
    {
        return 0;
    }

    // PTE 추출
    uint32_t pte = pt[pte_index(virt)];

    // 유효하지 않은 경우
    if ((pte & P_P) == 0)
    {
        return 0;
    }

    // 물리 프레임 주소 + 오프셋
    return (pte & 0xFFFFF000u) | (virt & 0xFFFu);
}
