#pragma once
#include "layer.h"

// 레이어 초기화 함수
void layer_init(void);
// 레이어 추가 함수
void layer_add(Layer* layer);
// 레이어 삭제 함수
void layer_remove(Layer* layer);
// 레이어 매니저가 관리하는 모든 레이어를 반환하는 함수
Layer** layer_get_all(int* count);