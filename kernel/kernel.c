#include <stdint.h>

void kernel_main(void) {
    // VGA 그래픽 메모리 시작
    uint8_t* vga = (uint8_t*)0xA0000;

    for(int i = 0; i < 320*200; ++i)
    {
        vga[i] = 0x0F;  // 흰색 픽셀
    }
}