#pragma once

#include <stdint.h>

// 인자로 전달된 port에서 1바이트를 읽어 반환하는 함수 정의
static inline uint8_t inb(uint16_t port)
{
    // 반환값(포트에서 읽어 온 1바이트 값)을 저장할 변수
    uint8_t ret;
    
    // 내부 어셈블리 실행 및 삭제 금지 선언
    // ((port) -> DX) -> inb -> (AL -> ret)
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "dN"(port));
    
    return ret;
}

// 인자로 전달된 1바이트 value 값을 port에 전달하는 함수 정의
static inline void outb(uint16_t port, uint8_t value)
{
    // 내부 어셈블리 실행 및 삭제 금지 선언
    // ((value) -> AX) -> outb -> (DX -> port))
    __asm__ volatile ("outb %0, %1" : : "a"(value), "dN"(port));
}