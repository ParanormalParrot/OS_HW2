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
} shm_struct;

// Указатели на разделяемую память.
static shm_struct *shared_mem;


int n, m, k;
int shmid; // идентификатор разделяемой памяти.
int semid; // идентификатор семафора.

void cleanup() {
    shmdt(shared_mem);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
}

void signal_handler(int signal) {
    printf("Acquired signal %d\n", signal);
    cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Обработка аргументов командной строки.
    if (argc < 4) {
        printf("Invalid number of arguments\n");
        return 1;
    }
    m = atoi(argv[1]);
    n = atoi(argv[2]);
    k = atoi(argv[3]);

    if(m*n*k > 1000){
        printf("Input values are too big\n");
        return 1;
    }

    // Обработка сигналов.
    signal(SIGINT, signal_handler);

    // Создание семафора.
    key_t semkey = ftok("semfile", 1);
    semid = semget(semkey, 2, 0666 | IPC_CREAT);
    semctl(semid, 0, SETVAL, ARRAY_SIZE);

    // Создание разделяемой памяти.
    key_t shmkey = ftok("shmfile", 1);
    shmid = shmget(shmkey, sizeof(shm_struct), 0644 | IPC_CREAT);
    shared_mem = (shm_struct *) shmat(shmid, NULL, 0);
    close(shmid);
    for (int i = 0; i < m * n * k; ++i) {
        shared_mem->books[i] = rand() % 1000;
    }
    struct sembuf buf;

    // Создание дочерних процессов-студентов(их количество равно количеству рядов)
    for (int i = 0; i < m; i++) {
        if (fork() == 0) { // дочерний процесс.
            buf.sem_num = 0;
            buf.sem_op = -1;
            buf.sem_flg = 0;
            semop(semid, &buf, 1);
            // Процесс-студент создаёт подмассив массива книг.
            int row[n * k];
            for (int j = 0; j < n * k; ++j) {
                row[j] = shared_mem->books[j + i * n * k];
            }
            buf.sem_num = 0;
            buf.sem_op = 1;
            buf.sem_flg = 0;
            semop(semid, &buf, 1);
            // Сортировка выбором
            for (int j = 0; j < n * k - 1; ++j) {
                buf.sem_num = 0;
                buf.sem_op = -1;
                buf.sem_flg = 0;
                semop(semid, &buf, 1);
                int min = j;
                for (int l = j + 1; l < n * k; ++l) {
                    if (row[l] < row[min]) {
                        min = l;
                    }
                }
                int tmp = row[j];
                row[j] = row[min];
                row[min] = tmp;
                printf("Student %d have inserted book %d at the position %d of the bookshelf %d in the row %d.\n", i + 1, row[j],
                       (j % k + 1), (j / n + 1), i + 1);
                usleep(rand()%10);
                buf.sem_num = 0;
                buf.sem_op = 1;
                buf.sem_flg = 0;
                semop(semid, &buf, 1);
            }
            buf.sem_num = 0;
            buf.sem_op = -1;
            buf.sem_flg = 0;
            semop(semid, &buf, 1);
            // Поток передаёт отсортированные значения обратно в массив  в разделённой памяти
            for (int j = 0; j < n * k; ++j) {
                shared_mem->books[j + i * n * k] = row[j];

            }
            buf.sem_num = 0;
            buf.sem_op = 1;
            buf.sem_flg = 0;
            semop(semid, &buf, 1);

            printf("Student %d have finished sorting his subcatalogue and passed it to the librarian.\n", i + 1);
            exit(0);
        }
    }


    for (int i = 0; i < m; i++) {
        buf.sem_num = 0;
        buf.sem_op = -1;
        buf.sem_flg = 0;
        semop(semid, &buf, 1);
    }

    // Ожидание завершения дочерних процессов.
    for (int i = 0; i < m; i++) {
        wait(NULL);
    }
    for (int i = 0; i < m * n * k - 1; ++i) {
        int min = i;
        for (int j = i + 1; j < m * n * k; ++j) {
            if (shared_mem->books[j] < shared_mem->books[min]) {
                min = j;
            }

        }
        int tmp = shared_mem->books[i];
        shared_mem->books[i] = shared_mem->books[min];
        shared_mem->books[min] = tmp;
    }


    printf("The librarian have completed the catalogue.\n");
    for (
            int i = 0;
            i < m * n * k;
            ++i) {
        printf("Book %d at the position %d of the bookshelf %d in the row %d.\n", shared_mem->books[i], (i % k)+1, (i / k % n)+1, (i / k / n)+1);

    }

// Освобождение ресурсов.
    cleanup();

    return 0;
}