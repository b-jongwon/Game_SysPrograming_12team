#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// pid_t, fork, kill, waitpid 사용을 위한 필수 헤더
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h> // (추가될 수도 있음, 현재는 미사용)

#include "sound.h"

// 백그라운드 BGM 프로세스의 PID를 저장할 전역 변수
static pid_t bgm_pid = -1;

/**
 * BGM을 백그라운드 프로세스로 실행합니다. (Non-blocking)
 */
void play_bgm(const char *filePath, int loop)
{

   if (bgm_pid != -1)
   {
      fprintf(stderr, "BGM is already playing (PID: %d).\n", bgm_pid);
      return;
   }

   // 1. fork() 시스템 콜을 사용하여 자식 프로세스 생성
   bgm_pid = fork();

   if (bgm_pid == 0)
   {
      // --- 자식 프로세스 (BGM 재생) 영역 ---

      // 반복 재생 구현을 위해 system() 루프 사용
      while (1)
      {
         char command[256];
         // aplay -q [파일명]: 조용히 재생
         sprintf(command, "aplay -q %s", filePath);

         // system() 호출: 내부적으로 fork-exec-wait을 수행하여 소리가 끝날 때까지 기다림
         if (system(command) == -1)
         {
            // aplay 실행 실패 시 에러 출력 후 루프 탈출
            perror("aplay command failed in BGM loop");
            break;
         }

         if (!loop)
            break; // 반복 옵션이 없으면 한 번 재생 후 루프 탈출
      }

      // 자식 프로세스는 여기서 종료되어야 합니다.
      exit(0);
   }
   else if (bgm_pid < 0)
   {
      // Fork 실패
      perror("fork failed for BGM");
      bgm_pid = -1;
   }
   // --- 부모 프로세스 (메인 게임) 영역 ---
   // 즉시 리턴하여 메인 게임 루프 계속 진행
}

/**
 * 백그라운드에서 재생 중인 BGM 프로세스를 종료합니다.
 */
void stop_bgm()
{
   if (bgm_pid > 0)
   {
      // kill() 시스템 콜을 사용하여 SIGTERM (종료) 시그널을 보냄
      if (kill(bgm_pid, SIGTERM) == 0)
      {
         printf("\nBGM process (PID %d) terminated by SIGTERM.\n", bgm_pid);
      }
      else
      {
         perror("Error killing BGM process");
      }

      // waitpid()를 사용하여 자식 프로세스가 완전히 종료될 때까지 기다림 (정리 목적)
      waitpid(bgm_pid, NULL, 0);

      bgm_pid = -1; // PID 초기화
   }
}

/**
 * 1. 일반 장애물 발각 시 소리 재생 (Blocking)
 */
void play_obstacle_caught_sound(const char *filePath)
{
   char command[256];

   // aplay -q [파일명] (WAV 파일 재생)
   sprintf(command, "aplay -q %s", filePath);

   printf("\n🔊 일반 장애물 사운드 재생: %s\n", filePath);

   // system() 호출: 소리 재생이 끝날 때까지 메인 프로세스를 블로킹
   if (system(command) == -1)
   {
      perror("Error executing sound command for obstacle");
   }
}

/**
 * 2. 교수님 발각 시 음성 재생 (TTS 파이프라인 구현, Blocking)
 * textFilePath에서 메시지를 읽어 TTS로 변환 후 재생합니다.
 */
void play_professor_caught_sound(const char *textFilePath)
{
   char tts_command[512];
   char message[256] = {0}; // TTS 메시지를 저장할 버퍼
   FILE *fp;

   // 1. 텍스트 파일에서 교수님 메시지 읽어오기 (시스템 I/O 활용)
   fp = fopen(textFilePath, "r");
   if (fp == NULL)
   {
      perror("Failed to open professor voice text file");
      return;
   }

   // 파일의 첫 줄만 읽어와 메시지로 사용 (fgets: C 표준 I/O 함수)
   if (fgets(message, sizeof(message), fp) != NULL)
   {
      // 읽어온 문자열의 끝에 있는 개행 문자 제거
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
   // 파이프라인: echo "메시지" | espeak [옵션] --stdout | aplay -q
   // **경고:** espeak과 aplay가 설치되어 있어야 합니다.
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