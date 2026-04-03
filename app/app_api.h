#pragma once

#include <stdint.h>
#include "../kernel/syscall.h"

// ring3 앱에서 사용할 시스템 콜 API
__attribute__((section(".usertext"), always_inline))
static inline int app_debug_puts(const char* s)
{
    uint32_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_DEBUG_PUTS), "c"((uint32_t)s), "d"(0), "b"(0)
        : "memory"
    );
    return (int)ret;
}

__attribute__((section(".usertext"), always_inline))
static inline uint32_t app_get_ticks(void)
{
    uint32_t ticks;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ticks)
        : "a"(SYS_GET_TICKS), "c"(0), "d"(0), "b"(0)
        : "memory"
    );
    return ticks;
}

__attribute__((section(".usertext"), always_inline))
static inline int app_key_down(uint8_t key)
{
    uint32_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_KEY_DOWN), "c"((uint32_t)key), "d"(0), "b"(0)
        : "memory"
    );
    return (int)ret;
}

__attribute__((section(".usertext"), always_inline))
static inline void app_yield(void)
{
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_YIELD), "c"(0), "d"(0), "b"(0)
        : "memory"
    );
}