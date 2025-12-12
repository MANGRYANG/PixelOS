#pragma once

#include <stdint.h>

typedef enum {
    LAYER_DESKTOP,
    LAYER_WINDOW,
    LAYER_CURSOR,
} LayerType;

// 레이어 구조체 정의
typedef struct Layer {
    int x, y;
    int w, h;
    uint8_t* buffer;
    int z;
    LayerType type;
    int visible;
} Layer;
