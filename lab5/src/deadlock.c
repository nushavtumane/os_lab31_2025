#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> 

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_func(void* arg) {
    printf("Thread 1: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Locked mutex1.\n");

    sleep(1); 

    printf("Thread 1: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2); 
    printf("Thread 1: Locked mutex2.\n");

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

void* thread2_func(void* arg) {
    printf("Thread 2: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Locked mutex2.\n");

    sleep(1); 

    printf("Thread 2: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1); 
    printf("Thread 2: Locked mutex1.\n");

    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int main() {
    pthread_t t1, t2;

    printf("Starting threads...\n");
    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Finished (this will never be printed).\n");
    return 0;
}
