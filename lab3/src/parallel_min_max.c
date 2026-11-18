#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> 

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

#define MAX_CHILDREN 256
pid_t child_pids[MAX_CHILDREN];
int pnum_global = 0;

// Обработчик сигнала SIGALRM
void timeout_handler(int sig) {
    printf("\n[TIMEOUT] Время вышло! Убиваем дочерние процессы...\n");
    for (int i = 0; i < pnum_global; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1; // Новый аргумент
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"timeout", required_argument, 0, 0}, // Добавили
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0: seed = atoi(optarg); break;
          case 1: array_size = atoi(optarg); break;
          case 2: pnum = atoi(optarg); break;
          case 3: timeout = atoi(optarg); break; // Обработка таймаута
          case 4: with_files = true; break;
        }
        break;
      case 'f': with_files = true; break;
      case '?': break;
    }
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  int pipes[pnum][2];
  int active_child_processes = 0;
  pnum_global = pnum;

  if (timeout > 0) {
      signal(SIGALRM, timeout_handler);
      alarm(timeout);
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    if (!with_files) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe failed");
        return 1;
      }
    }

    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        int chunk_size = array_size / pnum;
        int remainder = array_size % pnum;
        int begin = i * chunk_size + (i < remainder ? i : remainder);
        int end = begin + chunk_size + (i < remainder ? 1 : 0);

        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[30];
          sprintf(filename, "temp_min_max_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (f) {
            fprintf(f, "%d %d", local_min_max.min, local_min_max.max);
            fclose(f);
          }
        } else {
          close(pipes[i][0]);
          write(pipes[i][1], &local_min_max, sizeof(struct MinMax));
          close(pipes[i][1]);
        }
        exit(0); 
      } else {
        child_pids[i] = child_pid;
        if (!with_files) close(pipes[i][1]);
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Ожидание с WNOHANG
  while (active_child_processes > 0) {
    for (int i = 0; i < pnum; i++) {
        if (child_pids[i] > 0) {
            pid_t res = waitpid(child_pids[i], NULL, WNOHANG);
            if (res == child_pids[i]) {
                child_pids[i] = -1;
                active_child_processes -= 1;
            }
        }
    }
    usleep(10000); 
  }

  if (timeout > 0) alarm(0);

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[30];
      sprintf(filename, "temp_min_max_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f) {
        if (fscanf(f, "%d %d", &min, &max) == 2) {
            if (min < min_max.min) min_max.min = min;
            if (max > min_max.max) min_max.max = max;
        }
        fclose(f);
        remove(filename);
      }
    } else {
      struct MinMax local;
      if (read(pipes[i][0], &local, sizeof(struct MinMax)) == sizeof(struct MinMax)) {
          if (local.min < min_max.min) min_max.min = local.min;
          if (local.max > min_max.max) min_max.max = local.max;
      }
      close(pipes[i][0]);
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  if (min_max.min != INT_MAX) {
      printf("Min: %d\n", min_max.min);
      printf("Max: %d\n", min_max.max);
  } else {
      printf("Не удалось собрать результаты (процессы убиты).\n");
  }
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}