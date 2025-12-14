#include "compositor.h"
#include "graphics.h"
#include "color.h"
#include "layer_manager.h"

void compositor_compose(void)
{
    gfx_clear(COLOR_LIGHT_GRAY);

    int count;
    // 레이어 매니저에서 관리 중인 모든 레이어를 z-index 순서로 읽기
    Layer** layers = layer_get_all(&count);

    for (int i = 0; i < count; ++i)
    {
        Layer* layer = layers[i];
        // 레이어가 비가시 설정인 경우 다음 레이어로 이동
        if (!layer->visible) continue;

        for (int y = 0; y < layer->h; ++y)
        {
            int sy = layer->y + y;
            // y축 범위가 화면 범위를 넘어서는 경우 무시
            if (sy < 0 || sy >= HEIGHT) continue;

            for (int x = 0; x < layer->w; ++x)
            {
                int sx = layer->x + x;
                // x축 범위가 화면 범위를 넘어서는 경우 무시
                if (sx < 0 || sx >= WIDTH) continue;

                // 레이어의 (x, y) 픽셀을 화면 백 버퍼에 합성
                uint8_t c = layer->buffer[y * layer->w + x];
                gfx_putpixel(sx, sy, c);
            }
        }
    }

    gfx_present();
}