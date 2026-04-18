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

__attribute__((section(".usertext"), always_inline))
static inline void app_sleep(uint32_t ticks)
{
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_SLEEP), "c"(ticks), "d"(0), "b"(0)
        : "memory"
    );
}

__attribute__((section(".usertext"), always_inline))
static inline void app_game_clear(uint8_t color)
{
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_GAME_CLEAR), "c"((uint32_t)color), "d"(0), "b"(0)
        : "memory"
    );
}

__attribute__((section(".usertext"), always_inline))
static inline void app_game_fill_rect(
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    uint8_t color
)
{
    uint32_t pos = ((uint32_t)y << 16) | x;
    uint32_t size = ((uint32_t)h << 16) | w;

    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_GAME_FILL_RECT), "c"(pos), "d"(size), "b"((uint32_t)color)
        : "memory"
    );
}

__attribute__((section(".usertext"), always_inline))
static inline void app_present(void)
{
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(SYS_PRESENT), "c"(0), "d"(0), "b"(0)
        : "memory"
    );
}

__attribute__((section(".usertext"), always_inline))
static inline uint32_t app_game_get_size(void)
{
    uint32_t ret;

    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GAME_GET_SIZE), "c"(0), "d"(0), "b"(0)
        : "memory"
    );

    return ret;
}