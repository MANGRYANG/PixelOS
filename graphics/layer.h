#pragma once

#include <stdint.h>

typedef enum {
    LAYER_WINDOW,
    LAYER_CURSOR,
} LayerType;

// 레이어 구조체 정의
typedef struct Layer {
    // 레이어 시작 위치
    int x, y;
    // 레이어 크기
    int w, h;
    uint8_t* buffer;
    // 레이어 간 z-index
    int z;
    // 레이어 타입
    LayerType type;
    // 레이어의 가시성 여부
    int visible;
} Layer;
