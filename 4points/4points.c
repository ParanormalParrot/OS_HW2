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
#define ARRAY_SIZE 1000


// Структура для разделяемой памяти.
typedef struct {
    int books[ARRAY_SIZE];
} shm_struct;

// Указатели на разделяемую память и семафор.
static shm_struct *shared_mem;
static sem_t *semaphore;

int n, m, k;
int shmid; // идентификатор разделяемой памяти.


void cleanup() {
    // Удаляем семафор и разделяемую память.
    munmap(shared_mem, ARRAY_SIZE * sizeof(int));
    shm_unlink(SHM_NAME);
    sem_close(semaphore);
    sem_unlink(SEM_NAME);
}

void signal_handler(int signal) {
    printf("Received signal %d\n", signal);
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

    // Обработка сигналов.
    signal(SIGINT, signal_handler);

    // Создание семафора.
    semaphore = sem_open(SEM_NAME, O_CREAT, 0644, INT_MAX);

    // Создание разделяемой памяти.
    shmid = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    int ft2 = ftruncate(shmid, sizeof(shm_struct));
    shared_mem = mmap(NULL, sizeof(shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    close(shmid);
    for (int i = 0; i < m * n * k; ++i) {
        shared_mem->books[i] = rand() % 1000;
    }

    // Создание дочерних процессов-студентов(их количество равно количеству рядов)
    for (int i = 0; i < m; i++) {
        if (fork() == 0) { // дочерний процесс.
            sem_wait(semaphore);
            int row[n * k];
            for (int j = 0; j < n * k; ++j) {
                row[j] = shared_mem->books[j + i * n * k];
            }
            sem_post(semaphore);
            for (int j = 0; j < n * k - 1; ++j) {
                sem_wait(semaphore);
                int min = j;
                for (int l = j + 1; l < n * k; ++l) {
                    if (row[j] < row[l]) {
                        min = l;
                    }
                }
                int tmp = row[j];
                row[j] = row[min];
                row[min] = tmp;
                printf("Student %d have inserted book %d at the position %d of the bookshelf %d in the row %d.\n", i + 1, row[j],
                       (j % k + 1), (j / n + 1), i + 1);
                usleep(rand()%10);
                sem_post(semaphore);
            }
            sem_wait(semaphore);
            for (int j = 0; j < n * k; ++j) {
                shared_mem->books[j + i * n * k] = row[j];

            }
            sem_post(semaphore);

            printf("Student %d have finished sorting his subcatalogue and passed it to the librarian.\n", i + 1);
            exit(0);
        }
    }


    for (int i = 0; i < m; i++) {
        sem_post(semaphore);
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
        printf("Book %d at the position %d of the bookshelf %d in the row %d.\n", shared_mem->books[i], (i / m)+1, (i / n / m)+1, (i % k)+1);

    }

// Освобождение ресурсов.
    cleanup();

    return 0;
}