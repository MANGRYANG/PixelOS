#include "io.h"
#include "interrupts.h"
#include "task.h"
#include "syscall.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../graphics/graphics.h"
#include "../graphics/color.h"
#include "../font/font.h"
#include "game_window.h"
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

// 커널 디버그 메시지 출력 함수 가져오기
extern void kernel_debug_puts(const char* s, uint32_t line);

// Metric count를 추적할 시스템 콜의 개수
#define SYSCALL_METRIC_COUNT 12
// Metric count를 출력할 주기
#define SYSCALL_METRIC_INTERVAL_TICKS 100u

// 각 시스템 콜에 대한 Metric count를 저장할 배열
static uint32_t g_syscall_metric_counts[SYSCALL_METRIC_COUNT];
// 마지막으로 Metric count를 출력한 시점의 타이머 값
static uint32_t g_syscall_metric_last_tick = 0;

// 문자열 합성을 위한 내부 함수
static char* append_str(char* ptr, const char* str)
{
    while (*str)
    {
        *ptr++ = *str++;
    }

    return ptr;
}

// 숫자 합성을 위한 내부 함수
static char* append_u32(char* ptr, uint32_t num)
{
    char temp[10];
    int i = 0;

    if (num == 0)
    {
        *ptr++ = '0';
        return ptr;
    }

    while (num > 0)
    {
        temp[i++] = (char)('0' + (num % 10));
        num /= 10;
    }

    while (i > 0)
    {
        *ptr++ = temp[--i];
    }

    return ptr;
}

// 시스템 콜 Metric count 수집을 위한 내부 함수
static void syscall_metric_count(uint32_t syscall_no)
{
    if (syscall_no < SYSCALL_METRIC_COUNT)
    {
        ++g_syscall_metric_counts[syscall_no];
    }
}

// 시스템 콜 Metric count 배열 초기화를 위한 내부 함수
static void syscall_metric_reset(void)
{
    for (uint32_t i = 0; i < SYSCALL_METRIC_COUNT; ++i)
    {
        g_syscall_metric_counts[i] = 0;
    }
}

// 시스템 콜 Metric count를 출력하는 함수
static void syscall_metric_report(void)
{
    uint32_t now = g_timer_ticks;

    // 최초 실행
    if (g_syscall_metric_last_tick == 0)
    {
        g_syscall_metric_last_tick = now;
        return;
    }

    // Interval 체크
    if ((now - g_syscall_metric_last_tick) < SYSCALL_METRIC_INTERVAL_TICKS)
    {
        return;
    }

    // 버퍼 준비
    char buf_0[64];
    char* p_0 = buf_0;

    // SYS_PRESENT 시스템 콜 호출 횟수는 P 뒤에 숫자로 표기
    p_0 = append_str(p_0, "[M] P");
    p_0 = append_u32(p_0, g_syscall_metric_counts[SYS_PRESENT]);

    // SYS_GAME_FILL_RECT 시스템 콜 호출 횟수는 F 뒤에 숫자로 표기
    p_0 = append_str(p_0, " F");
    p_0 = append_u32(p_0, g_syscall_metric_counts[SYS_GAME_FILL_RECT]);

    // SYS_KEY_DOWN 시스템 콜 호출 횟수는 K 뒤에 숫자로 표기
    p_0 = append_str(p_0, " K");
    p_0 = append_u32(p_0, g_syscall_metric_counts[SYS_KEY_DOWN]);

    // 널 종료 문자(\0) 추가
    *p_0 = 0;

    // 버퍼 준비
    char buf_1[64];
    char* p_1 = buf_1;

    // SYS_SLEEP 시스템 콜 호출 횟수는 S 뒤에 숫자로 표기
    p_1 = append_str(p_1, "[M] S");
    p_1 = append_u32(p_1, g_syscall_metric_counts[SYS_SLEEP]);

    // SYS_GET_TICKS 시스템 콜 호출 횟수는 G 뒤에 숫자로 표기
    p_1 = append_str(p_1, " G");
    p_1 = append_u32(p_1, g_syscall_metric_counts[SYS_GET_TICKS]);

    // SYS_GAME_DRAW_TEXT 시스템 콜 호출 횟수는 T 뒤에 숫자로 표기
    p_1 = append_str(p_1, " T");
    p_1 = append_u32(p_1, g_syscall_metric_counts[SYS_GAME_DRAW_TEXT]);

    // 널 종료 문자(\0) 추가
    *p_1 = 0;

    // 버퍼 준비
    char buf_2[64];
    char* p_2 = buf_2;

    // SYS_GAME_FILL_RECTS_BATCH 시스템 콜 호출 횟수는 B 뒤에 숫자로 표기
    p_2 = append_str(p_2, "[M] B");
    p_2 = append_u32(p_2, g_syscall_metric_counts[SYS_GAME_FILL_RECTS_BATCH]);

    // 널 종료 문자(\0) 추가
    *p_2 = 0;

    // 시스템 콜 Metric count 출력
    kernel_debug_puts(buf_0, 0);
    kernel_debug_puts(buf_1, 1);
    kernel_debug_puts(buf_2, 2);

    // 상태 초기화
    syscall_metric_reset();
    g_syscall_metric_last_tick = now;
}

