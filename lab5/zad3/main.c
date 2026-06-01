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
#define SEM_JOBS_PRIO_NAME "/lab5jobspriosem"
#define SEM_EMPTYSLOTS_NAME "/lab5emptyslotssem"
#define SEM_EMPTYSLOTS_PRIO_NAME "/lab5emptyslotspriosem"
#define SEM_PROD_ADDR_MUTEX_NAME "/lab5prodmutex"
#define SEM_PROD_PRIO_ADDR_MUTEX_NAME "/lab5prodpriomutex"
#define SEM_CONS_ADDR_MUTEX_NAME "/lab5consmutex"
#define SEM_CONS_PRIO_ADDR_MUTEX_NAME "/lab5conspriomutex"
#define MANAGER_SLEEP 5
#define TASK_LEN 10
#define WAIT_TIME 300000

struct Semaphores {
    sem_t* jobs_sem_normal;
    sem_t* emptyslots_sem_normal;
    sem_t* jobs_sem_prio;
    sem_t* emptyslots_sem_prio;
    sem_t* mutex_normal;
    sem_t* mutex_prio;
};

void producer(struct Semaphores sems, char** ptr, char** ptr_prio, long k, char* start_addr, char* start_addr_prio);
void consumer(struct Semaphores sems, char** ptr,  char** ptr_prio, long k, char* start_addr, char* start_addr_prio);
void manager(struct Semaphores sems, char** ptr, char** ptr_prio, long k, char* start_addr, char* start_addr_prio);

void intHandler(int dummy) {
    killpg(0, SIGTERM);
}

