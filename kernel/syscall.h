#pragma once
#include <stdint.h>

// 앱/커널 공용 시스템 콜 번호 정의
typedef enum SyscallNum
{
    SYS_DEBUG_PUTS              = 1,
    SYS_GET_TICKS               = 2,
    SYS_KEY_DOWN                = 3,
    SYS_YIELD                   = 4,
    SYS_SLEEP                   = 5,
    SYS_GAME_CLEAR              = 6,
    SYS_GAME_FILL_RECT          = 7,
    SYS_PRESENT                 = 8,
    SYS_GAME_GET_SIZE           = 9,
    SYS_GAME_DRAW_TEXT          = 10,
    SYS_GAME_FILL_RECTS_BATCH   = 11
} SyscallNum;

typedef struct SyscallRect
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} SyscallRect;