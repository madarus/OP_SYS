#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>

#include "utils.h"

// Структура, представляющая аргументы для функции вычисления суммы
struct SumArgs {
    int *array; // Указатель на массив целых чисел
    int begin;  // Индекс начала подсчета суммы в массиве
    int end;    // Индекс конца подсчета суммы в массиве
};

// Функция вычисления суммы элементов массива в заданном диапазоне
long long int Sum(const struct SumArgs* args) {
    long long int sum = 0;

    // Проходим по элементам массива в заданном диапазоне и суммируем их
    for (int i = args->begin; i < args->end; i++) {
        sum += args->array[i];
    }

    return sum;
}

// Функция, выполняющаяся в отдельном потоке, вызывает Sum с переданными аргументами
void* ThreadSum(void* args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;
    return (void *)(long long int)Sum(sum_args);
}

int main(int argc, char **argv) {
    // Обработка опций командной строки с использованием getopt_long
    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"threads_num", required_argument, 0, 0},
                                      {0, 0, 0, 0}};
    int seed = -1;
    int array_size = -1;
    int threads_num = -1;

    while (true) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        if(c == -1) {
            break;
        }
        switch(c) {
            case 0: {
                // Обработка опций командной строки
                switch(option_index) {
                    case 0: {
                        seed = atoi(optarg);
                        if (seed < 0) {
                            printf("Seed must be a positive number or 0. Now seed is %d\n", seed);
                            return -1;
                        }
                        break;
                    } case 1: {
                        array_size = atoi(optarg);
                        if (array_size < 0) {
                            printf("Array_size must be a positive number. Now array_size is %d\n", array_size);
                            return -1;
                        }
                        break;
                    } case 2: {
                        threads_num = atoi(optarg);
                        if (threads_num <= 0) {
                            printf("Threads_num must be 1 or more. Now threads_num is %d\n", threads_num);
                            return -1;
                        }
                        break;
                    }
                }
                break;
            } case '?': {
                break;
            }
            default: {
                printf("getopt returned character code 0%o?\n", c);
            }
        }
    }
    
    // Проверка корректности введенных значений
    if (threads_num <= 0) {
        printf("Threads_num must be 1 or more. Now threads_num is %d\n", threads_num);
        return -1;
    }

    // Создание массива и заполнение его случайными значениями
    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    // Засекаем начальное время выполнения программы
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Подготовка аргументов для потоков
    struct SumArgs args[threads_num];
    args[0].begin = 0;

    // Создание и запуск потоков
    pthread_t threads[threads_num];
    for (uint32_t i = 0; i < threads_num; i++) {
        args[i].array = array;
        if (i != 0) {
            args[i].begin = args[i - 1].end;
        }

        if (args[i].begin == array_size) {
            break;
        }

        if (i != threads_num - 1) {
            args[i].end = (i + 1) * array_size / threads_num;
            if(args[i].end > array_size) {
                printf ("%d %d\n", args[i].begin, args[i].end);
                printf("SHIT!");
                return -1;
            }
        } else {
            args[i].end = array_size;
        }

        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)(args + i))) {
            printf("Error: pthread_create failed!\n");
            return 1;
        }
    }

    // Ожидание завершения выполнения всех потоков и суммирование результатов
    long long int total_sum = 0;
    for (uint32_t i = 0; i < threads_num; i++) {
        long long int sum = 0;
        pthread_join(threads[i], (void **)&sum);
        total_sum += sum;
    }

    // Засекаем конечное время выполнения программы
    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    // Вычисление времени выполнения и вывод результатов
    double elapsed_time = (finish_time.tv_usec - start_time.tv_usec) / 1000.0;
    free(array);
    printf("Total: %lld\n", total_sum);
    printf("It took %f milliseconds to find a sum\n", elapsed_time);
    return 0;
}
