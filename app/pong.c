#include "pong.h"
#include "app_api.h"
#include "app_keys.h"
#include "../graphics/color.h"

#define PADDLE_WIDTH 6
#define PADDLE_HEIGHT 28
#define PADDLE_OFFSET_X 8

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
    game->ball_curr_x = game->game_w / 2;
    game->ball_curr_y = game->game_h / 2;
    game->ball_velocity_x = 1;
    game->ball_velocity_y = 1;

    // game 객체의 패들 크기, 속도 정보 초기화
    game->paddle_w = PADDLE_WIDTH;
    game->paddle_h = PADDLE_HEIGHT;
    game->paddle_speed = 2;

    // game 객체의 패들 y축 위치 정보 초기화 (중앙)
    game->left_paddle_y = (game->game_h - game->paddle_h) / 2;
    game->right_paddle_y = (game->game_h - game->paddle_h) / 2;
}

// pong 게임의 입력 관리를 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_handle_input(PongState* game)
{
    // W 키 입력 시
    if (app_key_down(APP_KEY_W))
    {
        game->left_paddle_y -= game->paddle_speed;
    }

    // S 키 입력 시
    if (app_key_down(APP_KEY_S))
    {
        game->left_paddle_y += game->paddle_speed;
    }

    // 경계값 처리
    if (game->left_paddle_y < 0)
    {
        game->left_paddle_y = 0;
    }
    else if (game->left_paddle_y + game->paddle_h > game->game_h)
    {
        game->left_paddle_y = game->game_h - game->paddle_h;
    }
}

// pong 게임 상태 갱신을 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_update(PongState* game)
{
    // 공의 현재 위치 갱신
    game->ball_curr_x += game->ball_velocity_x;
    game->ball_curr_y += game->ball_velocity_y;

    // 천장 및 바닥에 충돌한 경우 y축 방향 반전
    if (game->ball_curr_y <= 0)
    {
        game->ball_curr_y = 0;
        game->ball_velocity_y = -game->ball_velocity_y;
    }
    else if (game->ball_curr_y + game->ball_size >= game->game_h)
    {
        game->ball_curr_y = game->game_h - game->ball_size;
        game->ball_velocity_y = -game->ball_velocity_y;
    }

    // 양쪽 벽에 충돌한 경우 x축 방향 반전 (Baseline 코드이므로 임시로 추가하였음)
    if (game->ball_curr_x <= 0)
    {
        game->ball_curr_x = 0;
        game->ball_velocity_x = -game->ball_velocity_x;
    }

    if (game->ball_curr_x + game->ball_size >= game->game_w)
    {
        game->ball_curr_x = game->game_w - game->ball_size;
        game->ball_velocity_x = -game->ball_velocity_x;
    }
}

__attribute__((section(".usertext")))
static void pong_render(PongState* game)
{
    // 게임 화면 초기화
    app_game_clear(COLOR_BLACK);

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