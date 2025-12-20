#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define RECORD_FILE "assets/records.txt"

double load_best_record(void)
{

    int fd = open(RECORD_FILE, O_RDONLY);

    if (fd < 0)
        return 0.0;

    char buf[64];

    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    close(fd);

    if (n <= 0)
        return 0.0;

    buf[n] = '\0';
    return atof(buf);
}

// new_time(이번 플레이 시간)이 최고 기록보다 좋으면 갱신.
void update_record_if_better(double new_time)
{

    // 현재까지 저장된 기록 불러오기
    double best = load_best_record();

    // 기록이 있고, 기존 기록이 더 좋거나 같으면 갱신 불필요
    if (best > 0.0 && best <= new_time)
    {
        printf("Best Record: %.3fs | Your Time: %.3fs\n", best, new_time);
        return;
    }

    // 새 기록을 저장하기 위한 파일 열기
    int fd = open(RECORD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0)
    {
        perror("open");
        return;
    }

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%.3f\n", new_time);

    write(fd, buf, len);

    close(fd);

    if (best <= 0.0)
    {
        printf("First Record! Time: %.3fs\n", new_time);
    }
    else
    {
        printf("New Record! Old: %.3fs -> New: %.3fs\n", best, new_time);
    }
}
