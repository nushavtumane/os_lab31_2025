#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include "sum_lib.h"

// Генерация (проверь, чтобы совпадала с utils.c из 3 лабы, если надо)
void GenerateArray(int *array, unsigned int array_size, unsigned int seed) {
    srand(seed);
    for (unsigned int i = 0; i < array_size; i++) {
        array[i] = rand() % 100;
    }
}

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  static struct option options[] = {
      {"threads_num", required_argument, 0, 't'},
      {"seed", required_argument, 0, 's'},
      {"array_size", required_argument, 0, 'a'},
      {0, 0, 0, 0}
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
      switch (opt) {
          case 't': threads_num = atoi(optarg); break;
          case 's': seed = atoi(optarg); break;
          case 'a': array_size = atoi(optarg); break;
          default:
              fprintf(stderr, "Usage: %s --threads_num num --seed num --array_size num\n", argv[0]);
              return 1;
      }
  }

  if (threads_num == 0 || array_size == 0) {
      fprintf(stderr, "Error: threads_num and array_size must be > 0\n");
      return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed); 

  pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
  struct SumArgs *args = malloc(sizeof(struct SumArgs) * threads_num);

  int chunk_size = array_size / threads_num;
  int remainder = array_size % threads_num;
  int current_begin = 0;

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = current_begin;
    args[i].end = current_begin + chunk_size + (i < remainder ? 1 : 0);
    current_begin = args[i].end;

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1E9;

  free(array);
  free(threads);
  free(args);

  printf("Total: %d\n", total_sum);
  printf("Time: %f seconds\n", time_spent);

  return 0;
}