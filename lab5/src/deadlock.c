#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *arg) {
    // Блокировка mutex1
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Locked mutex1\n");

    // Задержка для демонстрации deadlock
    sleep(2);

    // Попытка блокировать mutex2
    printf("Thread 1: Trying to lock mutex2\n");
    pthread_mutex_lock(&mutex2);

    // Недостижимый код из-за deadlock
    printf("Thread 1: This code is unreachable\n");

    // Разблокировка mutex2
    pthread_mutex_unlock(&mutex2);

    // Разблокировка mutex1
    pthread_mutex_unlock(&mutex1);

    pthread_exit(NULL);
}

void *thread2(void *arg) {
    // Блокировка mutex2
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Locked mutex2\n");

    // Задержка для демонстрации deadlock
    sleep(2);

    // Попытка блокировать mutex1
    printf("Thread 2: Trying to lock mutex1\n");
    pthread_mutex_lock(&mutex1);

    // Недостижимый код из-за deadlock
    printf("Thread 2: This code is unreachable\n");

    // Разблокировка mutex1
    pthread_mutex_unlock(&mutex1);

    // Разблокировка mutex2
    pthread_mutex_unlock(&mutex2);

    pthread_exit(NULL);
}

int main() {
    pthread_t t1, t2;

    // Создание двух потоков
    if (pthread_create(&t1, NULL, thread1, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    if (pthread_create(&t2, NULL, thread2, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    // Ожидание завершения потоков
    if (pthread_join(t1, NULL) != 0) {
        perror("pthread_join");
        return 1;
    }

    if (pthread_join(t2, NULL) != 0) {
        perror("pthread_join");
        return 1;
    }

    return 0;
}
