/* Программа для отображения информации об адресах в памяти процесса */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Макроопределение для вывода адреса переменной */
#define SHW_ADR(ID, I) (printf("ID %s \t находится по виртуальному адресу: %8X\n", ID, &I))

/* Глобальные переменные для памяти процесса */
extern int etext, edata, end;

/* Глобальная переменная - указатель на строку */
char *cptr = "Это сообщение выводится функцией showit()\n";

/* Статическая строка и буфер для демонстрации */
char buffer1[25];

/* Прототип функции showit() */
int showit();

int main() {
  int i = 0; /* Локальная переменная */

  /* Вывод информации об адресах в памяти */
  printf("\nАдрес etext: %8X \n", &etext);
  printf("Адрес edata: %8X \n", &edata);
  printf("Адрес end  : %8X \n", &end);

  /* Использование макроса для вывода адресов различных элементов памяти */
  SHW_ADR("main", main);
  SHW_ADR("showit", showit);
  SHW_ADR("cptr", cptr);
  SHW_ADR("buffer1", buffer1);
  SHW_ADR("i", i);

  /* Копирование строки в буфер с использованием библиотечной функции */
  strcpy(buffer1, "Демонстрация\n");

  /* Системный вызов для вывода строки в стандартный вывод */
  write(1, buffer1, strlen(buffer1) + 1);

  /* Вызов функции showit() с передачей ей указателя на строку cptr */
  showit(cptr);

  return 0;
}

/* Определение функции showit() */
int showit(char *p) {
  char *buffer2;

  /* Использование макроса для вывода адреса переменной внутри функции */
  SHW_ADR("buffer2", buffer2);

  /* Выделение динамической памяти для копирования строки */
  if ((buffer2 = (char *)malloc((unsigned)(strlen(p) + 1))) != NULL) {
    printf("Выделена память по адресу %X\n", buffer2);

    /* Копирование строки в выделенную память */
    strcpy(buffer2, p);

    /* Вывод скопированной строки */
    printf("%s", buffer2);

    /* Освобождение выделенной памяти */
    free(buffer2);
  } else {
    printf("Ошибка выделения памяти\n");
    exit(1);
  }

  return 0;
}
