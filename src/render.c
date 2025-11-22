// ============================================================================
// render.c
// ----------------------------------------------------------------------------
// 이 모듈은 게임 화면 출력(렌더링)을 전담한다.
//
// ■ 핵심 설계 개념
//   - Stage.map 은 '원본 맵 데이터'이므로 수정 금지
//   - 화면에 표시할 때는 단순히 출력만 수행 (레이어 개념)
//     1) 맵 타일 출력 (#, 공백 등)
//     2) 장애물(X) 출력 (맵 위 오버레이)
//     3) 골(G), 플레이어(P) 출력 (가장 위 레이어)
//
// ■ 왜 safe_mvaddch() 를 쓰는가? (중요!)
//   - 터미널 크기가 맵보다 작거나 리사이즈가 발생하는 경우,
//     mvaddch()가 화면 밖 좌표에 그리면 ncurses가 출력 실패 상태가 되면서
//     전체 화면이 사라지는 문제가 발생함
//   - 이를 방지하기 위해,
//       화면 범위 확인 → 범위 안일 때만 출력
//     기능을 safe_mvaddch() 로 제공
//
//   → 화면 크기 변화 문제를 해결하는 핵심 안전장치
//
// ============================================================================

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "../include/game.h"
#include "render.h"
#include "obstacle.h"

// ------------------------------------------------------------
// safe_mvaddch()
// ------------------------------------------------------------
// mvaddch() 호출 전에 화면 좌표가 터미널 범위를 벗어나지 않는지 검사
// ------------------------------------------------------------
void safe_mvaddch(int y, int x, char ch) {
    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w); // 현재 터미널 크기 읽기

    // 좌표가 화면 안에 있을 때만 출력 수행
    if (y >= 0 && y < term_h && x >= 0 && x < term_w) {
        mvaddch(y, x, ch);
    }
}


// ------------------------------------------------------------
// render()
// ------------------------------------------------------------
// 화면 전체를 그리는 함수 (프레임 기반 업데이트)
// - Stage/map 기반으로 맵 타일 출력
// - 장애물(X), 골(G), 플레이어(P) 레이어 순으로 출력
// - 상단 UI 표시
// ------------------------------------------------------------
void render(const Stage *stage, const Player *player, double elapsed_time,
            int current_stage, int total_stages)
{
    clear();   // 가상 화면 버퍼 초기화 (화면 전체 지움)

    // 상단 UI 정보 (스테이지 진행상황 + 시간)
    mvprintw(0, 0, "=== Stealth Adventure ===  Stage %d/%d   Time: %.2fs",
             current_stage, total_stages, elapsed_time);

    // ==========================
    // 1) 배경 맵 렌더링
    // ==========================
    for (int y = 0; y < stage->height; y++) {
        for (int x = 0; x < stage->width; x++) {
            safe_mvaddch(y + 1, x, stage->map[y][x]); // 1줄 띄워 UI 밑에 출력
        }
    }

    // ==========================
    // 2) 장애물 레이어 출력
    // ==========================
    for (int i = 0; i < stage->num_obstacles; i++) {
        Obstacle o = stage->obstacles[i];
        safe_mvaddch(o.y + 1, o.x, 'X');
    }

    // ==========================
    // 3) 골(G), 플레이어(P)
    // ==========================
    safe_mvaddch(stage->goal_y + 1, stage->goal_x, 'G');
    safe_mvaddch(player->y + 1, player->x, 'P');

    // 안내 메시지 (맵 하단)
    mvprintw(stage->height + 2, 0,
             "Controls: W/A/S/D move | Q Quit");

    refresh(); // 실제 화면으로 갱신 (렌더링 확정)
}
