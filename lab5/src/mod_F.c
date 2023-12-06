#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex; // Мьютекс для синхронизации доступа к общим данным
unsigned long long result = 1; // Общий результат

// Структура для передачи аргументов в поток
typedef struct {
    int k;
    int mod;
    int start;
    int end;
} FactorialArgs;

// Функция для вычисления факториала в заданном диапазоне
void *calculateFactorial(void *args) {
    FactorialArgs *fArgs = (FactorialArgs *)args;
    unsigned long long localResult = 1;

    for (int i = fArgs->start; i <= fArgs->end; i++) {
        localResult = (localResult * i) % fArgs->mod;
    }

    // Захват мьютекса для обновления общего результата
    pthread_mutex_lock(&mutex);
    result = (result * localResult) % fArgs->mod;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int k = 0;
    int pnum = 1;
    int mod = 1;

    // Обработка входных параметров
    int opt;
    while ((opt = getopt(argc, argv, "k:p:m:")) != -1) {
        switch (opt) {
        case 'k':
            k = atoi(optarg);
            break;
        case 'p':
            pnum = atoi(optarg);
            break;
        case 'm':
            mod = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -k <number> -p <threads> -m <mod>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Invalid input parameters\n");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[pnum];
    pthread_mutex_init(&mutex, NULL);

    int chunk_size = k / pnum;
    int extra = k % pnum;

    // Создание потоков
    for (int i = 0; i < pnum; i++) {
        FactorialArgs *args = (FactorialArgs *)malloc(sizeof(FactorialArgs));

        args->k = k;
        args->mod = mod;
        args->start = i * chunk_size + 1;
        args->end = (i == pnum - 1) ? ((i + 1) * chunk_size + extra) : (i + 1) * chunk_size;

        if (pthread_create(&threads[i], NULL, calculateFactorial, args) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Ожидание завершения потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    pthread_mutex_destroy(&mutex);

    // Вывод результата
    printf("%d! mod %d = %llu\n", k, mod, result);

    return 0;
}