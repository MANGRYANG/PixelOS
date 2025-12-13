#include "layer_manager.h"

// 최대 레이어 개수 설정
#define MAX_LAYERS 16

// 레이어를 저장할 배열 선언
static Layer* layers[MAX_LAYERS];
static int layer_count = 0;

// 레이어 초기화 함수
void layer_init(void)
{
    layer_count = 0;
}

// 레이어 추가 함수
void layer_add(Layer* layer)
{
    // 최대 레이어 개수를 넘는 레이어는 생성 불가
    if (layer_count >= MAX_LAYERS) return;
    layers[layer_count++] = layer;
}

// 레이어 삭제 함수
void layer_remove(Layer* layer)
{
    for (int i = 0; i < layer_count; ++i)
    {
        if (layers[i] == layer)
        {
            for (int j = i; j < layer_count - 1; ++j)
            {
                layers[j] = layers[j + 1];
            }
            --layer_count;
            
            return;
        }
    }
}

// 레이어 정렬 함수 (Insert Sort)
static void sort_layers(void)
{
    for (int i = 1; i < layer_count; ++i)
    {
        Layer* key = layers[i];
        int j = i - 1;
        while (j >= 0 && layers[j]->z > key->z)
        {
            layers[j + 1] = layers[j];
            j--;
        }
        layers[j + 1] = key;
    }
}

// 레이어 매니저가 관리 중인 모든 레이어를 반환하는 함수
// 인자로 받은 주소에 현재 관리 중인 레이어 수를 저장
Layer** layer_get_all(int* count)
{
    sort_layers();
    *count = layer_count;
    return layers;
}