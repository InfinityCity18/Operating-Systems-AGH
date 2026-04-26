#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FIFO_FILENAME "integral_fifofile"
#define TIMEOUT_SECONDS 30

void dummy_handler(int sig) {}

int main(int argc, char* argv[]) {
    char* rect_width = "0.00001";
    char* k = "10";
    
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    signal(SIGUSR1, dummy_handler);

    if (argc < 3) {
        printf("SENDER : Podano za mala liczba argumentow\n");
        exit(1);
    }
    double left_boundary = strtod(argv[1], NULL);
    double right_boundary = strtod(argv[2], NULL);

    if (errno == ERANGE) {
        printf("SENDER : Liczba wyszla poza zakres lub nie udalo sie jej przekonwertowac\n");
        exit(1);
    }

    unlink(FIFO_FILENAME);
    if (mkfifo(FIFO_FILENAME, 0660) == -1) {
        fprintf(stderr, "SENDER : Nie udalo sie stworzyc potoku nazwanego\n");
        exit(1);
    }
    
    char ppid_buf[12];
    pid_t ppid = getpid();
    sprintf(ppid_buf, "%d", ppid);

    pid_t pid = fork();
    if (pid == 0) {
        if (execl("./main_receiver", "main_receiver", rect_width, k, ppid_buf, NULL) == -1) {
            fprintf(stderr, "SENDER : Nie udalo sie wywolac exec w procesie potomnym\n");
            exit(1);
        }
    } else if (pid == -1)
    {
        fprintf(stderr, "SENDER : Proces potomny nie zostal wywolany\n");
    }

    int fd = open(FIFO_FILENAME, O_RDWR);

    if (write(fd, &left_boundary, sizeof(left_boundary)) == -1) {
        fprintf(stderr, "SENDER : Pierwszy zapis do potoku nieudany\n");
    }
    if (write(fd, &right_boundary, sizeof(right_boundary)) == -1) {
        fprintf(stderr, "SENDER : Drugi zapis do potoku nieudany\n");
    } 

    double integral_result = NAN;
    sigsuspend(&oldmask);
    ssize_t result = read(fd, &integral_result, sizeof(integral_result));
    if (result == -1) {
        perror("SENDER : Blad");
    }

    printf("Wyliczona wartosc calki to %f\n", integral_result);

    close(fd);
    unlink(FIFO_FILENAME);
}