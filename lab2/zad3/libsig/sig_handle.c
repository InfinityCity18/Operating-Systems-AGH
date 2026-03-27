#include <signal.h>
#include <stdio.h>

void handler(int signum) {
    printf("Wywołano handler dla sygnału %d\n", signum);
}

void sig_handle() {
    signal(SIGUSR1, handler);
}