int main(int argc, char* argv[]) {
    shm_unlink(MEMORY_MAP_FILENAME);
    sem_unlink(SEM_JOBS_NAME);
    sem_unlink(SEM_JOBS_PRIO_NAME);
    sem_unlink(SEM_EMPTYSLOTS_NAME);
    sem_unlink(SEM_EMPTYSLOTS_PRIO_NAME);
    sem_unlink(SEM_PROD_ADDR_MUTEX_NAME);
    sem_unlink(SEM_PROD_PRIO_ADDR_MUTEX_NAME);
    sem_unlink(SEM_CONS_ADDR_MUTEX_NAME);
    sem_unlink(SEM_CONS_PRIO_ADDR_MUTEX_NAME);

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
    void* addr = mmap(NULL, 4 * sizeof(void *) + 2 * k * TASK_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    char** prod_ptr = addr;
    char** prod_ptr_prio = addr + sizeof(void *);
    char** cons_ptr = addr + 2 * sizeof(void *);
    char** cons_ptr_prio = addr + 3 * sizeof(void *);
    char* start_addr = (char*)addr + 4*sizeof(void *);
    char* start_addr_prio = (char*)addr + 4 * sizeof(void *) + k * TASK_LEN;
    *prod_ptr = start_addr;
    *cons_ptr = start_addr;
    *prod_ptr_prio = start_addr_prio;
    *cons_ptr_prio = start_addr_prio;

    sem_t* jobs_sem = sem_open(SEM_JOBS_NAME, O_CREAT, 0660, 0);
    sem_t* jobs_prio_sem = sem_open(SEM_JOBS_PRIO_NAME, O_CREAT, 0660, 0);
    sem_t* emptyslots_sem = sem_open(SEM_EMPTYSLOTS_NAME, O_CREAT, 0660, k);
    sem_t* emptyslots_prio_sem = sem_open(SEM_EMPTYSLOTS_PRIO_NAME, O_CREAT, 0660, k);
    sem_t* prod_mutex = sem_open(SEM_PROD_ADDR_MUTEX_NAME, O_CREAT, 0660, 1);
    sem_t* prod_prio_mutex = sem_open(SEM_PROD_PRIO_ADDR_MUTEX_NAME, O_CREAT, 0660, 1);
    sem_t* cons_mutex = sem_open(SEM_CONS_ADDR_MUTEX_NAME, O_CREAT, 0660, 1);
    sem_t* cons_prio_mutex = sem_open(SEM_CONS_PRIO_ADDR_MUTEX_NAME, O_CREAT, 0660, 1);

    struct Semaphores sems = {jobs_sem, emptyslots_sem, jobs_prio_sem, emptyslots_prio_sem, prod_mutex, prod_prio_mutex};

    for (int i = 0; i < n; i++) {
        if (fork() == 0) {
            producer(sems, prod_ptr, prod_ptr_prio, k, start_addr, start_addr_prio);
            _exit(0);
        }
    }
    sems.mutex_normal = cons_mutex;
    sems.mutex_prio = cons_prio_mutex;
    for (int i = 0; i < m; i++) {
        if (fork() == 0) {
            consumer(sems, cons_ptr, cons_ptr_prio, k, start_addr, start_addr_prio);
            _exit(0);
        }
    }
    sems.mutex_normal = cons_mutex;
    sems.mutex_prio = prod_prio_mutex;
    if (fork() == 0) {
        manager(sems, cons_ptr, prod_ptr_prio, k, start_addr, start_addr_prio);
        _exit(0);
    }

    signal(SIGINT, intHandler);
    pause();
    // while (1) {
    //     usleep(3000000);
    // }
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

void producer(struct Semaphores sems, char** ptr, char** ptr_prio, long k, char* start_addr, char* start_addr_prio) {
    while (1) {
        char str[TASK_LEN];
        rand_str_task(str);
        sem_t *emptyslots_sem, *jobs_sem, *prod_mutex;
        char **chosen_ptr, *chosen_start_addr;
        if (((double)rand() / RAND_MAX < 0.3)) {
            emptyslots_sem = sems.emptyslots_sem_prio;
            jobs_sem = sems.jobs_sem_prio;
            prod_mutex = sems.mutex_prio;
            chosen_ptr = ptr_prio;
            chosen_start_addr = start_addr_prio;
        } else {
            emptyslots_sem = sems.emptyslots_sem_normal;
            jobs_sem = sems.jobs_sem_normal;
            prod_mutex = sems.mutex_normal;
            chosen_ptr = ptr;
            chosen_start_addr = start_addr;
        }

        if (sem_wait(emptyslots_sem) == -1) {
            perror("Blad w sem_wait w producencie");
        }

        if (sem_wait(prod_mutex) == -1) {
            perror("Blad w sem_wait mutex w producencie");
        }

        memcpy(*chosen_ptr, str, TASK_LEN);
        increment_loop(chosen_ptr, chosen_start_addr, k);

        if (sem_post(prod_mutex) == -1) {
            perror("Blad w sem_post mutex w producencie");
        }

        if (sem_post(jobs_sem) == -1) {
            perror("Blad w sem_post w producencie");
        }
    }
}

void consumer(struct Semaphores sems, char** ptr,  char** ptr_prio, long k, char* start_addr, char* start_addr_prio) {
    while (1) {
        char buf[TASK_LEN];
        sem_t *emptyslots_sem, *cons_mutex;
        char **chosen_ptr, *chosen_start_addr;
        if (sem_trywait(sems.jobs_sem_prio) == 0) {
            emptyslots_sem = sems.emptyslots_sem_prio;
            cons_mutex = sems.mutex_prio;
            chosen_ptr = ptr_prio;
            chosen_start_addr = start_addr_prio;
        } else {
            sem_wait(sems.jobs_sem_normal);
            emptyslots_sem = sems.emptyslots_sem_normal;
            cons_mutex = sems.mutex_normal;
            chosen_ptr = ptr;
            chosen_start_addr = start_addr;
        }

        if (sem_wait(cons_mutex) == -1) {
            perror("Blad w sem_wait mutex w konsumencie");
        }

        memcpy(buf, *chosen_ptr, TASK_LEN);
        increment_loop(chosen_ptr, chosen_start_addr, k);

        if (sem_post(cons_mutex) == -1) {
            perror("Blad w sem_post mutex w konsumencie");
        }

        if (sem_post(emptyslots_sem) == -1) {
            perror("Blad w sem_post w konsumencie");
        }
        for (int i = 0; i < TASK_LEN; i++) {
            printf("PID : %d -> %c\n", getpid(), buf[i]);
            usleep(WAIT_TIME);
        }
    }
}

void manager(struct Semaphores sems, char** ptr, char** ptr_prio, long k, char* start_addr, char* start_addr_prio) {
    while (1) {
        sleep(MANAGER_SLEEP);
        if (sem_trywait(sems.jobs_sem_normal) == 0) {
            if (sem_trywait(sems.emptyslots_sem_prio) == 0) {
                sem_wait(sems.mutex_normal);
                sem_wait(sems.mutex_prio);
                memcpy(*ptr_prio, *ptr, TASK_LEN); //copy task to prio
                increment_loop(ptr, start_addr, k);
                increment_loop(ptr_prio, start_addr_prio, k);
                sem_post(sems.mutex_prio);
                sem_post(sems.mutex_normal);
                sem_post(sems.jobs_sem_prio);
                sem_post(sems.emptyslots_sem_normal);
            } else {
                sem_post(sems.jobs_sem_normal);
            }
        }
        int normal, prio;
        sem_getvalue(sems.jobs_sem_normal, &normal);
        sem_getvalue(sems.jobs_sem_prio, &prio);
        printf("MANAGER : Normal jobs : %d, Prio jobs: %d\n", normal, prio);
    }
}
