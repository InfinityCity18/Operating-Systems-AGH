#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

// Te zadanie mogło mieć bardziej szczegółowy opis, przepraszam jeśli zostało źle zinterpretowane.

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Podano zla ilosc argumentow\n");
        exit(1);
    }

    { //block SIGUSR2
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    }

    pid_t child = fork();
    if (child == 0) {
        if (execlp("./child", "child", NULL) == -1) {
            printf("'execlp' nie powiodl sie");
            _exit(1);
        }
    } else if (child == -1) {
        printf("'fork()' nie powiodl sie");
        exit(1);
    }

    union sigval u;
    if (strcmp(argv[1], "default") == 0) {
        u.sival_int = 0; //numer reakcji w child.c
    }
    else if (strcmp(argv[1], "ignore") == 0) {
        u.sival_int = 1;
    }
    else if (strcmp(argv[1], "handle") == 0) {
        u.sival_int = 2;
    }
    else if (strcmp(argv[1], "mask") == 0) {
        u.sival_int = 3;
    }
    else {
        printf("Zle polecenie\n");
        exit(1);
    }
    sigqueue(child, SIGUSR2, u);
    printf("Wysłano SIGUSR2 do procesu potomnego\n");
    while(wait(NULL) > 0);
    printf("Koniec procesu macierzynskiego\n");
}