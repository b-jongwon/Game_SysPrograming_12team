// stage.h

// 스테이지 데이터를 외부 파일(예: 텍스트 파일)에서 읽어와
// Stage 구조체에 채워 넣는 기능을 제공하는 헤더.


#ifndef STAGE_H
#define STAGE_H

#include "../include/game.h"   // Stage 구조체 정의 사용


// - 인자 stage: 로드 결과를 저장할 Stage 구조체 포인터.
// - 인자 stage_id: 몇 번째 스테이지를 가져올지 지정 (1, 2, 3 ...).
int load_stage(Stage *stage, int stage_id);
int get_stage_count(void);

// CLI 인자로 전달된 맵 파일명을 스테이지 ID로 역매핑한다.
// - 인자가 "1f.map" 혹은 "assets/1f.map" 모두 허용한다.
// - 일치하는 항목이 없으면 -1을 반환하여 호출자가 사용자 피드백을 줄 수 있도록 한다.
int find_stage_id_by_filename(const char *filename);

#endif // STAGE_H
