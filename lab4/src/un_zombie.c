#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

//Пример исправления с использованием wait

int main() {
    pid_t child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Код дочернего процесса
        printf("Child process (PID=%d) is exiting\n", getpid());
        exit(EXIT_SUCCESS);
    } else {
        // Код родительского процесса
        printf("Parent process (PID=%d) is sleeping\n", getpid());
        sleep(10);  // Позволяет дочернему процессу завершиться

        // Родитель вызывает wait, чтобы избежать создания зомби-процесса
        wait(NULL);
        printf("Parent process (PID=%d) woke up\n", getpid());
    }

    return 0;
}
