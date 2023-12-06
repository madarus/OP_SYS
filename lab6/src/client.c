#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "common/common.h"

// Структура, представляющая сервер с IP-адресом и портом
struct Server {
    char ip[255];
    int port;
};

// Структура, содержащая данные для каждого потока
struct ThreadData {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
    pthread_mutex_t *mutex; // Мьютекс для обеспечения безопасного доступа к общим данным
};

// Функция для умножения двух чисел по модулю
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }
    return result % mod;
}

// Функция для преобразования строки в беззнаковое 64-битное целое число
bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}

// Функция, которая будет выполняться в каждом потоке для взаимодействия с сервером
void *handle_server(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;

    // Создаем сокет
    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        pthread_exit(NULL);
    }

    // Настраиваем адрес сервера
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->server.port);
    if (inet_pton(AF_INET, data->server.ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address: %s\n", data->server.ip);
        pthread_exit(NULL);
    }

    // Подключаемся к серверу
    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection failed to %s:%d\n", data->server.ip, data->server.port);
        pthread_exit(NULL);
    }

    // Отправляем задание на сервер
    uint64_t task[3];
    task[0] = data->begin;
    task[1] = data->end;
    task[2] = data->mod;

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", data->server.ip, data->server.port);
        pthread_exit(NULL);
    }

    // Получаем ответ от сервера
    uint64_t response;
    if (recv(sck, &response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", data->server.ip, data->server.port);
        pthread_exit(NULL);
    }

    close(sck); // Закрываем сокет

    // Сохраняем результат в структуру и освобождаем мьютекс
    pthread_mutex_lock(data->mutex);
    data->result = response;
    pthread_mutex_unlock(data->mutex);

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers_file[256] = {'\0'};

    // Обработка аргументов командной строки
    while (true) {
        static struct option options[] = {{"k", required_argument, 0, 0},
                                          {"mod", required_argument, 0, 0},
                                          {"servers", required_argument, 0, 0},
                                          {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 0: {
                switch (option_index) {
                    case 0:
                        ConvertStringToUI64(optarg, &k);
                        break;
                    case 1:
                        ConvertStringToUI64(optarg, &mod);
                        break;
                    case 2:
                        strcpy(servers_file, optarg);
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
            } break;

            case '?':
                printf("Arguments error\n");
                break;
            default:
                fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    // Проверка наличия всех обязательных аргументов
    if (k == -1 || mod == -1 || !strlen(servers_file)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }

    // Считывание списка серверов из файла
    FILE *servers_fp = fopen(servers_file, "r");
    if (servers_fp == NULL) {
        fprintf(stderr, "Failed to open servers file: %s\n", servers_file);
        return 1;
    }

    int servers_num = 0;
    struct Server *servers = NULL;
    char line[255];

    // Парсинг файла серверов
    while (fgets(line, sizeof(line), servers_fp)) {
        servers_num++;
        servers = realloc(servers, sizeof(struct Server) * servers_num);

        char *ip = strtok(line, ":");
        char *port_str = strtok(NULL, " \n");
        servers[servers_num - 1].port = atoi(port_str);
        strcpy(servers[servers_num - 1].ip, ip);
    }

    fclose(servers_fp);

    // Создание потоков для каждого сервера
    pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    uint64_t size = k / servers_num;
    uint64_t begin = 1;
    uint64_t end = begin + size;

    struct ThreadData *thread_data = malloc(sizeof(struct ThreadData) * servers_num);

    for (int i = 0; i < servers_num; i++) {
        // Установка границ задачи для каждого сервера
        if(i == servers_num - 1) {
            end = k;
        }

        thread_data[i].server = servers[i];
        thread_data[i].begin = begin;
        thread_data[i].end = end;
        thread_data[i].mod = mod;
        thread_data[i].result = 0;
        thread_data[i].mutex = &mutex;

        begin = end + 1;
        end += size;

        // Создание потока для обработки задачи на сервере
        if (pthread_create(&threads[i], NULL, handle_server, &thread_data[i]) != 0) {
            fprintf(stderr, "Failed to create thread for server %s:%d\n", servers[i].ip, servers[i].port);
        }
    }

    // Ожидание завершения всех потоков
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
    }

    // Объединение результатов из разных серверов
    uint64_t total_result = 1;
    for (int i = 0; i < servers_num; i++) {
        printf("Server: %d, result: %lu\n", i, thread_data[i].result);
        total_result = MultModulo(total_result, thread_data[i].result, mod);
    }

    printf("Total result: %lu\n", total_result);

    // Освобождение ресурсов
    free(threads);
    free(thread_data);
    free(servers);
    pthread_mutex_destroy(&mutex);

    return 0;
}
