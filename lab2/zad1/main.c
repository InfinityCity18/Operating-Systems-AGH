#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

void sig_default() {
    signal(SIGUSR1, SIG_DFL);
}

void sig_ignore() {
    signal(SIGUSR1, SIG_IGN);
}

void handler(int signum) {
    printf("Wywołano handler dla sygnału %d\n", signum);
}

void sig_handle() {
    signal(SIGUSR1, handler);
}

void sig_mask() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void sig_unblock() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Podano zla ilosc argumentow\n");
        exit(1);
    }

    if (strcmp(argv[1], "default") == 0) {
        sig_default();
        printf("Wywołano funkcję 'sig_default()'\n");
    }
    else if (strcmp(argv[1], "ignore") == 0) {
        sig_ignore();
        printf("Wywołano funkcję 'sig_ignore()'\n");
    }
    else if (strcmp(argv[1], "handle") == 0) {
        sig_handle();
        printf("Wywołano funkcję 'sig_handle()'\n");
    }
    else if (strcmp(argv[1], "mask") == 0) {
        sig_mask();
        printf("Wywołano funkcję 'sig_mask()'\n");
    }
    else {
        printf("Zle polecenie\n");
        exit(1);
    }

    for (int i = 1; i <= 20; i++) {
        printf("i = %d\n", i);
        sigset_t s;
        sigpending(&s);
        if (i == 5 || i == 15) {
            printf("Wysyłam sygnał USR1\n");
            raise(SIGUSR1);
        } else if (i == 10 && sigismember(&s, SIGUSR1)) {
            printf("Odblokowuję USR1\n");
            sig_unblock();
        }
        sleep(1);
    }
    printf("Pętla została wykonana w całości.\n");
}
