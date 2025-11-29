#include <pthread.h> // pthread_t, pthread_create, pthread_join, pthread_mutex_t 등
#include <unistd.h>  // usleep
#include <math.h>
#include <stdlib.h>

#include "../include/obstacle.h"       // move_obstacles, start/stop 함수 선언 및 g_stage_mutex extern
#include "../include/game.h"           // Stage, Obstacle, MAX_X, MAX_Y 등
#include "../include/signal_handler.h" // g_running 전역 플래그

// 전역 뮤텍스 정의.
// - obstacle.h에 extern으로 선언되어 있고,
//   여기서 실제 메모리를 하나 만들어 초기화.
pthread_mutex_t g_stage_mutex = PTHREAD_MUTEX_INITIALIZER;

// 장애물 스레드에서 접근할 현재 스테이지 포인터.
// - start_obstacle_thread에서 설정.
// - obstacle_thread_func에서 사용.
static Stage *g_stage = NULL;

// 장애물 스레드 핸들
static pthread_t g_thread;

// 스레드가 실행 중인지 여부 플래그
static int g_thread_running = 0;

// [추가] 장애물 스레드가 플레이어 위치를 참고하기 위한 포인터
static const Player *g_player_ref = NULL;

// [추가] 메인에서 플레이어 주소를 넘겨받는 함수 (obstacle.h에도 선언 필요)
void set_obstacle_player_ref(const Player *p)
{
    g_player_ref = p;
}

// ----------------------------------------------------------
// 내부 헬퍼 함수 1: 스피너 이동 로직
// ----------------------------------------------------------
static void update_spinner(Obstacle *o, Stage *stage)
{
    // 1. 각도 증가 (속도 조절: 0.2 라디안씩 회전)
    // angle_index를 실수형 각도 누적값처럼 활용하거나, 별도 float 필드를 쓸 수도 있지만
    // 여기서는 간단히 angle_index를 각도로 환산한다고 가정.
    double speed = 0.2;
    double current_angle = o->angle_index * speed;

    // 2. 새로운 위치 계산 (중심점 + 반지름 * 삼각비)
    int nx = o->center_x + (int)round(o->radius * cos(current_angle));
    int ny = o->center_y + (int)round(o->radius * sin(current_angle));

    // 3. 맵 경계 체크 (혹시 모를 에러 방지)
    if (nx > 0 && nx < MAX_X && ny > 0 && ny < MAX_Y)
    {
        // 벽이 아닐 때만 이동 (스피너는 벽을 뚫을지 말지 선택 가능, 여기선 안 뚫음)
        if (stage->map[ny][nx] != '#')
        {
            o->x = nx;
            o->y = ny;
        }
    }

    // 4. 다음 프레임을 위해 인덱스 증가
    o->angle_index++;
}

// ----------------------------------------------------------
// 내부 헬퍼 함수 2: 교수님 AI (추격) 로직
// ----------------------------------------------------------
static void update_professor(Obstacle *o, Stage *stage)
{
    if (!g_player_ref)
        return; // 플레이어 정보 없으면 아무것도 안 함

    // 1. 플레이어와의 거리 계산 (맨해튼 거리)
    int dx = g_player_ref->x - o->x;
    int dy = g_player_ref->y - o->y;
    int dist = abs(dx) + abs(dy);

    // 2. 시야 체크 및 상태 변경
    if (dist <= o->sight_range)
    {
        o->alert = 1; // 발견!
    }
    else
    {
        o->alert = 0; // 놓침 (또는 원래대로)
    }

    int nx = o->x;
    int ny = o->y;

    if (o->alert)
    {
        // [추격 모드]
        // X축 차이가 더 크면 X축으로, 아니면 Y축으로 이동 시도
        if (abs(dx) > abs(dy))
        {
            nx += (dx > 0) ? 1 : -1;
        }
        else
        {
            ny += (dy > 0) ? 1 : -1;
        }

        // 만약 가려는 곳이 벽이라면? (간단한 장애물 회피)
        if (stage->map[ny][nx] == '#')
        {
            // 막혔으니 다른 축으로 이동 시도
            if (nx != o->x)
            { // 원래 X로 가려다 막힘 -> Y로 변경
                nx = o->x;
                ny += (dy > 0) ? 1 : -1;
            }
            else
            { // 원래 Y로 가려다 막힘 -> X로 변경
                ny = o->y;
                nx += (dx > 0) ? 1 : -1;
            }
        }
    }
    else
    {
        // [순찰 모드]
        // 기존 선형(Linear) 이동 로직을 재사용하거나, 랜덤 배회
        // 여기서는 기존 코드를 약간 변형해 "좌우 순찰"을 한다고 가정
        nx += o->dir;

        // 경계나 벽 만나면 방향 반전
        if (nx <= 1 || nx >= MAX_X - 2 || stage->map[o->y][nx] == '#')
        {
            o->dir *= -1;
            nx = o->x + o->dir; // 반대 방향으로 재설정
        }
    }

    // 최종 위치 반영 (벽 체크 한 번 더 안전하게)
    if (stage->map[ny][nx] != '#')
    {
        o->x = nx;
        o->y = ny;
    }
}
// ----------------------------------------------------------
// move_obstacles()
// ----------------------------------------------------------
// 스테이지 내의 모든 장애물을 한 번씩 이동시키는 함수.
// - 각 장애물의 type(0: 수평, 1: 수직)과 dir(+1 / -1)을 사용해 한 칸 이동.
// - 맵 경계나 벽('@')에 닿으면 방향을 반대로 뒤집는다.
void move_obstacles(Stage *stage)
{
    for (int i = 0; i < stage->num_obstacles; i++)
    {
        Obstacle *o = &stage->obstacles[i];
        if (!o->active)
            continue;

        switch (o->kind)
        {
        case OBSTACLE_KIND_SPINNER:
            update_spinner(o, stage);
            break;

        case OBSTACLE_KIND_PROFESSOR:
            update_professor(o, stage);
            break;

        case OBSTACLE_KIND_LINEAR:
        default:
            // 기존 선형 이동 로직 유지
            if (o->type == 0)
            { // 가로
                o->x += o->dir;
                if (o->x <= 1 || o->x >= MAX_X - 2 || stage->map[o->y][o->x] == '#')
                {
                    o->dir *= -1;
                    o->x += o->dir;
                }
            }
            else
            { // 세로
                o->y += o->dir;
                if (o->y <= 1 || o->y >= MAX_Y - 2 || stage->map[o->y][o->x] == '#')
                {
                    o->dir *= -1;
                    o->y += o->dir;
                }
            }
            break;
        }
    }
}

