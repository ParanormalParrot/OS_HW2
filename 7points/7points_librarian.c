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

    if(m*n*k > 1000){
        printf("Input values are too big\n");
        return 1;
    }

    // Обработка сигналов.
    signal(SIGINT, signal_handler);

    // Создание семафора.
    semaphore = sem_open(SEM_NAME, O_CREAT, 0644, ARRAY_SIZE);

    // Создание разделяемой памяти.
    shmid = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    int ft2 = ftruncate(shmid, sizeof(shm_struct));
    shared_mem = mmap(NULL, sizeof(shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    close(shmid);
    for (int i = 0; i < m * n * k; ++i) {
        shared_mem->books[i] = rand() % 1000;
    }
    shared_mem->m = m;
    shared_mem->n = n;
    shared_mem->k = k;


    // Создание дочерних процессов-студентов(их количество равно количеству рядов)
    for (int i = 0; i < m; i++) {
        if (fork() == 0) { // дочерний процесс.
            char str[sizeof(int)];
            sprintf(str, "%d", i);
            execl("./7points_student", "./7points_student", str, NULL);
            exit(0);
        }
    }
    sleep(3);


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

}