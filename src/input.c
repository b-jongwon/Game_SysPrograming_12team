#include <ncurses.h>


// ncurses의 getch()는 non-blocking (nodelay) 상태에서 ERR 또는 입력 문자를 반환한다.
int poll_input(void) {
    int ch = getch();   // 입력 없으면 ERR 반환
    if (ch == ERR)
        return -1;
    return ch;
}
