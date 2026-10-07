#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Дочерний процесс: заменяем его образ на sequential_min_max
        // Аргументы: путь, имя программы, seed, array_size, NULL
        execlp("./sequential_min_max", "sequential_min_max", "123", "1000", NULL);
        
        // Если exec вернулся, значит произошла ошибка
        perror("exec failed");
        exit(1);
    } else {
        // Родительский процесс ждет завершения ребенка
        wait(NULL);
        printf("Program finished.\n");
    }
    
    return 0;
}