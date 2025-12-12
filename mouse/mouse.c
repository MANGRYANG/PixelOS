#include "mouse.h"
#include "../kernel/io.h"
#include "../graphics/graphics.h"

// 내부 마우스 상태
static int mouse_x = WIDTH / 2;
static int mouse_y = HEIGHT / 2;
static uint8_t mouse_buttons = 0;

// 3바이트 패킷 조립
static uint8_t packet[3];
static uint8_t packet_cycle = 0;

// 0x64 포트의 1번째 비트의 값이 해제될 때까지 대기
// 해제되었다면 0x64 포트로 명령을 전달할 수 있음
static void mouse_wait_send(void)
{
    while (inb(0x64) & 0x02);
}

// 0x64 포트의 0번째 비트의 값이 해제될 때까지 대기
// 해제되었다면 0x64 포트에서 바이트를 읽어올 수 있음
static void mouse_wait_receive(void)
{
    while (!(inb(0x64) & 0x01));
}

// 마우스 초기화 함수
void mouse_init(void)
{
    // -- Compaq 상태 설정/IRQ12 활성화 --

    // 명령을 보낼 수 있을 때까지 대기
    mouse_wait_send();
    // 현재 Compaq Status Byte를 포트 0x60로 내보내는 명령 전송
    outb(0x64, 0x20);   

    // 바이트를 읽어 올 수 있을 때까지 대기
    mouse_wait_receive();
    // 현재 Compaq Status Byte 읽기 
    uint8_t status = inb(0x60);
    // 현재 Compaq Status Byte의 비트 1을 1로 설정
    // IRQ12 활성화
    status |= 0x02;
    // 현재 Compaq Status Byte의 비트 5를 0으로 설정
    // 마우스 클럭 비활성화
    status &= ~0x20;

    // 명령을 보낼 수 있을 때까지 대기
    mouse_wait_send();
    // 다음 0x60에 쓸 값을 Compaq Status로 설정하는 명령 전송 
    outb(0x64, 0x60);

    // 명령을 보낼 수 있을 때까지 대기
    mouse_wait_send();
    // 수정된 Compaq Status Byte를 포트 0x60에 쓰기
    outb(0x60, status);

    // -- 보조 입력 활성화 명령 --

    // 명령을 보낼 수 있을 때까지 대기
    mouse_wait_send();
    // 보조 장치 활성화 명령 전송
    outb(0x64, 0xA8);  

    // -- PS/2 마우스 명령 --

    // 명령을 보낼 수 있을 때까지 대기
    mouse_wait_send();
    // 다음 0x60에 쓸 값을 마우스 명령으로 사용하도록 명령 전송
    outb(0x64, 0xD4);
    
    // 명령을 보낼 수 있을 때까지 대기
    mouse_wait_send();
    // 패킷 스트리밍 활성화 명령 전송
    outb(0x60, 0xF4);

    // 바이트를 읽어 올 수 있을 때까지 대기
    mouse_wait_receive();
    // 마우스로부터 ACK 응답 읽기
    inb(0x60);
}

// 인터럽트 핸들러에서 호출하는 마우스 데이터 전달 함수
void mouse_on_data(uint8_t data)
{
    if (packet_cycle == 0)
    {
        // 패킷의 첫 번째 바이트의 비트 3을 검사
        if (!(data & 0x08))
        {
            return;
        }
    }

    packet[packet_cycle++] = data;

    if (packet_cycle == 3)
    {
        packet_cycle = 0;
        mouse_on_packet(packet);
    }
}

// 3바이트 패킷이 완성되면 실행되는 함수
void mouse_on_packet(uint8_t p[3])
{
    int dx = (int8_t)p[1];
    int dy = (int8_t)p[2];
    
    if (dx < -50 || dx > 50 || dy < -50 || dy > 50)
    {
        return;
    }

    mouse_x += dx;
    // PS/2 마우스 - y축 반대 방향으로 이동
    mouse_y -= dy;

    // 화면 경계 체크
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= WIDTH)  mouse_x = WIDTH - 1;
    if (mouse_y >= HEIGHT) mouse_y = HEIGHT - 1;

    // 버튼 업데이트
    mouse_buttons = 0;
    if (p[0] & 0x01) mouse_buttons |= MOUSE_LEFT;
    if (p[0] & 0x02) mouse_buttons |= MOUSE_RIGHT;
    if (p[0] & 0x04) mouse_buttons |= MOUSE_MIDDLE;
}

// Getter 함수
int get_mouse_x(void) { return mouse_x; }
int get_mouse_y(void) { return mouse_y; }
uint8_t get_mouse_buttons(void) { return mouse_buttons; }
