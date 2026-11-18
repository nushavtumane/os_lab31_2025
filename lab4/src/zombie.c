#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("Child process %d finished.\n", getpid());
        exit(0);
    } else if (pid > 0) {
        printf("Parent process %d created child %d.\n", getpid(), pid);
        sleep(20);
        wait(NULL);
        printf("Zombie process removed.\n");
    } else {
        perror("fork failed");
        return 1;
    }

    return 0;
}