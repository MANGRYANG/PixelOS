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

#define SCORE_LIMIT 10

#define PLAYER_NONE 0
#define PLAYER_1 1
#define PLAYER_2 2

#define PONG_FRAME_TICKS 1u

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

    game->score_limit = SCORE_LIMIT;
    // 게임 진행 : 0, 게임 오버: 1
    game->game_over = 0;
    game->winner = PLAYER_NONE;

    // 게임 진행: 0, 일시정지: 1
    game->paused = 0;
    game->space_was_down = 0;
}

// pong 게임의 전역 입력 관리를 위한 내부 함수 (일시정지, 재시작)
__attribute__((section(".usertext")))
static void pong_handle_global_input(PongState* game)
{
    int restart_down = app_key_down(APP_KEY_RESTART);
    
    // 게임 종료 시 R 키를 누르는 순간 한 번만 재시작
    if (game->game_over && restart_down && !game->restart_was_down)
    {
        pong_init(game);
        return;
    }

    game->restart_was_down = restart_down;

    int space_down = app_key_down(APP_KEY_SPACE);

    // SPACE 키를 누르는 순간 한 번만 일시정지 상태 토글
    if (!game->game_over && space_down && !game->space_was_down)
    {
        game->paused = !game->paused;
    }

    // space_was_down 갱신
    game->space_was_down = space_down;
}

