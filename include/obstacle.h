#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <pthread.h>   
#include "../include/game.h"       
#include "../include/professor_pattern.h"


extern pthread_mutex_t g_stage_mutex;

void set_obstacle_player_ref( Player *p);

int start_obstacle_thread(Stage *stage);

void stop_obstacle_thread(void);


void move_obstacles(Stage *stage, double delta_time); 

int check_trap_collision(const Stage *stage, const Player *player); 


#endif // OBSTACLE_H
