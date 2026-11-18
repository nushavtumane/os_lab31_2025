#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

typedef struct {
    unsigned long long start;
    unsigned long long end;
    unsigned long long mod;
} ThreadData;

unsigned long long result = 1; 
pthread_mutex_t mutex;         

void* calculate_factorial(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    unsigned long long local_result = 1;

    for (unsigned long long i = data->start; i <= data->end; i++) {
        local_result = (local_result * (i % data->mod)) % data->mod;
    }

    pthread_mutex_lock(&mutex);
    result = (result * local_result) % data->mod;
    pthread_mutex_unlock(&mutex);

    free(data); 
    return NULL;
}

int main(int argc, char *argv[]) {
    unsigned long long k = 10;   
    int pnum = 4;                
    unsigned long long mod = 10; 

    static struct option long_options[] = {
        {"pnum", required_argument, 0, 'p'},
        {"mod",  required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "k:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'k': k = strtoull(optarg, NULL, 10); break;
            case 'p': pnum = atoi(optarg); break;
            case 'm': mod = strtoull(optarg, NULL, 10); break;
            default:
                fprintf(stderr, "Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (mod == 0) {
        printf("Modulus cannot be zero!\n");
        return 1;
    }

    pthread_t *threads = malloc(pnum * sizeof(pthread_t));
    pthread_mutex_init(&mutex, NULL);

    unsigned long long chunk_size = k / pnum;
    unsigned long long remainder = k % pnum;
    unsigned long long current_start = 1;

    for (int i = 0; i < pnum; i++) {
        ThreadData *data = malloc(sizeof(ThreadData));
        data->start = current_start;
        
        unsigned long long current_chunk = chunk_size + (i < remainder ? 1 : 0);
        data->end = current_start + current_chunk - 1;
        
        if (current_chunk == 0) data->end = data->start - 1; 
        
        data->mod = mod;

        if (pthread_create(&threads[i], NULL, calculate_factorial, data) != 0) {
            perror("pthread_create");
            exit(1);
        }

        current_start = data->end + 1;
    }

    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Result of (%llu!) mod %llu = %llu\n", k, mod, result);

    pthread_mutex_destroy(&mutex);
    free(threads);

    return 0;
}
