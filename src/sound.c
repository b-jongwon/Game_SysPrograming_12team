#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// pid_t, fork, kill, waitpid, setpgid 사용을 위한 필수 헤더
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "sound.h"

// 백그라운드 BGM 프로세스의 PID를 저장할 전역 변수
static pid_t bgm_pid = -1;

// =========================================================================
// BGM 제어 함수 (Non-blocking)
// =========================================================================

/**
 * BGM을 백그라운드 프로세스로 실행합니다. (Execvp 방식)
 */
void play_bgm(const char *filePath, int loop)
{

    if (bgm_pid != -1)
    {
        fprintf(stderr, "BGM is already playing (PID: %d).\n", bgm_pid);
        return;
    }

    // fork() 시스템 콜을 사용하여 자식 프로세스 생성
    bgm_pid = fork();

    if (bgm_pid == 0)
    {
        // --- 자식 프로세스 (BGM 재생) 영역 ---

        setpgid(0, 0); // 새로운 프로세스 그룹의 리더가 됨

        // BGM 반복 재생은 system()이 가장 간결하므로,
        // 여기서는 다시 system() 방식의 안정성을 유지합니다.
        // (단, stop_bgm 로직은 kill(-pid)로 프로세스 그룹을 확실히 죽입니다.)
        while (1)
        {
            char command[256];
            // BGM 재생은 지연보다 안정성이 중요하므로 -B 옵션은 제외하고, 오류 출력만 막습니다.
            sprintf(command, "aplay -q %s 2> /dev/null", filePath);

            if (system(command) == -1)
            {
                perror("aplay command failed in BGM loop");
                break;
            }

            if (!loop)
                break;
        }

        exit(0);
    }
    else if (bgm_pid < 0)
    {
        perror("fork failed for BGM");
        bgm_pid = -1;
    }
}

/**
 * 백그라운드에서 재생 중인 BGM 프로세스를 종료합니다. (SIGKILL)
 */
void stop_bgm()
{
    if (bgm_pid > 0)
    {
        // SIGKILL(9) 사용: BGM 프로세스 그룹 전체를 강제 종료합니다.
        if (kill(-bgm_pid, SIGKILL) == 0)
        {
            printf("\nBGM process group (Root PID %d) forcibly terminated by SIGKILL.\n", bgm_pid);
        }
        else
        {
            perror("Error killing BGM process group");
        }

        waitpid(bgm_pid, NULL, 0);
        bgm_pid = -1;
    }
}

// =========================================================================
// SFX 재생 함수 (지연 최소화 적용)
// =========================================================================

/**
 * 짧은 효과음을 논블로킹(Non-blocking) 방식으로 백그라운드에서 재생합니다.
 * (Execvp 및 버퍼링 최소화 적용으로 딜레이 최소화)
 */
void play_sfx_nonblocking(const char *filePath)
{

    pid_t pid = fork();

    if (pid == 0)
    {
        // --- 자식 프로세스 영역 (SFX 재생) ---

        // 1. 새로운 프로세스 그룹 설정 (선택 사항이지만 안전함)
        setpgid(0, 0);

        // 2. aplay 명령의 인자 준비
        // ✅ [핵심 수정]: -B 1000 옵션을 추가하여 버퍼 크기를 최소화하고 지연을 줄입니다.
        // 2> /dev/null을 사용하여 오류 메시지 출력을 막습니다.
        char *aplay_args[] = {"aplay", "-q", "-B", "1000", (char *)filePath, (char *)NULL};

        // system() 대신 execvp()를 사용하여 쉘 오버헤드를 제거합니다.
        execvp(aplay_args[0], aplay_args);

        // execvp가 실패했을 경우만 실행됩니다.
        perror("Failed to execute aplay via execvp");
        exit(1);
    }
    else if (pid < 0)
    {
        perror("SFX fork failed");
    }
    // 부모 프로세스(메인 루프)는 즉시 리턴하여 작업을 계속합니다.
}

// =========================================================================
// 게임 오버 함수 (Blocking)
// =========================================================================

/**
 * 1. 일반 장애물 발각 시 소리 재생 (Blocking)
 */
void play_obstacle_caught_sound(const char *filePath)
{
    char command[256];

    sprintf(command, "aplay -q %s", filePath);

    printf("\n🔊 일반 장애물 사운드 재생: %s\n", filePath);

    if (system(command) == -1)
    {
        perror("Error executing sound command for obstacle");
    }
}

/**
 * 2. 교수님 발각 시 음성 재생 (TTS 파이프라인 구현, Blocking)
 */
void play_professor_caught_sound(const char *textFilePath)
{
    char tts_command[512];
    char message[256] = {0};
    FILE *fp;

    // 1. 텍스트 파일에서 교수님 메시지 읽어오기
    fp = fopen(textFilePath, "r");
    if (fp == NULL)
    {
        perror("Failed to open professor voice text file");
        return;
    }

    if (fgets(message, sizeof(message), fp) != NULL)
    {
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n')
        {
            message[len - 1] = '\0';
        }
    }
    fclose(fp);

    if (strlen(message) == 0)
    {
        fprintf(stderr, "Professor message file is empty.\n");
        return;
    }

    // 2. TTS 파이프라인 명령어 생성 (espeak -> aplay)
    snprintf(tts_command, sizeof(tts_command),
             "echo \"%s\" | espeak -ven+f1 -k1 -s130 --stdout | aplay -q",
             message);

    printf("\n📢 교수님 음성 (TTS) 재생: %s\n", message);

    // 3. system() 호출: 음성 재생이 끝날 때까지 블로킹
    if (system(tts_command) == -1)
    {
        perror("Error executing TTS pipeline command (espeak/aplay). Check if espeak is installed.");
    }
}