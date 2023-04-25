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
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_NAME "/shared_memory"
#define SEM_NAME "/semaphore"
#define ARRAY_SIZE 1000

// Структура для разделяемой памяти.
typedef struct {
int books[ARRAY_SIZE];
int m, n, k;
} shm_struct;

// Указатели на разделяемую память и семафор.
static shm_struct *shared_mem;


int row_index;
int shmid; // идентификатор разделяемой памяти.
int semid;

int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Обработка аргументов командной строки.
    if (argc < 2) {
        printf("Invalid number of arguments\n");
        return 1;
    }
    row_index = atoi(argv[1]);


    // Открытие семафора.
    key_t semkey = ftok("semfile", 1);
    semid = semget(semkey, 1, 0666);
    struct sembuf my_buf;

    // Открытие разделяемой памяти.
    key_t shmkey = ftok("shmfile", 1);
    shmid = shmget(shmkey, sizeof(shm_struct), 0644);
    shared_mem = (shm_struct *) shmat(shmid, NULL, 0);


    // Создание дочерних процессов-студентов(их количество равно количеству рядов)
    my_buf.sem_num = 0;
    my_buf.sem_op = -1;
    my_buf.sem_flg = 0;
    semop(semid, &my_buf, 1);
    int m = shared_mem->m;
    int n = shared_mem->n;
    int k = shared_mem->k;
    int row[n * k];;
    for (int j = 0; j < n * k; ++j) {
        row[j] = shared_mem->books[j + row_index * n * shared_mem->k];
    }
    my_buf.sem_num = 0;
    my_buf.sem_op = 1;
    my_buf.sem_flg = 0;
    semop(semid, &my_buf, 1);
    for (int j = 0; j < n * k - 1; ++j) {
        my_buf.sem_num = 0;
        my_buf.sem_op = -1;
        my_buf.sem_flg = 0;
        int min = j;
        for (int l = j + 1; l < n * k; ++l) {
            if (row[j] < row[l]) {
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
        my_buf.sem_num = 0;
        my_buf.sem_op = 1;
        my_buf.sem_flg = 0;
        semop(semid, &my_buf, 1);

    }
    my_buf.sem_num = 0;
    my_buf.sem_op = -1;
    my_buf.sem_flg = 0;
    for (int j = 0; j < n * k; ++j) {
        shared_mem->books[j + row_index * n * k] = row[j];

    }
    my_buf.sem_num = 0;
    my_buf.sem_op = 1;
    my_buf.sem_flg = 0;
    semop(semid, &my_buf, 1);

    printf("Student %d have finished sorting his subcatalogue and passed it to the librarian.\n", row_index + 1);
    exit(0);
}



