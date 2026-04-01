#include "io.h"
#include "interrupts.h"
#include "task.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../font/font.h"
#include <stdint.h>

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
extern void mouse_isr(void);
extern void page_fault_isr(void);
extern void syscall_isr(void);

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

// 페이지 폴트 디버깅용
volatile uint32_t g_last_pf_addr = 0;
volatile uint32_t g_last_pf_err = 0;

// asm 코드에서 호출되는 타이머 핸들러 정의
void timer_handler(void)
{
    // 타이머 tick 증가
    ++g_timer_ticks;
}

// asm 코드에서 호출되는 키보드 핸들러 정의
void keyboard_handler(void)
{
    // PS/2 스캔코드를 읽기
    uint8_t scancode = inb(0x60);
    // 키보드 드라이브에 스캔 코드 전달   
    keyboard_on_scancode(scancode);
}

// asm 코드에서 호출되는 마우스 핸들러 정의
void mouse_handler(void)
{
    // PS/2 데이터를 읽기
    uint8_t data = inb(0x60);
    // 마우스 드라이브에 데이터 전달
    mouse_on_data(data);
}

// 32비트 비부호 정수를 16진수 문자열로 변환하는 내부 헬퍼 (에러 코드 출력용)
static void hex32_to_str(uint32_t v, char out[11])
{
    static const char* hex = "0123456789ABCDEF";
    out[0] = '0';
    out[1] = 'x';
    for (int i = 0; i < 8; ++i)
    {
        uint32_t shift = (7 - i) * 4;
        out[2 + i] = hex[(v >> shift) & 0xF];
    }
    out[10] = 0;
}

// asm 코드에서 호출되는 페이지 폴트 핸들러 정의
void page_fault_handler(uint32_t fault_addr, uint32_t error_code)
{
    g_last_pf_addr = fault_addr;
    g_last_pf_err = error_code;

    // 실행 중단
    __asm__ volatile ("cli");

    // 화면 초기화(빨간색)
    gfx_clear(COLOR_LIGHT_RED);

    // 에러 화면 구성
    put_string(8, 8,  "PAGE FAULT OUTBREAK!", COLOR_WHITE);
    put_string(8, 28, "fault_addr =", COLOR_WHITE);
    put_string(8, 48, "error_code =", COLOR_WHITE);

    // 페이지 폴트가 발생한 주소, 에러 코드를 저장할 버퍼
    char addr_buf[11];
    char err_buf[11];

    // 16진수 문자열로 변환
    hex32_to_str(fault_addr, addr_buf);
    hex32_to_str(error_code, err_buf);

    // 페이지 폴트가 발생한 주소, 에러 코드 출력
    put_string(120, 28, addr_buf, COLOR_WHITE);
    put_string(120, 48, err_buf,  COLOR_WHITE);

    // 백버퍼 내용을 실제 VGA로 반영
    gfx_present();
    
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}

// 시스템 콜 넘버 매핑용 Enum
enum
{
    SYS_DEBUG_PUTS = 1,
    SYS_GET_TICKS  = 2,
    SYS_KEY_DOWN   = 3,
    SYS_YIELD      = 4,
};

// asm 코드에서 호출되는 시스템 콜 핸들러 정의
uint32_t syscall_handler(uint32_t syscall_no, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    (void)arg1;
    (void)arg2;

    switch(syscall_no)
    {
        case SYS_DEBUG_PUTS:
        {
            const char* s = (const char*)arg0;

            // 기존 문자열 영역 지우기
            for (int i = 0; s[i] != '\0'; ++i)
            {
                remove_char(8 + i * 8, 8, COLOR_BLACK);
            }

            // 디버그 문자열 출력
            put_string(8, 8, s, COLOR_LIGHT_GREEN);
            gfx_present();

            return 0;
        }

        case SYS_GET_TICKS:
        {
            // tick 반환
            return g_timer_ticks;
        }

        case SYS_KEY_DOWN:
        {
            // 키 눌림 여부 반환
            return keyboard_is_key_down((uint8_t)arg0) ? 1u : 0u;
        }

        case SYS_YIELD:
        {
            // CPU 점유 양보
            task_yield();
            return 0;
        }

        default:
            return -1;
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

    // 마우스 인터럽트 설정(IRQ12) -> IDT 인덱스 0x2C에 매핑됨
    // 마우스 인터럽트 발생 시 mouse_isr 호출
    // 셀렉터(GDT 코드 세그먼트) 0x08
    // type_attributes 0x8E (DPL 0, 32비트 인터럽트 게이트)
    idt_set_gate(0x2C, (uint32_t)mouse_isr, 0x08, 0x8E);

    // 페이지 폴트 인터럽트 설정 -> IDT 인덱스 0x0E에 매핑됨
    // 페이지 폴트 발생 시 page_fault_isr 호출
    // 셀렉터(GDT 코드 세그먼트) 0x08
    // type_attributes 0x8E (DPL 0, 32비트 인터럽트 게이트)
    idt_set_gate(0x0E, (uint32_t)page_fault_isr, 0x08, 0x8E);

    // 시스템 콜 인터럽트 설정 -> IDT 인덱스 0x80에 매핑됨
    // 시스템 콜 발생 시 syscall_isr 호출
    // 셀렉터(GDT 코드 세그먼트) 0x08
    // type_attributes 0xEE (DPL 3, 32비트 인터럽트 게이트)
    idt_set_gate(0x80, (uint32_t)syscall_isr, 0x08, 0xEE);

    // IDT 로드
    idt_load((uint32_t)&idtp);
}

// 타이머 활성화 (마스크 해제)
void irq_enable_timer(void)
{
    // 현재 마스킹 정보를 읽어 mask 변수에 저장
    uint8_t mask = inb(PIC1_DATA);

    // IRQ0 마스킹 해제
    mask &= ~(1 << 0);

    // 마스터 PIC에 마스킹 정보 적용
    outb(PIC1_DATA, mask);
}

// 키보드 활성화 (마스크 해제)
void irq_enable_keyboard(void)
{
    // 현재 마스킹 정보를 읽어 mask 변수에 저장
    uint8_t mask = inb(PIC1_DATA);

    // IRQ1 마스킹 해제
    mask &= ~(1 << 1);

    // 마스터 PIC에 마스킹 정보 적용
    outb(PIC1_DATA, mask);
}

// 마우스 활성화 (마스크 해제)
void irq_enable_mouse(void)
{
    // 슬레이브 PIC 활성화를 위해 IRQ2 라인 마스킹 해제
    uint8_t mask_master = inb(PIC1_DATA);
    mask_master &= ~(1 << 2);
    outb(PIC1_DATA, mask_master);

    // 마우스 IRQ12 마스킹 해제
    uint8_t mask_slave = inb(PIC2_DATA);
    mask_slave &= ~(1 << 4);
    outb(PIC2_DATA, mask_slave);
}
