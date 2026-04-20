#include "pong.h"
#include "app_api.h"
#include "app_keys.h"
#include "../graphics/color.h"
#include <stdbool.h>

#define PADDLE_WIDTH 6
#define PADDLE_HEIGHT 28
#define PADDLE_OFFSET_X 8

#define CENTER_LINE_WIDTH 2
#define CENTER_LINE_HEIGHT 6
#define CENTER_LINE_GAP 4

#define BALL_SIZE 8

// pong 게임 초기화를 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_init(PongState* game)
{
    // 게임 렌더링 영역의 사이즈 가져오기
    uint32_t game_size = app_game_get_size();

    // game 객체의 게임 렌더링 영역 사이즈 초기화
    game->game_w = (int)(game_size & 0xFFFF);
    game->game_h = (int)(game_size >> 16);

    // game 객체의 공 관련 정보 초기화
    // 크기: 8, 시작 위치: 게임 렌더링 영역의 중앙, 속도: x, y축 방향으로 1
    game->ball_size = BALL_SIZE;
    game->ball_curr_x = (game->game_w - game->ball_size) / 2;
    game->ball_curr_y = (game->game_h - game->ball_size) / 2;
    game->ball_velocity_x = 1;
    game->ball_velocity_y = 1;

    // game 객체의 패들 크기, 속도 정보 초기화
    game->paddle_w = PADDLE_WIDTH;
    game->paddle_h = PADDLE_HEIGHT;
    game->paddle_speed = 2;

    // game 객체의 패들 y축 위치 정보 초기화 (중앙)
    game->left_paddle_y = (game->game_h - game->paddle_h) / 2;
    game->right_paddle_y = (game->game_h - game->paddle_h) / 2;

    game->player1_score = 0;
    game->player2_score = 0;
}

// pong 게임의 입력 관리를 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_handle_input(PongState* game)
{
    // Player 1: W/S 키로 왼쪽 패들 조작
    if (app_key_down(APP_KEY_W))
    {
        game->left_paddle_y -= game->paddle_speed;
    }

    if (app_key_down(APP_KEY_S))
    {
        game->left_paddle_y += game->paddle_speed;
    }

    // Player 2: 방향키 위/아래로 오른쪽 패들 조작
    if (app_key_down(APP_KEY_UP))
    {
        game->right_paddle_y -= game->paddle_speed;
    }

    if (app_key_down(APP_KEY_DOWN))
    {
        game->right_paddle_y += game->paddle_speed;
    }

    // 왼쪽 패들 경계값 처리
    if (game->left_paddle_y < 0)
    {
        game->left_paddle_y = 0;
    }
    else if (game->left_paddle_y + game->paddle_h > game->game_h)
    {
        game->left_paddle_y = game->game_h - game->paddle_h;
    }

    // 오른쪽 패들 경계값 처리
    if (game->right_paddle_y < 0)
    {
        game->right_paddle_y = 0;
    }
    else if (game->right_paddle_y + game->paddle_h > game->game_h)
    {
        game->right_paddle_y = game->game_h - game->paddle_h;
    }
}

// 두 사각형의 충돌 여부를 확인하는 내부 함수
__attribute__((section(".usertext")))
static int pong_rects_overlap(
    int ax, int ay, int aw, int ah,
    int bx, int by, int bw, int bh
)
{
    return (ax < (bx + bw)) && ((ax + aw) > bx) && (ay < (by + bh)) && ((ay + ah) > by);
}

// 공의 방향을 제어하기 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_reflect_ball(PongState* game, bool reverse_x, bool reverse_y)
{
    game->ball_velocity_x *= (reverse_x ? -1 : 1);
    game->ball_velocity_y *= (reverse_y ? -1 : 1);
}

// 공의 위치를 중앙으로 되돌리는 내부 함수
__attribute__((section(".usertext")))
static void pong_reset_ball(PongState* game, int direction)
{
    // 공 위치 재설정
    game->ball_curr_x = (game->game_w - game->ball_size) / 2;
    game->ball_curr_y = (game->game_h - game->ball_size) / 2;

    // 공의 이동 방향 설정
    game->ball_velocity_x = direction;
    game->ball_velocity_y = 1;
}

