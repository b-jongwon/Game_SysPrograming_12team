#ifndef STAGE_H
#define STAGE_H

#include "../include/game.h"  



int load_stage(Stage *stage, int stage_id);
int get_stage_count(void);


int find_stage_id_by_filename(const char *filename);

#endif 
