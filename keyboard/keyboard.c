// kernel/keyboard.c

#include "keyboard.h"

// Scan code set 1 기준 ASCII 매핑 테이블
// 기능 키(Escape, Shift, Alt)는 0으로 매핑
static const char scancode_table[128] =
{
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',    // 0x00 - 0x0E
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,  // 0x0F - 0x1D
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',       // 0x1E - 0x2B
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',           // 0x2C - 0x39
    // 나머지는 0으로 초기화
};

// Shift 키가 눌렸을 때 사용할 매핑 테이블
// Shift 키가 눌렸을 때 변경되지 않는 키와 영문 키는 0으로 매핑
static const char scancode_table_shift[128] =
{
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,        // 0x00 - 0x0E
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '{', '}', 0, 0,                            // 0x0F - 0x1D
    0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '"', '~', 0, '|',                           // 0x1E - 0x2B
    0, 0, 0, 0, 0, 0, 0, '<', '>', '?', 0, '*', 0, ' ',                         // 0x2C - 0x39
};

// 키보드 입력 버퍼(원형 큐 구조)
#define KBD_BUFFER_SIZE 64
static char kbd_buffer[KBD_BUFFER_SIZE];
// buf_head: 데이터가 삽입될 위치를 표현
// buf_tail: 데이터가 삭제될 위치를 표현
static uint8_t buf_head = 0;
static uint8_t buf_tail = 0;

// 키의 상태를 저장할 배열
static bool key_state[128];

// Shift 키의 상태를 표현하기 위한 변수
static bool shift_pressed = false;
// Caps Lock의 적용 여부를 표현하기 위한 변수
static bool capslock_enabled = false;

// 키보드 상태 초기화 함수
void keyboard_reset_state(void)
{
    // 버퍼의 head와 tail을 0으로 초기화
    buf_head = 0;
    buf_tail = 0;
    // Shift 키의 상태 및 Caps Lock 적용 여부 초기화
    shift_pressed = false;
    capslock_enabled = false;

    // 모든 키의 상태를 false로 초기화
    for (int i = 0; i < 128; ++i)
    {
        key_state[i] = false;
    }
}

// 버퍼 push를 위한 내부 함수
static void buffer_push(char c)
{
    // buf_next: buf_head의 다음 위치를 표현
    uint8_t buf_next = (buf_head + 1) % KBD_BUFFER_SIZE;

    // buf_head를 제외한 모든 자리에 데이터가 차 있는 경우
    if (buf_next == buf_tail)
    {
        // 버림 처리
        return;
    }
    
    // 버퍼에 데이터 삽입
    kbd_buffer[buf_head] = c;
    // buf_head 갱신
    buf_head = buf_next;
}

// 버퍼에 문자가 있는지 체크하는 함수
bool keyboard_has_char(void)
{
    // buf_head와 buf_tail이 동일하면 버퍼가 비어 있는 상태
    return buf_head != buf_tail;
}

// 버퍼에서 문자를 반환하는 함수
char keyboard_get_char(void)
{
    // 버퍼가 비어 있는 경우 0 반환
    if (buf_head == buf_tail)
    {
        return 0;
    }
    
    // 버퍼에서 반환할 데이터를 임시 변수에 저장
    char c = kbd_buffer[buf_tail];
    // buf_tail의 위치를 다음 위치로 이동
    buf_tail = (buf_tail + 1) % KBD_BUFFER_SIZE;
    
    return c;
}

// 특정 스캔코드 눌림 여부 확인 함수
bool keyboard_is_key_down(uint8_t scancode)
{   
    if (scancode < 128)
        return key_state[scancode];
    return false;
}

// 스캔코드 -> ASCII 변환을 위한 내부 함수
static char translate_scancode(uint8_t code)
{
    char c = 0;

    // Shift 키가 눌림 상태인 경우
    if (shift_pressed)
    {
        // scancode_table_shift 테이블에서 변환
        c = scancode_table_shift[code];

        // shift 테이블에서 0으로 매핑되면 기본 테이블 사용
        if (!c)
        {
            c = scancode_table[code];
        }
    }

    // Shift 키가 눌림 상태가 아닌 경우
    else
    {
        // 기본 테이블을 사용해 ASCII 문자로 변환
        c = scancode_table[code];
    }

    // 영문 키의 경우 Shift와 Caps Lock을 조합하여 처리
    if (c >= 'a' && c <= 'z') {
        bool upper = (shift_pressed ^ capslock_enabled);    // XOR 연산
        // 대문자 변환 처리
        if (upper) {
            // 대문자와 소문자의 ASCII 값 차이: 32
            c = (char)(c - 32);
        }
    }

    return c;
}

// 인터럽트 핸들러에서 호출하는 함수
void keyboard_on_scancode(uint8_t scancode)
{
    // break code인지 확인 (최상위 비트가 1이면 break code)
    bool is_break = (scancode & 0x80) != 0;
    // make code로 변환
    uint8_t code = scancode & 0x7F;

    // 잘못된 스캔 코드 무시
    if (code >= 128)
    {
        return;
    }

    // break code인 경우 (키가 떼질 때 발생하는 신호인 경우)
    if (is_break)
    {
        // 키 떼짐
        key_state[code] = false;

        // Shift 키 신호인 경우
        if (code == 0x2A || code == 0x36)
        {
            // Shift 키의 상태를 false로 설정
            shift_pressed = false;
        }
    }

    // make code인 경우 (키가 눌릴 때 발생하는 신호인 경우)
    else {
        // 키 눌림
        key_state[code] = true;

        // Shift 키 신호인 경우
        if (code == 0x2A || code == 0x36)
        {
            // Shift 키의 상태를 true로 설정
            shift_pressed = true;
            return;
        }

        // Caps Lock 키 신호인 경우
        if (code == 0x3A)
        {
            // Caps Lock 상태를 변경
            capslock_enabled = !capslock_enabled;
            return;
        }

        // 실제 입력할 ASCII 문자로 변환
        char c = translate_scancode(code);
        
        // 버퍼 push
        if (c)
        {
            buffer_push(c);
        }
    }
}
