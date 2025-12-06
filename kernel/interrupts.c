#include "io.h"
#include "interrupts.h"

// IDT 엔트리 구조체
struct idt_entry
{
    uint16_t base_low;          // ISR의 하위 16 bit
    uint16_t selector;          // GDT 코드 세그먼트
    uint8_t  zero;              // 항상 0으로 설정
    uint8_t  type_attributes;   // Gate 타입, DPL, P 비트를 설정
    uint16_t base_high;         // ISR의 상위 16 bit
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;             // IDT의 마지막 바이트 오프셋
    uint32_t base;              // IDT의 시작 주소
} __attribute__((packed));

#define IDT_SIZE 256            // IDT 엔트리 개수 = 256

static struct idt_entry idt[IDT_SIZE];
static struct idt_ptr idtp;

// idt.asm에서 제공되는 ASM 함수
extern void idt_load(uint32_t idtp_addr);
extern void timer_isr(void);
extern void keyboard_isr(void);

// IDT 초기화를 위한 간단한 memset 함수(내부 함수)
static void* memset(void *dest, uint8_t value, uint32_t size)
{
    uint8_t *p = (uint8_t*)dest;
    for (uint32_t i = 0; i < size; ++i)
    {
        p[i] = value;
    }
    return dest;
}

// IDT 엔트리 설정 함수
static void idt_set_gate(uint8_t idx, uint32_t base, uint16_t sel, uint8_t types)
{
    idt[idx].base_low  = (uint16_t)(base & 0xFFFF);
    idt[idx].selector  = sel;   // GDT 코드 세그먼트(0x08)
    idt[idx].zero = 0;
    idt[idx].type_attributes = types;
    idt[idx].base_high = (uint16_t)((base >> 16) & 0xFFFF);
}

// PIC 포트 매크로
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

// PIC 리매핑 함수(내부 함수)
// 마스터 PIC(IRQ 0~7) -> IDT 0x20~0x27
// 슬레이브 PIC(IRQ 8~15) -> IDT 0x28~0x2F
static void pic_remap(void)
{
    uint8_t pic1_mask, pic2_mask;

    // 현재 마스크 상태 저장
    pic1_mask = inb(PIC1_DATA);
    pic2_mask = inb(PIC2_DATA);

    // PIC에 ICW1(초기화 모드 진입 명령어) 전달
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);

    // PIC에 ICW2(인터럽트 벡터 시작 오프셋) 전달
    outb(PIC1_DATA, 0x20);  // 마스터 PIC 오프셋: 0x20
    outb(PIC2_DATA, 0x28);  // 슬레이브 PIC 오프셋: 0x28

    // PIC에 ICW3(마스터-슬레이브 연결 구조) 전달
    outb(PIC1_DATA, 0x04);  // 마스터 PIC: 슬레이브 PIC는 IRQ2를 통해 연결 (2번째 비트가 1)
    outb(PIC2_DATA, 0x02);  // 슬레이브 PIC: ID = 2

    // PIC에 ICW4(PIC 추가 모드 정보) 전달
    outb(PIC1_DATA, 0x01);  // 8086 모드 사용
    outb(PIC2_DATA, 0x01);  // 8086 모드 사용

    // PIC에 OCW1(IRQ 마스킹 정보) 전달
    outb(PIC1_DATA, pic1_mask);
    outb(PIC2_DATA, pic2_mask);
}

// 타이머 tick을 저장할 변수 선언
volatile uint32_t g_timer_ticks = 0;

// asm 코드에서 호출되는 타이머 핸들러 정의
void timer_handler(void)
{
    // 타이머 tick 증가
    ++g_timer_ticks;
}

// 키보드 스캔코드를 저장할 변수 선언
volatile uint8_t g_latest_scancode = 0;

// 테스트 출력을 위한 16진수 변환 함수
static char hex_digit(uint8_t v)
{
    return "0123456789ABCDEF"[v & 0x0F];
}

// asm 코드에서 호출되는 키보드 핸들러 정의
void keyboard_handler(void)
{
    uint8_t scancode = inb(0x60);  // PS/2 스캔코드를 읽기
    g_latest_scancode = scancode;

    // 테스트용 스캔코드 출력 로직

    // 릴리즈 이벤트 무시
    if (scancode & 0x80)
    {
        return;
    }

    static int cursor_x = 0;    // X좌표
    static int cursor_y = 0;    // Y좌표

    char buf[3];
    buf[0] = hex_digit(scancode >> 4);  // 스캔코드 상위 4비트
    buf[1] = hex_digit(scancode);       // 스캔코드 하위 4비트
    buf[2] = 0;

    put_string(cursor_x, cursor_y, buf, 0x00);
    
    cursor_x += 16;  // 두 글자만큼 이동

    // 줄이 길어지면 다음 줄로 이동
    if (cursor_x >= 300)
    {
        cursor_x = 0;
        cursor_y += 16;
    }
}

// 인터럽트 초기화 함수
void interrupts_init(void)
{
    // IDT 포인터 구조체 초기화
    idtp.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
    idtp.base  = (uint32_t)&idt;

    // IDT를 0으로 초기화
    memset(&idt, 0, sizeof(idt));

    // PIC 리매핑
    pic_remap();

    // 타이머 인터럽트 설정(IRQ0) -> IDT 인덱스 0x20에 매핑됨
    // 타이머 인터럽트 발생 시 timer_isr 호출
    // 셀렉터(GDT 코드 세그먼트) 0x08
    // type_attributes 0x8E (DPL 0, 32비트 인터럽트 게이트)
    
    idt_set_gate(0x20, (uint32_t)timer_isr, 0x08, 0x8E);

    // 키보드 인터럽트 설정(IRQ1) -> IDT 인덱스 0x21에 매핑됨
    // 키보드 인터럽트 발생 시 keyboard_isr 호출
    // 셀렉터(GDT 코드 세그먼트) 0x08
    // type_attributes 0x8E (DPL 0, 32비트 인터럽트 게이트)
    idt_set_gate(0x21, (uint32_t)keyboard_isr, 0x08, 0x8E);

    // IDT 로드
    idt_load((uint32_t)&idtp);
}

// 타이머 활성화 (마스크 해제)
void timer_init(void)
{
    // 현재 마스킹 정보를 읽어 mask 변수에 저장
    uint8_t mask = inb(PIC1_DATA);

    // IRQ0 마스킹 해제
    mask &= ~(1 << 0);

    // 마스터 PIC에 마스킹 정보 적용
    outb(PIC1_DATA, mask);
}

// 키보드 활성화 (마스크 해제)
void keyboard_init(void)
{
    // 현재 마스킹 정보를 읽어 mask 변수에 저장
    uint8_t mask = inb(PIC1_DATA);

    // IRQ1 마스킹 해제
    mask &= ~(1 << 1);

    // 마스터 PIC에 마스킹 정보 적용
    outb(PIC1_DATA, mask);
}
