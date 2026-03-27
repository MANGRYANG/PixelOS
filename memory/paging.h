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
void paging_init(void);

// 페이징 활성화 함수 (CR3 로드 후 CR0.PE 및 CR0.PG 세팅)
// 유효한 페이지 테이블 보장을 위해 paging_init() 선행 필요
void paging_enable(void);

// 현재 페이지 디렉토리 물리 주소를 반환하는 함수
uint32_t paging_get_directory_phys(void);

// 단일 4KiB 페이지 매핑/해제를 위한 함수
int map_page(uint32_t virt, uint32_t phys, uint32_t flags);
int unmap_page(uint32_t virt);

// 이미 매핑된 페이지 범위에 플래그를 부여하는 함수
int paging_set_range_flags(uint32_t virt, uint32_t size, uint32_t flags);

// 가상 주소를 물리 주소로 변환하는 함수
uint32_t virt_to_phys(uint32_t virt);
