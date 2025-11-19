// input.h
// ----------------------------------------------------
// 터미널에서 키보드 입력을 비차단(non-blocking)으로 읽어오기 위한 인터페이스.
// - init_input: 터미널 모드 변경 (버퍼링/에코 끄기 등)
// - restore_input: 프로그램 종료 시 터미널 설정 복구
// - poll_input: 키가 눌렸는지 확인하고, 눌렸으면 해당 문자 반환

#ifndef INPUT_H
#define INPUT_H

// 터미널 입력 모드를 게임에 적합하게 초기화하는 함수.
// - 예: 리눅스/유닉스에서 termios를 사용해 canonical 모드 끄기, 에코 끄기,
//       입력을 즉시 읽을 수 있도록 설정 등.
// - 메인 함수에서 게임 시작 전에 한 번 호출.
void init_input(void);

// init_input에서 변경한 터미널 설정을 원래대로 되돌리는 함수.
// - 게임이 정상 종료되거나, 시그널 등으로 빠져나갈 때 호출해서
//   터미널이 망가지지 않도록 보호.
// - 보통 atexit()이나 시그널 핸들러에서 함께 사용.
void restore_input(void);

// 현재 키보드 입력을 비동기적으로 검사하는 함수.
// - 키가 눌리지 않았으면 -1을 반환.
// - 키가 눌렸으면 해당 키의 ASCII 코드(char)를 int로 반환.
// - 메인 게임 루프에서 매 프레임마다 호출해서 "현재 눌린 키"를 체크하는 용도로 사용.
//   예: int ch = poll_input(); if (ch == 'w') 위로 이동, 등.
int poll_input(void); // returns -1 if no key, otherwise char

#endif // INPUT_H
