#pragma once
#include <stdint.h>

// 앱/커널 공용 시스템 콜 번호 정의
typedef enum SyscallNum
{
    SYS_DEBUG_PUTS = 1,
    SYS_GET_TICKS  = 2,
    SYS_KEY_DOWN   = 3,
    SYS_YIELD      = 4,
} SyscallNum;