// --------------------------------------------------------------
// render.c
// --------------------------------------------------------------
// 이 파일은 실제 화면 출력(렌더링)을 담당한다.
//
// 설계 방식:
//   1) Stage.map[y][x] 내용을 buffer[][] 에 복사
//      → 원본 Stage.map을 절대 수정하지 않기 위함
//
//   2) 장애물, 플레이어, 골 위치를 buffer 위에 덮어씀
//      → Layer 방식의 렌더링 (바닥 → 오브젝트)
//
//   3) buffer를 한 줄씩 화면에 출력
//
// 왜 buffer를 쓰는가?
//   - Stage.map은 게임의 원본 데이터
//   - 렌더링 중에 Stage.map을 수정하면 다음 tick에 문제가 생김
//   - 이를 방지하기 위해 buffer라는 “렌더링 전용 임시 캔버스”를 사용
//
#include <ncurses.h>
#include <stdio.h>     // printf, putchar
#include <string.h>    // strncpy

#include "../include/game.h"
#include "render.h"
#include "obstacle.h"


// --------------------------------------------------------------
// render()
// --------------------------------------------------------------
// 입력:
//   - stage : 현재 스테이지 상태 (맵 정보, 장애물, 골 위치 포함)
//   - player: 플레이어 상태(x,y)
//   - elapsed_time : 이번 스테이지에서 흘러간 시간
//   - current_stage / total_stages: 상단 UI 표시용
//
// 출력:
//   - 실제 화면(터미널)에 렌더링한다.
//
void render(const Stage *stage, const Player *player, double elapsed_time,
            int current_stage, int total_stages)
{
    clear();   // 가상 화면만 초기화

    // 상단 UI
    mvprintw(0, 0, "=== Stealth Adventure ===  Stage %d/%d   Time: %.2fs",
             current_stage, total_stages, elapsed_time);

    // 맵 출력
    for (int y = 0; y < stage->height; y++) {
        for (int x = 0; x < stage->width; x++) {
            mvaddch(y+1, x, stage->map[y][x]);
        }
    }

    // 장애물
    for (int i = 0; i < stage->num_obstacles; i++) {
        Obstacle o = stage->obstacles[i];
        mvaddch(o.y+1, o.x, 'X');
    }

    // 골
    mvaddch(stage->goal_y+1, stage->goal_x, 'G');

    // 플레이어
    mvaddch(player->y+1, player->x, 'P');

    // 안내 메시지
    mvprintw(stage->height + 2, 0,
             "Controls: W/A/S/D to move, q to quit.");

    refresh();   // 여기서 한 번에 출력 (깜빡임 제거)
}