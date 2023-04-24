// Неименованные семафоры POSIX и разделяемая память POSIX.
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define AREAS_NUMBER 10
#define SHM_NAME "/shared_memory"

// Структура для разделяемой памяти.
typedef struct {
    int areas[AREAS_NUMBER];
    int bear_index;
    sem_t semaphore;
} shm_struct;

// Указатель на разделяемую память.
static shm_struct *shared_mem;

int num_bees; // количество стай неправильных пчел.
int shmid; // идентификатор разделяемой памяти.

void cleanup() {
    // Удаляем семафор и разделяемую память.
    munmap(shared_mem, num_bees * sizeof(int));
    shm_unlink(SHM_NAME);
}

void signal_handler(int signal) {
    printf("Received signal %d\n", signal);
    cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Обработка аргументов командной строки.
    if (argc < 2) {
        printf("Specify number of bees\n");
        return 1;
    }
    num_bees = atoi(argv[1]);

    // Обработка сигналов.
    signal(SIGINT, signal_handler);

    // Создание семафора.
    sem_init(&shared_mem->semaphore, 1, num_bees);

    // Создание разделяемой памяти.
    shmid = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    int ft2 = ftruncate(shmid, sizeof(shm_struct));
    shared_mem = mmap(NULL, sizeof(shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
    close(shmid);
    for (int i = 0; i < num_bees; ++i) {
        shared_mem->areas[i] = 0;
    }
    shared_mem->bear_index = rand() % 10;
    printf("Bear area: %d\n", shared_mem->bear_index + 1);

    // Создание дочерних процессов-стай пчел.
    for (int i = 0; i < num_bees; i++) {
        if (fork() == 0) { // дочерний процесс.
            while (1) {
                sem_wait(&shared_mem->semaphore);
                for (int j = 0; j < AREAS_NUMBER; ++j) {
                    if (shared_mem->areas[j] == 0) {
                        if (j == shared_mem->bear_index) {
                            shared_mem->areas[j] = 1;
                            printf("[Bees %d]\tbear found and punished in area %d\n", i + 1, j + 1);
                        } else {
                            shared_mem->areas[j] = -1;
                            printf("[Bees %d]\tarea %d viewed, bear not found, returning to the hive\n", i + 1, j + 1);
                        }
                        break;
                    }
                    sleep(rand() % 3);
                }
                sem_post(&shared_mem->semaphore);
            }
            exit(0);
        }
    }
    for (int i = 0; i < num_bees; i++) {
        sem_post(&shared_mem->semaphore);
    }

    // Ожидание завершения дочерних процессов.
    for (int i = 0; i < num_bees; i++) {
        wait(NULL);
    }

    // Освобождение ресурсов.
    cleanup();
    return 0;
}