// ----------------------------------------------------------
// obstacle_thread_func()
// ----------------------------------------------------------
// 실제로 장애물을 주기적으로 움직이는 스레드 함수.
// - g_running(게임 전체 실행 플래그)와 g_thread_running이 둘 다 1인 동안 반복.
// - mutex로 g_stage를 보호하면서 move_obstacles 호출.
// - usleep(120ms)로 이동 간격(애니메이션 속도) 조절.
static void *obstacle_thread_func(void *arg)
{
    (void)arg; // 인자를 사용하지 않으므로 경고 방지용 캐스팅

    while (g_running && g_thread_running)
    {

        // 스테이지 데이터 보호를 위해 mutex lock
        pthread_mutex_lock(&g_stage_mutex);

        if (g_stage)
        {
            // 현재 스테이지에 대해 장애물 한 번씩 이동
            move_obstacles(g_stage);
        }

        // 임계구역 끝
        pthread_mutex_unlock(&g_stage_mutex);

        // 120ms 정도 대기 → 장애물 이동 속도 결정
        usleep(120000);
    }

    return NULL; // 스레드 종료
}

// ----------------------------------------------------------
// start_obstacle_thread()
// ----------------------------------------------------------
// 장애물을 자동으로 움직이는 스레드 시작.
// - 인자 stage: 이 스레드가 다룰 현재 스테이지 포인터.
// - 성공 시 0, 실패 시 -1 반환.
int start_obstacle_thread(Stage *stage)
{

    // 전역 포인터에 현재 스테이지 등록
    g_stage = stage;

    // 스레드 실행 플래그 ON
    g_thread_running = 1;

    // 스레드 생성: obstacle_thread_func를 실행
    if (pthread_create(&g_thread, NULL, obstacle_thread_func, NULL) != 0)
    {
        // 실패 시 플래그 되돌리고 에러 리턴
        g_thread_running = 0;
        return -1;
    }

    return 0; // 성공
}

// ----------------------------------------------------------
// stop_obstacle_thread()
// ----------------------------------------------------------
// 현재 실행 중인 장애물 스레드를 안전하게 종료시키는 함수.
// - g_thread_running 플래그를 0으로 바꿔 루프를 끝내고,
//   pthread_join으로 스레드가 완전히 종료될 때까지 기다린 뒤,
//   g_stage 포인터를 NULL로 리셋.
void stop_obstacle_thread(void)
{

    // 스레드가 이미 안 돌고 있으면 할 일 없음
    if (!g_thread_running)
        return;

    // 루프를 빠져나오도록 플래그 OFF
    g_thread_running = 0;

    // 스레드가 종료될 때까지 대기 (리소스 정리)
    pthread_join(g_thread, NULL);

    // 현재 스테이지 참조 제거
    g_stage = NULL;
}