extern uint8_t __user_data_start;
extern uint8_t __user_data_end;

// 포인터가 userdata 섹션 범위 안의 유효한 메모리 구간을 가리키는지 확인하는 함수
static int syscall_ptr_in_userdata(uint32_t ptr, uint32_t bytes)
{
    uint32_t start = (uint32_t)&__user_data_start;
    uint32_t end = (uint32_t)&__user_data_end;

    // 포인터가 userdata 섹션 범위를 벗어나는 경우 실패 처리
    if (ptr < start || ptr > end)
    {
        return 0;
    }

    // 요청한 바이트 수가 userdata 섹션을 넘어가면 실패 처리
    if (bytes > end - ptr)
    {
        return 0;
    }

    return 1;
}


// asm 코드에서 호출되는 시스템 콜 핸들러 정의
TrapFrame* syscall_handler(TrapFrame* frame, uint32_t syscall_no, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    syscall_metric_count(syscall_no);
    syscall_metric_report();

    switch(syscall_no)
    {
        case SYS_DEBUG_PUTS:
        {
            const char* s = (const char*)arg0;
            kernel_debug_puts(s, 0);

            frame->eax = 0;
            return frame;
        }

        case SYS_GET_TICKS:
        {
            // tick 반환
            frame->eax = g_timer_ticks;
            return frame;
        }

        case SYS_KEY_DOWN:
        {
            // 키 눌림 여부 반환
            frame->eax = keyboard_is_key_down((uint8_t)arg0) ? 1u : 0u;
            return frame;
        }

        case SYS_YIELD:
        {
            // CPU 점유 양보
            frame->eax = 0;
            return task_yield_from_user(frame);
        }

        case SYS_SLEEP:
        {
            // 현재 유저 태스크를 슬립 상태로 전환
            frame->eax = 0;
            return task_sleep_from_user(frame, arg0);
        }

        case SYS_GAME_CLEAR:
        {
            kernel_game_clear((uint8_t)arg0);
            frame->eax = 0;
            return frame;
        }

        case SYS_GAME_FILL_RECT:
        {
            uint16_t x = (uint16_t)(arg0 & 0xFFFF);
            uint16_t y = (uint16_t)((arg0 >> 16) & 0xFFFF);
            uint16_t w = (uint16_t)(arg1 & 0xFFFF);
            uint16_t h = (uint16_t)((arg1 >> 16) & 0xFFFF);
            uint8_t color = (uint8_t)arg2;

            kernel_game_fill_rect(x, y, w, h, color);

            frame->eax = 0;
            return frame;
        }

        case SYS_GAME_FILL_RECTS_BATCH:
        {
            const SyscallRect* rects = (const SyscallRect*)arg0;
            uint32_t count = arg1;
            uint8_t color = (uint8_t)arg2;

            if (count > 20u)
            {
                count = 20u;
            }

            if (!syscall_ptr_in_userdata(arg0, count * sizeof(SyscallRect)))
            {
                frame->eax = (uint32_t)-1;
                return frame;
            }

            kernel_game_fill_rects_batch(rects, count, color);

            frame->eax = 0;
            return frame;
        }

        case SYS_PRESENT:
        {
            kernel_game_present();
            frame->eax = 0;
            return frame;
        }

        case SYS_GAME_GET_SIZE:
        {
            frame->eax = kernel_game_get_size();
            return frame;
        }

        case SYS_GAME_DRAW_TEXT:
        {
            uint16_t x = (uint16_t)(arg0 & 0xFFFF);
            uint16_t y = (uint16_t)((arg0 >> 16) & 0xFFFF);

            uint8_t color = (uint8_t)(arg1 & 0xFF);
            const char* text = (const char*)arg2;

            kernel_game_draw_text(x, y, text, color);

            frame->eax = 0;
            return frame;
        }

        default:
            frame->eax = (uint32_t)-1;
            return frame;
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
