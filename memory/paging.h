#pragma once
#include <stdint.h>

// 페이지 크기 (4KiB)
#define PAGE_SIZE 4096u

// Page Directory Entry 수 (10bit로 표현 가능한 엔트리 수 : 1024)
#define PDE_COUNT 1024u
// Page Table Entry 수 (10bit로 표현 가능한 엔트리 수 : 1024)
#define PTE_COUNT 1024u

// -- Page flags (PDE/PTE)
// 엔트리 유효성 검사 플래그
#define P_P 0x001u
// 쓰기 허용인지 확인하는 플래그
#define P_RW 0x002u
// 유저 모드에서 접근 가능한지 확인하는 플래그
#define P_US 0x004u

// 페이징 자료구조 초기화 함수
// 아직 페이징 허용하지 않음(CR0.PG). 추후 해결
void paging_init(void);

// 단일 4KiB 페이지 매핑/해제를 위한 함수
int map_page(uint32_t virt, uint32_t phys, uint32_t flags);
int unmap_page(uint32_t virt);

// 가상 주소를 물리 주소로 변환하는 함수
uint32_t virt_to_phys(uint32_t virt);
