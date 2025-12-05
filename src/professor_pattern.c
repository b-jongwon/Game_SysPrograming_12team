// src/professor_patterns.c



#include "../include/professor_pattern.h"
#include <stdio.h> 


typedef int (*PatternFunc)(Stage*, Obstacle*,  Player*, double);

static const PatternFunc kPatterns[] = {
    NULL,            // 0 (Not used)
    pattern_stage_1, // Stage 1
    pattern_stage_2, // Stage 2
    pattern_stage_3, // Stage 3
    pattern_stage_4, // Stage 4
    pattern_stage_5, // Stage 5
    pattern_stage_6  // Stage 6
};

int pattern_stage_1(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    
    return 1; 
}


int pattern_stage_2(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    
    return 1;
}


int pattern_stage_3(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    
    return 1;
}


int pattern_stage_4(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
   
    return 1;
}


int pattern_stage_5(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    
    return 1;
}


int pattern_stage_6(Stage *stage, Obstacle *prof,  Player *player, double delta_time)
{
    return 1;
}







