#define _POSIX_C_SOURCE 199309L
#include <signal.h>
#include <stddef.h>
#include "../include/signal_handler.h"

volatile sig_atomic_t g_running = 1;

static void handle_signal(int signo)
{
    (void)signo;
    g_running = 0;
}

void setup_signal_handlers(void)
{

    struct sigaction sa;

    sa.sa_handler = handle_signal;

    sigemptyset(&sa.sa_mask);

    // 특별한 플래그 사용 안 함
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
