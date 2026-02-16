#include "timer.h"
#include "io.h"

// PIT 채널 0의 입력 클럭
#define PIT_BASE_HZ 1193182

// PIT 채널 0을 원하는 주파수(Hz)로 설정하는 함수
void pit_set_frequency(uint32_t hz)
{
    // 하한 틱 : 1193182 / 65535 (16bit) = 약 18.2 Hz
    if (hz < 19) { hz = 19; }       // 주파수가 너무 낮으면 기본 틱 수준으로 설정
    if (hz > 1000) { hz = 1000; }   // 주파수가 너무 높으면 인터럽트 과부하 발생 가능하므로 제한

    // divisor 범위 : 16bit (1 ~ 0xFFFF)
    uint32_t divisor = PIT_BASE_HZ / hz;
    if (divisor < 1) { divisor = 1; }
    if (divisor > 0xFFFF) { divisor = 0xFFFF; }

    // Select channel : 00 = Channel 0
    // Access mode : 11 = lobyte/hibyte
    // Operating mode : 011 = Mode 3 (square wave generator)
    // BCD/Binary mode : 0 = 16-bit binary
    // 0011 0110 -> 0x36
    // https://wiki.osdev.org/Programmable_Interval_Timer 참고
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}