// pong 게임의 패들 조작을 위한 입력 관리를 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_handle_paddle_input(PongState* game)
{
    // 일시정지, 게임 종료 상태인 경우 패들 조작 불가 처리
    if (game->paused || game->game_over)
    {
        return;
    }

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

// pong 게임의 입력 관리를 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_handle_input(PongState* game)
{
    pong_handle_global_input(game);
    pong_handle_paddle_input(game);
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

// 득점 처리를 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_score_point(PongState* game, int player)
{
    if (player == PLAYER_1)
    {
        if (game->player1_score < game->score_limit)
        {
            game->player1_score++;
        }

        if (game->player1_score >= game->score_limit)
        {
            game->game_over = 1;
            game->winner = PLAYER_1;
            return;
        }

        // Player1이 득점한 경우 공은 Player2 방향으로 다시 시작
        pong_reset_ball(game, -1);
    }
    else if (player == PLAYER_2)
    {
        if (game->player2_score < game->score_limit)
        {
            game->player2_score++;
        }

        if (game->player2_score >= game->score_limit)
        {
            game->game_over = 1;
            game->winner = PLAYER_2;
            return;
        }

        // Player2가 득점한 경우 공은 Player1 방향으로 다시 시작
        pong_reset_ball(game, 1);
    }
}

// pong 게임 상태 갱신을 위한 내부 함수
__attribute__((section(".usertext")))
static void pong_update(PongState* game)
{
    // 게임이 일시정지 상태이거나 종료 상태인 경우 업데이트하지 않음
    if (game->game_over || game->paused)
    {
        return;
    }

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
        pong_reflect_ball(game, true, false);
    }

    // 왼쪽 패들을 지나치는 경우: Player2 득점
    if (game->ball_curr_x + game->ball_size < 0)
    {
        pong_score_point(game, PLAYER_2);
        return;
    }

    // 오른쪽 패들을 지나치는 경우: Player1 득점
    else if (game->ball_curr_x > game->game_w)
    {
        pong_score_point(game, PLAYER_1);
        return;
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

    // 점수가 99 이상인 경우 99로 고정
    if (score > 99)
    {
        score = 99;
    }

    // 점수가 10 이상인 경우 (10-99)
    if (score >= 10)
    {
        out[0] = (char)('0' + (score / 10));
        out[1] = (char)('0' + (score % 10));
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
static void pong_render_center_line(PongState* game)
{
    int center_x = (game->game_w - CENTER_LINE_WIDTH) / 2;
    for (int i = 0; i < game->game_h; i += (CENTER_LINE_HEIGHT + CENTER_LINE_GAP))
    {
        app_game_fill_rect(center_x, i, CENTER_LINE_WIDTH, CENTER_LINE_HEIGHT, COLOR_LIGHT_GRAY);
    }
}

__attribute__((section(".usertext")))
static void pong_render_score(PongState* game)
{
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
}

__attribute__((section(".usertext")))
static void pong_render_paddles(PongState* game)
{
    app_game_fill_rect(PADDLE_OFFSET_X, game->left_paddle_y, game->paddle_w, game->paddle_h, COLOR_WHITE);
    app_game_fill_rect(game->game_w - (PADDLE_OFFSET_X + game->paddle_w), game->right_paddle_y, game->paddle_w, game->paddle_h, COLOR_WHITE);
}

__attribute__((section(".usertext")))
static void pong_render_ball(PongState* game)
{
    app_game_fill_rect(game->ball_curr_x, game->ball_curr_y, game->ball_size, game->ball_size, COLOR_LIGHT_GREEN);
}

__attribute__((section(".usertext")))
static void pong_render_overlay(PongState* game)
{
    // 게임이 종료된 상태인 경우 Game Over 메시지와 승자 출력
    if (game->game_over)
    {
        char winner_text[] = "WINNER";
        char player1_win[] = "Player1";
        char player2_win[] = "Player2";

        int winner_text_w = 6 * 8;
        int winner_text_h = 16;
        int player_win_w = 7 * 8;
        int player_win_h = 16;

        int winner_text_x = (game->game_w - winner_text_w) / 2;
        int winner_text_y = (game->game_h - winner_text_h) / 2 - 8;
        int player_win_x = (game->game_w - player_win_w) / 2;
        int player_win_y = (game->game_h - player_win_h) / 2 + 8;

        // 텍스트 및 텍스트 배경 박스 출력
        app_game_fill_rect(winner_text_x - 1, winner_text_y - 1, winner_text_w + 2, winner_text_h + player_win_h + 4, COLOR_BLACK);
        app_game_draw_text(winner_text_x, winner_text_y, winner_text, COLOR_YELLOW);
        app_game_draw_text(player_win_x, player_win_y, (game->winner == PLAYER_1) ? player1_win : player2_win, COLOR_YELLOW);
    }

    // 게임이 일시정지 상태인 경우 Paused 메시지 출력
    else if (game->paused)
    {
        char pause_text[] = "Paused";

        int pause_text_w = 6 * 8;
        int pause_text_h = 16;

        int pause_text_x = (game->game_w - pause_text_w) / 2;
        int pause_text_y = (game->game_h - pause_text_h) / 2;

        // 텍스트 및 텍스트 배경 박스 출력
        app_game_fill_rect(pause_text_x - 1, pause_text_y - 1, pause_text_w + 2, pause_text_h + 2, COLOR_BLACK);
        app_game_draw_text(pause_text_x, pause_text_y, pause_text, COLOR_WHITE);
    }
}

__attribute__((section(".usertext")))
static void pong_render(PongState* game)
{
    // 게임 화면 초기화
    app_game_clear(COLOR_BLACK);

    // 중앙 선 렌더링
    pong_render_center_line(game);

    // 점수 UI 렌더링
    pong_render_score(game);

    // 패들 렌더링
    pong_render_paddles(game);
    
    // 공 렌더링
    pong_render_ball(game);

    // 오버레이 렌더링
    pong_render_overlay(game);

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

    // 마지막으로 frame을 처리한 tick
    uint32_t last_frame_tick = app_get_ticks();

    // 게임 루프
    for (;;)
    {
        uint32_t now = app_get_ticks();

        // 아직 다음 frame을 처리할 tick이 되지 않았다면 CPU를 양보
        if ((uint32_t)(now - last_frame_tick) < PONG_FRAME_TICKS)
        {
            app_sleep(1);
            continue;
        }

        // 지연이 누적되었을 때 여러 frame을 몰아서 처리하지 않고,
        // 현재 tick 기준으로 다음 frame을 진행
        last_frame_tick = now;

        pong_handle_input(&game);
        pong_update(&game);
        pong_render(&game);

        app_sleep(1);
    }
}