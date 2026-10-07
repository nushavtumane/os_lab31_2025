#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  // Парсинг аргументов командной строки
  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("Seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("Array size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("Pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          default:
            printf("Index %d is out of options\n", option_index);
            return 1;
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  // Выделение памяти и генерация массива
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  // Массив для дескрипторов pipe (только если не используем файлы)
  int pipes[pnum][2];

  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Создание дочерних процессов
  for (int i = 0; i < pnum; i++) {
    // Создаем pipe перед fork, если не используем файлы
    if (!with_files) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe failed");
        return 1;
      }
    }

    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // Успешный fork
      active_child_processes += 1;
      
      if (child_pid == 0) {
        // ================= ДОЧЕРНИЙ ПРОЦЕСС =================
        
        // Вычисляем границы для текущего процесса
        int chunk_size = array_size / pnum;
        int remainder = array_size % pnum;
        
        // Распределяем остаток: первые remainder процессов получают на 1 элемент больше
        int begin = i * chunk_size + (i < remainder ? i : remainder);
        int end = begin + chunk_size + (i < remainder ? 1 : 0);

        // Ищем min/max на своем участке
        struct MinMax local_min_max = GetMinMax(array, begin, end);

        if (with_files) {
          // Синхронизация через файлы
          char filename[30];
          sprintf(filename, "temp_min_max_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (f) {
            fprintf(f, "%d %d", local_min_max.min, local_min_max.max);
            fclose(f);
          } else {
            perror("fopen failed");
            exit(1);
          }
        } else {
          // Синхронизация через pipe
          close(pipes[i][0]); // Закрываем чтение в ребенке
          write(pipes[i][1], &local_min_max, sizeof(struct MinMax));
          close(pipes[i][1]); // Закрываем запись после отправки
        }
        
        exit(0); // Важно завершить дочерний процесс
      } else {
        // ================= РОДИТЕЛЬСКИЙ ПРОЦЕСС =================
        if (!with_files) {
          close(pipes[i][1]); // Закрываем запись в родителе
        }
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Ожидание завершения всех дочерних процессов
  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  // Сбор и обработка результатов
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // Чтение из файлов
      char filename[30];
      sprintf(filename, "temp_min_max_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f) {
        fscanf(f, "%d %d", &min, &max);
        fclose(f);
        remove(filename); // Удаляем временный файл
      }
    } else {
      // Чтение из pipe
      struct MinMax local;
      read(pipes[i][0], &local, sizeof(struct MinMax));
      close(pipes[i][0]); // Закрываем чтение
      min = local.min;
      max = local.max;
    }

    // Обновляем глобальные значения
    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}