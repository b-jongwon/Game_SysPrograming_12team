// projectile.c
#include "../include/game.h"
#include <stdio.h>

void fire_projectile(Stage *stage, const Player *player) // 플레이어 투사체 발사 함수
{
    if (!stage || !player)
        return;

    if (stage->remaining_ammo <= 0) // 현재 남은 탄약검사
    {
        return;
    }

    
    int slot_index = -1; //
    
    for (int i = 0; i < stage->num_projectiles; i++)  // 동시 발사 
    {
        if (stage->projectiles[i].active == 0) // Stage 구조체 생성될 때 
        {
            slot_index = i;
            break;
        }
    }

    // 빈 자리 없으면 배열 늘리기
    if (slot_index == -1)
    {
        if (stage->num_projectiles < MAX_PROJECTILES)
        {
            slot_index = stage->num_projectiles++;
        }
        else
        {
            return; 
        }
    }
  
    stage->remaining_ammo--;    // 탄약 1발 소모

    int dir_x = 0, dir_y = 0;   //플레이어가 보는 방향으로 투사체 방향 계산.
    switch (player->facing)
    {
    case PLAYER_FACING_UP:
        dir_y = -1;
        break;
    case PLAYER_FACING_DOWN:
        dir_y = 1;
        break;
    case PLAYER_FACING_LEFT:
        dir_x = -1;
        break;
    case PLAYER_FACING_RIGHT:
        dir_x = 1;
        break;
    default:
        return;
    }

    Projectile *p = &stage->projectiles[slot_index]; //투사체 정보 초기화.
    p->world_x = player->world_x;
    p->world_y = player->world_y;
    p->dir_x = dir_x;
    p->dir_y = dir_y;
    p->active = 1;
    p->distance_traveled = 0;
}

static int is_wall_cell(const Stage *stage, int tile_x, int tile_y)
{
    if (!stage)
        return 1;
    if (tile_x < 0 || tile_y < 0)
        return 1;

    int stage_width = (stage->width > 0) ? stage->width : MAX_X;
    int stage_height = (stage->height > 0) ? stage->height : MAX_Y;
    if (tile_x >= stage_width || tile_y >= stage_height)   //맵 끝에 도달하는지 검사
        return 1;

    char cell = stage->map[tile_y][tile_x];
    return is_tile_impassable_char(cell); // @,#,w,W,m,M,l,L은 물리적으로 막힘
}

void move_projectiles(Stage *stage)
{
    if (!stage)
        return;

    const int step = SUBPIXELS_PER_TILE; //1프레임당 이동거리 (1타일)
    int max_range_pixels = CONSTANT_PROJECTILE_RANGE * SUBPIXELS_PER_TILE; // 최대 사거리

    for (int i = 0; i < stage->num_projectiles; i++)
    {
        Projectile *p = &stage->projectiles[i];
        if (!p->active)
            continue;

        int next_world_x = p->world_x + p->dir_x * step;  //현재 투사체 위치 임시 저장
        int next_world_y = p->world_y + p->dir_y * step;

        p->distance_traveled += step; // 이동 거리 누적

        if (p->distance_traveled >= max_range_pixels)  // 사거리 초과 -> 소멸
        {
            p->active = 0; 
            continue;
        }

        int next_tile_x = next_world_x / SUBPIXELS_PER_TILE;
        int next_tile_y = next_world_y / SUBPIXELS_PER_TILE;

        if (is_wall_cell(stage, next_tile_x, next_tile_y)) //벽 충돌 검사
        {
            p->active = 0;
            continue;
        }

        // 장애물 충돌 체크
        for (int j = 0; j < stage->num_obstacles; j++)
        {
            Obstacle *o = &stage->obstacles[j];
            if (!o->active)
                continue;

            if (o->kind == OBSTACLE_KIND_PROFESSOR && stage->id != 6) // 6스테이지에서는 교수 맞추기 가능
                continue;  

            int obstacle_tile_x = o->world_x / SUBPIXELS_PER_TILE;
            int obstacle_tile_y = o->world_y / SUBPIXELS_PER_TILE;

            if (obstacle_tile_x == next_tile_x && obstacle_tile_y == next_tile_y) //장애물 충돌 검사
            {
                o->hp--;   //hp 1 감소
                if (o->hp <= 0)
                {
                    o->active = 0;  //hp 없으면 죽음
                }
                p->active = 0;
                break;
            }
        }

        if (!p->active)
            continue;

        p->world_x = next_world_x;
        p->world_y = next_world_y;
    }
}
