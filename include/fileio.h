#ifndef FILEIO_H          
#define FILEIO_H          

// 현재 저장된 최고 기록을 파일에서 읽어 와서 double 형태로 반환하는 함수.
double load_best_record(void);

// 새로운 클리어 시간 new_time이 기존 최고 기록보다 더 좋으면 기록 업데이트 함수
void update_record_if_better(double new_time);

#endif 