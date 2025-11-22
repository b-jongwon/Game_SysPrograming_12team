#include <stdio.h>    // printf, fprintf 등 표준 입출력
#include <stdlib.h>   // exit, atexit 등
#include <unistd.h>   // usleep, sleep 등
#include <sys/time.h> // struct timeval, gettimeofday
#include <ncurses.h>  // 화면 제어용 ncurses 라이브러리

// 게임 내부 도메인/모듈 헤더들
#include "../include/game.h" // "/home/baek/TestGameCPtest/include/game.h"     // Player, Stage 구조체 정의
#include "stage.h"           // load_stage
#include "player.h"          // init_player, move_player
#include "obstacle.h"        // 장애물 스레드 및 이동 처리
#include "render.h"          // ncurses 기반 렌더링
#include "timer.h"           // 시간 처리 유틸
#include "fileio.h"          // 기록 저장/불러오기
#include "input.h"           // 키 입력 처리(논블로킹)
#include "signal_handler.h"  // 안전 종료 위한 시그널 핸들러

// 외부 파일에서 정의된 함수들
extern int is_goal_reached(const Stage *stage, const Player *player);
extern int check_collision(const Stage *stage, const Player *player);

// 스테이지 개수
#define NUM_STAGES 5

int main(void)
{

    // ============================================================
    // 0. ncurses 시스템 초기화
    // ============================================================
    // ncurses는 '화면 전체를 가상 버퍼에 그려두었다가 refresh() 시점에 한번에 출력'하는 구조.
    // 따라서 render()에서 mvprintw/mvaddch를 정상 사용하려면
    // 여기에서 반드시 initscr()가 먼저 호출되어야 한다.
    initscr();   // 터미널을 ncurses 모드로 전환
    curs_set(0); // 커서 숨김 (게임 화면 깜빡임 방지)
    keypad(stdscr, TRUE);

    // ============================================================
    // 1. 시그널 핸들러/입력 초기화
    // ============================================================
    // Ctrl+C 같은 시그널을 받으면 g_running = 0 으로 바꿔
    // 모든 루프를 안전하게 종료시키도록 설정
    setup_signal_handlers();

    // noncanonical 터미널 입력 설정
    //  - 즉시 입력 처리 (엔터 없이)
    //  - echo 방지
    //  - non-blocking
    init_input();

    // ============================================================
    // 2. 전체 플레이 시간 측정 시작
    // ============================================================
    struct timeval global_start, global_end;
    gettimeofday(&global_start, NULL);
    int prev_h, prev_w;
    getmaxyx(stdscr, prev_h, prev_w);

    int cleared_all = 1; // 모든 스테이지 클리어 여부

    // ============================================================
    // 3. 스테이지 루프 (1~NUM_STAGES)
    // ============================================================
    for (int s = 1; s <= NUM_STAGES && g_running; s++)
    {

        Stage stage;

        // -------------------------------
        // 스테이지 파일 로드 (.map 파싱)
        // -------------------------------
        if (load_stage(&stage, s) != 0)
        {
            fprintf(stderr, "Failed to load stage %d\n", s);
            endwin(); // ncurses 모드 해제
            exit(1);
        }

        // -------------------------------
        // 플레이어 시작 위치 초기화
        // -------------------------------
        Player player;
        init_player(&player, &stage);

        // -------------------------------
        // 장애물 스레드 시작
        //   → stage->obstacles[] 를 계속 움직임
        //   → 메인 루프와 충돌하지 않게 mutex 필요
        // -------------------------------
        if (start_obstacle_thread(&stage) != 0)
        {
            fprintf(stderr, "Failed to start obstacle thread\n");
            endwin();
            exit(1);
        }

        // ============================================================
        // 4. 스테이지 내부 게임 루프
        // ============================================================
        struct timeval stage_start, now;
        gettimeofday(&stage_start, NULL);

        int stage_cleared = 0;
        int stage_failed = 0;

        while (g_running)
        {
            // 터미널 리사이즈 감지 처리
            int new_h, new_w;
            getmaxyx(stdscr, new_h, new_w);
            if (new_h != prev_h || new_w != prev_w)
            {
                endwin();
                refresh();
                clear();
                prev_h = new_h;
                prev_w = new_w;
            }
            // -------------------------------
            // 시간 업데이트
            // -------------------------------
            gettimeofday(&now, NULL);
            double elapsed = get_elapsed_time(stage_start, now);

            // =======================================================
            // 렌더링
            // =======================================================
            // 장애물 스레드와 동시에 Stage.map을 읽기 때문에
            // mutex로 보호해야 Race Condition 방지됨.
            pthread_mutex_lock(&g_stage_mutex);

            // system("clear") 사용 금지!
            // → ncurses clear()는 render() 안에서 이미 호출됨.
            // → 여기서 system("clear") 사용하면 화면이 사라짐.

            render(&stage, &player, elapsed, s, NUM_STAGES);

            pthread_mutex_unlock(&g_stage_mutex);

            // =======================================================
            // 충돌 체크 (플레이어 ↔ 장애물)
            // =======================================================
            pthread_mutex_lock(&g_stage_mutex);
            if (check_collision(&stage, &player))
            {
                stage_failed = 1;
                pthread_mutex_unlock(&g_stage_mutex);
                break; // 스테이지 실패 → 루프 종료
            }

            if (is_goal_reached(&stage, &player))
            {
                stage_cleared = 1;
                pthread_mutex_unlock(&g_stage_mutex);
                break; // 골 도달 → 루프 종료
            }
            pthread_mutex_unlock(&g_stage_mutex);

            // =======================================================
            // 입력 처리
            // =======================================================
            int key = poll_input(); // 없으면 -1

            if (key != -1)
            {
                if (key == 'q' || key == 'Q')
                {
                    // 전체 게임 종료 요청
                    g_running = 0;
                    break;
                }
                else
                {
                    // WASD 이동키 처리
                    pthread_mutex_lock(&g_stage_mutex);
                    move_player(&player, (char)key, &stage);
                    pthread_mutex_unlock(&g_stage_mutex);
                }
            }

            // 루프 속도 너무 빠르면 CPU 100% 먹으므로 텀 둠
            usleep(10000); // 10ms
        }

        // ============================================================
        // 5. 장애물 스레드 종료
        // ============================================================
        stop_obstacle_thread();

        // ============================================================
        // 6. 스테이지 결과 처리
        // ============================================================
        if (!g_running)
        {
            cleared_all = 0;
            break;
        }

        if (stage_failed)
        {
            mvprintw(stage.height + 4, 0,
                     "You were caught at Stage %d! Game Over.\n", s);
            refresh();
            cleared_all = 0;
            break;
        }

        if (stage_cleared)
        {
            mvprintw(stage.height + 4, 0, "Stage %d Cleared!\n", s);
            refresh();
            sleep(1);
        }
    }

    // ============================================================
    // 7. 전체 결과 출력
    // ============================================================
    clear();
refresh();

gettimeofday(&global_end, NULL);
double total_time = get_elapsed_time(global_start, global_end);

// 결과 헤더
mvprintw(2, 2, "===== GAME RESULT =====");

// 플레이 시간 표시
mvprintw(4, 2, "Total Playtime: %.3fs", total_time);

double best_time = load_best_record();

// 결과 메시지
if (cleared_all && g_running) {
    mvprintw(6, 2, "You cleared all stages!");
    if (best_time <= 0.0 || total_time < best_time) {
        mvprintw(7, 2, "🎉 New Record!");
    }
    update_record_if_better(total_time);
} else {
    mvprintw(6, 2, "You failed to clear all stages.");
    mvprintw(7, 2, "Record unchanged.");
}

// 최고 기록 표시
best_time = load_best_record();
mvprintw(9, 2, "Best Record: %.3fs", best_time);
mvprintw(10, 2, "Your Time  : %.3fs", total_time);

// 종료 안내
mvprintw(12, 2, "Press any key to q...");
refresh();

// 입력 대기 (q를 눌러야 종료)
nodelay(stdscr, FALSE);
while (getch() != 'q') {
    // 아무것도 안함 → q 누를 때까지 대기
}
    // ============================================================
    // 8. ncurses 종료
    // ============================================================
    // (이걸 호출해야 터미널이 원래 모드로 돌아감)
    restore_input(); // 터미널 원복 먼저!
    endwin();

    return 0;
}
