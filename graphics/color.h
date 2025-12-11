#pragma once

#include <stdint.h>

typedef uint8_t Color;

// VGA 256-color 팔레트 기본값 매핑
enum
{
    COLOR_BLACK             = 0x00,
    COLOR_BLUE              = 0x01,
    COLOR_GREEN             = 0x02,
    COLOR_CYAN              = 0x03,
    COLOR_RED               = 0x04,
    COLOR_MAGENTA           = 0x05,
    COLOR_BROWN             = 0x06,
    COLOR_LIGHT_GRAY        = 0x07,
    COLOR_DARK_GRAY         = 0x08,
    COLOR_LIGHT_BLUE        = 0x09,
    COLOR_LIGHT_GREEN       = 0x0A,
    COLOR_LIGHT_CYAN        = 0x0B,
    COLOR_LIGHT_RED         = 0x0C,
    COLOR_LIGHT_MAGENTA     = 0x0D,
    COLOR_YELLOW            = 0x0E,
    COLOR_WHITE             = 0x0F,
};