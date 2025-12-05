// include/professor_patterns.h
// --------------------------------------------------------

#ifndef PROFESSOR_PATTERNS_H
#define PROFESSOR_PATTERNS_H

#include "../include/game.h"




int pattern_stage_1(Stage *stage, Obstacle *prof,  Player *player, double delta_time);


int pattern_stage_2(Stage *stage, Obstacle *prof,  Player *player, double delta_time);



int pattern_stage_3(Stage *stage, Obstacle *prof,  Player *player, double delta_time);


int pattern_stage_4(Stage *stage, Obstacle *prof,  Player *player, double delta_time);


int pattern_stage_5(Stage *stage, Obstacle *prof,  Player *player, double delta_time);


int pattern_stage_6(Stage *stage, Obstacle *prof,  Player *player, double delta_time);


int update_professor_pattern(Stage *stage, Obstacle *prof,  Player *player, double delta_time);

#endif 