#include <sys/stat.h> 
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <semaphore.h>

#define MEMORY_MAP_FILENAME "/lab5memoryfile"
#define SEM_JOBS_NAME "/lab5jobssem"
#define SEM_EMPTYSLOTS_NAME "/lab5emptyslotssem"
#define SEM_PROD_ADDR_MUTEX_NAME "/lab5prodmutex"
#define SEM_CONS_ADDR_MUTEX_NAME "/lab5consmutex"
#define TASK_LEN 10

void producer(sem_t* jobs_sem, sem_t* emptyslots_sem, char** ptr, sem_t* prod_mutex, long k, char* start_addr);
void consumer(sem_t* jobs_sem, sem_t* emptyslots_sem, char** ptr, sem_t* cons_mutex, long k, char* start_addr);

void intHandler(int dummy) {
    killpg(0, SIGTERM);
}

int main(int argc, char* argv[]) {
    shm_unlink(MEMORY_MAP_FILENAME);
    sem_unlink(SEM_JOBS_NAME);
    sem_unlink(SEM_EMPTYSLOTS_NAME);
    sem_unlink(SEM_PROD_ADDR_MUTEX_NAME);
    sem_unlink(SEM_CONS_ADDR_MUTEX_NAME);

    if (argc != 4) {
        exit(1);
    }

    char* end;
    long n = strtol(argv[1], &end, 10);
    long m = strtol(argv[2], &end, 10);
    long k = strtol(argv[3], &end, 10);

    int fd = shm_open(MEMORY_MAP_FILENAME, O_CREAT | O_RDWR, 0660);
    if (fd == -1) {
        perror("Nie udalo sie stworzyc segmentu pamieci");
        exit(1);
    }
    ftruncate(fd, 2 * sizeof(void *) + k * TASK_LEN);
    void* addr = mmap(NULL, 2 * sizeof(void *) + k * TASK_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    char** prod_ptr = addr;
    char** cons_ptr = addr + sizeof(void *);
    char* start_addr = (char*)addr + 2*sizeof(void *);
    *prod_ptr = start_addr;
    *cons_ptr = start_addr;

    sem_t* jobs_sem = sem_open(SEM_JOBS_NAME, O_CREAT, 0660, 0);
    sem_t* emptyslots_sem = sem_open(SEM_EMPTYSLOTS_NAME, O_CREAT, 0660, k);
    sem_t* prod_mutex = sem_open(SEM_PROD_ADDR_MUTEX_NAME, O_CREAT, 0660, 1);
    sem_t* cons_mutex = sem_open(SEM_CONS_ADDR_MUTEX_NAME, O_CREAT, 0660, 1);

    for (int i = 0; i < n; i++) {
        if (fork() == 0) {
            producer(jobs_sem, emptyslots_sem, prod_ptr, prod_mutex, k, start_addr);
        }
    }
    for (int i = 0; i < m; i++) {
        if (fork() == 0) {
            consumer(jobs_sem, emptyslots_sem, cons_ptr, cons_mutex, k, start_addr);
        }
    }
    signal(SIGINT, intHandler);
    pause();
}

void rand_str_task(char* buf) {
    for (int i = 0; i < TASK_LEN; i++) {
        buf[i] = rand() % 92 + 33; // ascii from 33 to 125
    }
}

void increment_loop(char** ptr, char* start_addr, long k) {
    *ptr = *ptr + TASK_LEN;
    if (*ptr - start_addr >= k * TASK_LEN) {
        *ptr = start_addr;
    }
}

void producer(sem_t* jobs_sem, sem_t* emptyslots_sem, char** ptr, sem_t* prod_mutex, long k, char* start_addr) {
    while (1) {
        char str[TASK_LEN];
        rand_str_task(str);

        if (sem_wait(emptyslots_sem) == -1) {
            perror("Blad w sem_wait w producencie");
        }

        if (sem_wait(prod_mutex) == -1) {
            perror("Blad w sem_wait mutex w producencie");
        }

        memcpy(*ptr, str, TASK_LEN);
        increment_loop(ptr, start_addr, k);

        if (sem_post(prod_mutex) == -1) {
            perror("Blad w sem_post mutex w producencie");
        }

        if (sem_post(jobs_sem) == -1) {
            perror("Blad w sem_post w producencie");
        }
    }
}

void consumer(sem_t* jobs_sem, sem_t* emptyslots_sem, char** ptr, sem_t* cons_mutex, long k, char* start_addr) {
    while (1) {
        char buf[TASK_LEN];

        if (sem_wait(jobs_sem) == -1) {
            perror("Blad w sem_wait w konsumencie");
        }

        if (sem_wait(cons_mutex) == -1) {
            perror("Blad w sem_wait mutex w konsumencie");
        }

        memcpy(buf, *ptr, TASK_LEN);
        increment_loop(ptr, start_addr, k);

        if (sem_post(cons_mutex) == -1) {
            perror("Blad w sem_post mutex w konsumencie");
        }

        if (sem_post(emptyslots_sem) == -1) {
            perror("Blad w sem_post w konsumencie");
        }
        for (int i = 0; i < TASK_LEN; i++) {
            printf("PID : %d -> %c\n", getpid(), buf[i]);
            usleep(300000);
        }
    }
}