// pong 게임 상태 갱신을 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_update(PongState* game)
{
    // 패들의 x 좌표 (고정)
    const int left_paddle_x = PADDLE_OFFSET_X;
    const int right_paddle_x = game->game_w - (PADDLE_OFFSET_X + game->paddle_w);

    // 공의 현재 위치 갱신
    game->ball_curr_x += game->ball_velocity_x;
    game->ball_curr_y += game->ball_velocity_y;

    // 천장 충돌 감지
    if (game->ball_curr_y <= 0)
    {
        game->ball_curr_y = 0;
        pong_reflect_ball(game, false, true);
    }
    // 바닥 충돌 감지
    else if (game->ball_curr_y + game->ball_size >= game->game_h)
    {
        game->ball_curr_y = game->game_h - game->ball_size;
        pong_reflect_ball(game, false, true);
    }

    // 왼쪽 패들 충돌 감지
    if (game->ball_velocity_x < 0 &&
        pong_rects_overlap(
            game->ball_curr_x, game->ball_curr_y, game->ball_size, game->ball_size,
            left_paddle_x, game->left_paddle_y, game->paddle_w, game->paddle_h
        ))
    {
        game->ball_curr_x = left_paddle_x + game->paddle_w;
        pong_reflect_ball(game, true, false);
    }
    // 오른쪽 패들 충돌 감지
    else if (game->ball_velocity_x > 0 &&
        pong_rects_overlap(
            game->ball_curr_x, game->ball_curr_y, game->ball_size, game->ball_size,
            right_paddle_x, game->right_paddle_y, game->paddle_w, game->paddle_h
        ))
    {
        game->ball_curr_x = right_paddle_x - game->ball_size;
        game->ball_velocity_x = -game->ball_velocity_x;
    }

    // 왼쪽 패들을 지나치는 경우: 오른쪽 득점
    if (game->ball_curr_x + game->ball_size < 0)
    {
        if (game->player2_score < 10) {
            game->player2_score++;
        }
        pong_reset_ball(game, 1);
    }

    // 오른쪽 패들을 지나치는 경우: 왼쪽 득점
    if (game->ball_curr_x > game->game_w)
    {
        if (game->player1_score < 10) {
            game->player1_score++;
        }
        pong_reset_ball(game, -1);
    }
}
__attribute__((section(".usertext")))
static void pong_score_to_text(int score, char* out)
{
    // 점수가 0 이하인 경우 0으로 고정
    if (score < 0)
    {
        score = 0;
    }

    // 점수가 10 이상인 경우
    if (score >= 10)
    {
        out[0] = '1';
        out[1] = '0';
        out[2] = '\0';
    }
    // 점수가 10 이하인 경우 (0-9)
    else
    {
        out[0] = (char)('0' + score);
        out[1] = '\0';
    }
}

__attribute__((section(".usertext")))
static void pong_render(PongState* game)
{
    // 게임 화면 초기화
    app_game_clear(COLOR_BLACK);

    // 중앙 선 렌더링
    int center_x = (game->game_w - CENTER_LINE_WIDTH) / 2;
    for (int i = 0; i < game->game_h; i += (CENTER_LINE_HEIGHT + CENTER_LINE_GAP))
    {
        app_game_fill_rect(center_x, i, CENTER_LINE_WIDTH, CENTER_LINE_HEIGHT, COLOR_LIGHT_GRAY);
    }

    // 점수 UI 렌더링

    // 점수를 문자열로 변환하여 저장
    char score_player1[3];
    char score_player2[3];
    pong_score_to_text(game->player1_score, score_player1);
    pong_score_to_text(game->player2_score, score_player2);

    // 중앙선을 기준으로 대칭이 되어 출력하도록 설정
    // 한 자리 숫자일 경우, 중앙에서 (왼쪽으로 12 pixel + 글자 1개 너비(8) pixel) 띄운 위치에서 출력
    // 두 자리 숫자일 경우, 중앙에서 (왼쪽으로 12 pixel + 글자 2개 너비(16) pixel) 띄운 위치에서 출력
    app_game_draw_text(
        (game->player1_score < 10) ? (game->game_w / 2 - 20) : (game->game_w / 2 - 28),
        4,
        score_player1,
        COLOR_WHITE
    );
    // 중앙에서 오른쪽으로 13 pixel 띄워서 출력
    app_game_draw_text(game->game_w / 2 + 13, 4, score_player2, COLOR_WHITE);

    // 패들 렌더링
    app_game_fill_rect(PADDLE_OFFSET_X, game->left_paddle_y, game->paddle_w, game->paddle_h, COLOR_WHITE);
    app_game_fill_rect(game->game_w - (PADDLE_OFFSET_X + game->paddle_w), game->right_paddle_y, game->paddle_w, game->paddle_h, COLOR_WHITE);

    // 공 렌더링
    app_game_fill_rect(game->ball_curr_x, game->ball_curr_y, game->ball_size, game->ball_size, COLOR_LIGHT_GREEN);

    // 화면 합성
    app_present();
}

__attribute__((section(".usertext"), noreturn))
void pong_main(void)
{
    // 게임 상태 관리를 위한 객체 선언
    PongState game;

    // pong 게임 초기화
    pong_init(&game);

    // 게임 루프
    for (;;)
    {
        pong_handle_input(&game);
        pong_update(&game);
        pong_render(&game);

        app_sleep(1);
    }
}