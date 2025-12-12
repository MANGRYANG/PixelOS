#include "graphics.h"
#include "../kernel/io.h"

static volatile uint8_t* const vga = (volatile uint8_t*)VGA_ADDR;

// 백 버퍼
static uint8_t backbuffer[WIDTH * HEIGHT];
// 그리기 대상 버퍼
static uint8_t* const draw = backbuffer;

// 화면 지우기 함수
void gfx_clear(uint8_t color)
{
    for (int i = 0; i < WIDTH * HEIGHT; ++i)
    {
        draw[i] = color;
    }
}

// 특정 좌표에 위치한 픽셀의 색을 설정하는 함수
void gfx_putpixel(int x, int y, uint8_t color)
{
    // 그릴 수 없는 위치의 픽셀인 경우 무시
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
    {
        return;
    }
    // 인자로 주어진 좌표의 픽셀의 색 변경
    draw[y * WIDTH + x] = color;
}

// 특정 좌표에 위치한 픽셀의 색을 읽어오는 함수
uint8_t gfx_getpixel(int x, int y)
{
    // 범위를 벗어난 픽셀인 경우 무시
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
    {
        return 0;
    }
    // 인자로 주어진 좌표의 픽셀의 색 반환
    return draw[y * WIDTH + x];
}

// 특정 좌표에서 w * h 크기의 사각형을 색으로 채워 그리는 함수
void gfx_fill_rect(int x, int y, int w, int h, uint8_t color)
{
    for (int row = 0; row < h; ++row)
    {
        // 현재 픽셀의 y좌표
        int current_y = y + row;
        // 화면을 벗어나는 경우 그리지 않음
        if (current_y < 0 || current_y >= HEIGHT) continue;

        for (int column = 0; column < w; ++column) {
            // 현재 픽셀의 x좌표
            int current_x = x + column;
            // 화면을 벗어나는 경우 그리지 않음
            if (current_x < 0 || current_x >= WIDTH) continue;

            // 이외의 경우 색 변경
            draw[current_y * WIDTH + current_x] = color;
        }
    }
}

// 특정 좌표에서 w * h 크기의 사각형을 테두리만 그리는 함수
void gfx_draw_rect(int x, int y, int w, int h, uint8_t color)
{
    // 단순 테두리만 그리기
    for (int column = 0; column < w; ++column)
    {
        gfx_putpixel(x + column, y, color);
        gfx_putpixel(x + column, y + (h - 1), color);
    }

    for (int row = 0; row < h; ++row)
    {
        gfx_putpixel(x, y + row, color);
        gfx_putpixel(x + (w - 1), y + row, color);
    }
}

// 백 버퍼의 내용을 VGA 메모리로 출력하는 함수
void gfx_present(void)
{
    gfx_wait_vsync();

    uint32_t* dst = (uint32_t*)vga;
    uint32_t* src = (uint32_t*)backbuffer;

    const int dwords = (WIDTH * HEIGHT) / 4;
    for (int i = 0; i < dwords; ++i)
    {
        dst[i] = src[i];
    }
}

// VSYNC 동기화 함수
void gfx_wait_vsync(void)
{
    while (inb(0x3DA) & 0x08) { }
    while (!(inb(0x3DA) & 0x08)) { }
}

// 백 버퍼 포인터 반환 함수
uint8_t* gfx_get_backbuffer(void)
{
    return backbuffer;
}