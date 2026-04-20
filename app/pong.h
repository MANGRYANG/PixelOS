#pragma once

#include <stdint.h>

// 게임 상태 관리를 위한 구조체
typedef struct PongState
{
    // 게임 렌더링 영역의 너비 및 높이
    int game_w;
    int game_h;

    // 공의 현재 x, y 좌표
    int ball_curr_x;
    int ball_curr_y;

    // 공의 현재 속도
    int ball_velocity_x;
    int ball_velocity_y;

    // 공의 크기 (ball_size * ball_size)
    int ball_size;

    // 양쪽 패들의 y 좌표
    int left_paddle_y;
    int right_paddle_y;

    // 패들의 너비 및 높이
    int paddle_w;
    int paddle_h;

    // 키 입력이 있을 때 패들이 한 번에 이동하는 픽셀 수
    int paddle_speed;

    // player1, player2의 점수
    int player1_score;
    int player2_score;

    // 게임 오버 기준 점수
    int score_limit;
    // 게임 오버 여부
    int game_over;
    // 승자
    int winner;

    // 게임 일시정지 상태를 저장하는 멤버
    int paused;
    // 이전 프레임에 일시정지 키(SPACE)가 눌려 있었는지 확인하기 위한 멤버
    int space_was_down;
    // 이전 프레임에 재시작 키(R)가 눌려 있었는지 확인하기 위한 멤버
    int restart_was_down;
} PongState;

// usertext 영역으로 선언
// pong 게임의 메인 루프
__attribute__((section(".usertext"), noreturn))
void pong_main(void);