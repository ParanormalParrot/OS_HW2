#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SHM_NAME "/shared_memory"
#define SEM_NAME "/semaphore"
#define ARRAY_SIZE 1024

// Структура для разделяемой памяти.
typedef struct {
int books[ARRAY_SIZE];
int m, n, k;
} shm_struct;

// Указатели на разделяемую память и семафор.
static shm_struct *shared_mem;
static sem_t *semaphore;

int row_index;
int shmid; // идентификатор разделяемой памяти.


int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Обработка аргументов командной строки.
    if (argc < 2) {
        printf("Invalid number of arguments\n");
        return 1;
    }
    row_index = atoi(argv[1]);


    // Создание семафора.
    semaphore = sem_open(SEM_NAME, O_RDWR);

    // Создание разделяемой памяти.
    shmid = shm_open("/shared_memory", O_RDWR, 0);
    shared_mem = mmap(NULL, sizeof(shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);


    // Создание дочерних процессов-студентов(их количество равно количеству рядов)
    sem_wait(semaphore);
    int m = shared_mem->m;
    int n = shared_mem->n;
    int k = shared_mem->k;
    // Процесс-студент создаёт подмассив массива книг.
    int row[n * k];
    for (int j = 0; j < n * k; ++j) {
        row[j] = shared_mem->books[j + row_index * n * shared_mem->k];
    }
    sem_post(semaphore);
    // Сортировка выбором
    for (int j = 0; j < n * k - 1; ++j) {
        sem_wait(semaphore);
        int min = j;
        for (int l = j + 1; l < n * k; ++l) {
            if (row[l] < row[min]) {
                min = l;
            }
        }
        int tmp = row[j];
        row[j] = row[min];
        row[min] = tmp;
        printf("Student %d have inserted book %d at the position %d of the bookshelf %d in a row %d. \n", row_index + 1,
               row[j],
               (j % k + 1), (j / n + 1), row_index + 1);
        usleep(rand() % 10);
        sem_post(semaphore);
    }
    sem_wait(semaphore);
    // Поток передаёт отсортированные значения обратно в массив  в разделённой памяти
    for (int j = 0; j < n * k; ++j) {
        shared_mem->books[j + row_index * n * k] = row[j];

    }
    sem_post(semaphore);

    printf("Student %d have finished sorting his subcatalogue and passed it to the librarian.\n", row_index + 1);
    exit(0);
}



