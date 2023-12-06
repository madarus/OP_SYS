#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include "common/common.h"

// Функция для вычисления факториала в заданном интервале
uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;

    // Вычисление факториала в заданном интервале
    for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }

    return ans;
}

// Функция, которая будет выполняться в каждом созданном потоке
void *ThreadFactorial(void *args_ptr) {
    // Приведение аргументов к нужному типу
    struct FactorialArgs *args = (struct FactorialArgs *)args_ptr;
    
    // Вычисление факториала и возвращение результата
    uint64_t result = Factorial(args);
    return (void *)result;
}

int main(int argc, char **argv) {
    // Инициализация переменных для количества потоков и порта
    int tnum = -1;  // Количество потоков
    int port = -1;  // Порт, на котором сервер будет слушать подключения

    // Обработка аргументов командной строки
    while (true) {
        // Опции командной строки для порта и количества потоков
        static struct option options[] = {
                {"port", required_argument, 0, 0},
                {"tnum", required_argument, 0, 0},
                {0, 0, 0, 0},
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        // Обработка опций
        switch (c) {
            case 0: {
                switch (option_index) {
                    case 0:
                        port = atoi(optarg);
                        break;
                    case 1:
                        tnum = atoi(optarg);
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
            } break;

            case '?':
                printf("Unknown argument\n");
                break;
            default:
                fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    // Проверка наличия всех обязательных аргументов
    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
        return 1;
    }

    // Создание серверного сокета
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Can not create server socket!");
        return 1;
    }

    // Настройка параметров серверного адреса
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Установка опции SO_REUSEADDR для повторного использования порта
    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    // Привязка серверного сокета к адресу
    int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    if (err < 0) {
        fprintf(stderr, "Can not bind to socket!");
        return 1;
    }

    // Прослушивание подключений
    err = listen(server_fd, 128);
    if (err < 0) {
        fprintf(stderr, "Could not listen on socket\n");
        return 1;
    }

    // Вывод сообщения о готовности к приему подключений
    printf("Server listening at %d\n", port);

    // Бесконечный цикл обработки подключений
    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);

        // Принятие нового подключения
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            fprintf(stderr, "Could not establish new connection\n");
            continue;
        }

        // Бесконечный цикл обработки заданий от клиентов
        while (true) {
            struct FactorialArgs args;

            // Получение задания от клиента
            int read = recv(client_fd, &args, sizeof(args), 0);

            uint64_t total_end = args.end;

            // Проверки на корректность полученных данных
            if (!read)
                break;
            if (read < 0) {
                fprintf(stderr, "Client read failed\n");
                break;
            }
            if (read < sizeof(args)) {
                fprintf(stderr, "Client send wrong data format\n");
                break;
            }

            // Создание потоков для выполнения факториализации
            pthread_t threads[tnum];
            uint64_t chunk_size = (args.end - args.begin + 1) / tnum;

            uint64_t start = args.begin;
            uint64_t end = start + chunk_size - 1;

            uint64_t total = 1;

            // Создание потоков
            for (int i = 0; i < tnum; i++) {
                if (i == tnum - 1) {
                    end = total_end;
                }

                // Подготовка аргументов для потока
                struct FactorialArgs *thread_args = malloc(sizeof(struct FactorialArgs));
                if (thread_args == NULL) {
                    fprintf(stderr, "Error: failed to allocate memory for thread args\n");
                    return 1;
                }

                thread_args->begin = start;
                thread_args->end = end;
                thread_args->mod = args.mod;

                // Запуск потока
                if (pthread_create(&threads[i], NULL, ThreadFactorial, thread_args) != 0) {
                    fprintf(stderr, "Error: pthread_create failed!\n");
                    return 1;
                }

                start = end + 1;
                end = start + chunk_size - 1;
            }

            // Дожидаемся завершения всех потоков
            for (int i = 0; i < tnum; i++) {
                uint64_t result = 0;
                pthread_join(threads[i], (void **)&result);
                total = MultModulo(total, result, args.mod);
            }

            // Вывод общего результата
            printf("Total: %lu\n", total);

            // Отправка результата клиенту
            err = send(client_fd, &total, sizeof(total), 0);
            if (err < 0) {
                fprintf(stderr, "Can't send data to client\n");
                break;
            }
        }

        // Закрытие соединения с клиентом
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }

    return 0;
}