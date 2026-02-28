#include "gdt.h"

// GDT 엔트리(세그먼트) 구조체
struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access_byte;
    uint8_t granularity;    // flags + limit 최상위 4비트
    uint8_t base_high;
} __attribute__((packed));

// GDT 디스크립터 구조체
struct gdt_descriptor
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// TSS 구조체
struct tss
{
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

// GDT 및 TSS를 레지스터에 로드하는 함수
extern void flush_gdt(uint32_t gdt_descriptor);
extern void flush_tss(uint16_t tss_selector);

// 전역 정적 구조체
static struct gdt_entry g_gdt[6];
static struct gdt_descriptor g_gdt_descriptor;
static struct tss g_tss;

// 커널이 사용할 스택 메모리 공간(4KiB)
static uint8_t g_ring0_stack[4096];

// GDT 엔트리 등록을 위한 헬퍼
// flags 파라미터의 상위 4비트를 GDT 엔트리의 플래그(G/D/L/AVL) 비트로 사용
static void gdt_set_gate(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    g_gdt[idx].base_low = (uint16_t)(base & 0xFFFF);
    g_gdt[idx].base_mid = (uint8_t)((base >> 16) & 0xFF);
    g_gdt[idx].base_high = (uint8_t)((base >> 24) & 0xFF);
    g_gdt[idx].limit_low = (uint16_t)(limit & 0xFFFF);
    // 상위 4비트는 flags 4비트, 하위 4비트는 limit의 최상위 4비트로 설정
    g_gdt[idx].granularity = (uint8_t)((flags & 0xF0) | ((limit >> 16) & 0x0F));
    g_gdt[idx].access_byte = access;
}

// TSS 작성 및 등록을 위한 헬퍼
static void tss_write(int idx, uint32_t base, uint32_t limit)
{
    gdt_set_gate(idx, base, limit, 0x89, 0x00);
}

// GDT 초기화 함수
void gdt_init(void)
{
    // GDT 디스크립터 설정
    g_gdt_descriptor.limit = (uint16_t)(sizeof(g_gdt) - 1);
    g_gdt_descriptor.base = (uint32_t)&g_gdt;

    // Null 디스크립터
    gdt_set_gate(0, 0, 0, 0, 0);

    // 커널 코드/데이터 세그먼트 디스크립터 (DPL=0) 추가
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xC0);
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xC0);

    // 사용자 코드/데이터 세그먼트 디스크립터 (DPL=3) 추가
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xC0);
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xC0);

    // TSS 구조체 0으로 초기화 (memset)
    for (uint8_t* p = (uint8_t*)&g_tss; p < (uint8_t*)(&g_tss + 1); ++p)
    {
        *p = 0;
    }

    // 전역 TSS 구조체 작성
    g_tss.ss0  = GDT_KERNEL_DATA_SEG;
    g_tss.esp0 = (uint32_t)(g_ring0_stack + sizeof(g_ring0_stack));

    // I/O 비트맵 비활성화 (TSS의 끝 주소로 설정)
    g_tss.iomap_base = (uint16_t)sizeof(g_tss);

    // TSS 디스크립터 추가
    tss_write(5, (uint32_t)&g_tss, (uint32_t)(sizeof(g_tss) - 1));

    // GDT 및 세그먼트 레지스터 로드
    flush_gdt((uint32_t)&g_gdt_descriptor);

    // TSS 로드
    flush_tss(GDT_TASK_STATE_SEG);
}

// TSS의 esp0 값을 변경하는 함수
void gdt_set_kernel_stack(uint32_t esp0)
{
    g_tss.esp0 = esp0;
}