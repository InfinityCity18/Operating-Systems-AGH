#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

// Te zadanie mogło mieć bardziej szczegółowy opis, przepraszam jeśli zostało źle zinterpretowane.

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


int argument;
void handler_sigusr2(int sig, siginfo_t *si, void *ucontext)
{
    argument = si->si_value.sival_int;
}

int main(int argc, char* argv[]) {
    printf("Początek procesu potomnego\n");

    // ustawienie handlera na SIGUSR2
    struct sigaction sa;
    sa.sa_sigaction = handler_sigusr2;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &sa, NULL);

    //maska SIGUSR2
    sigset_t sigusr2mask;
    sigemptyset(&sigusr2mask);
    sigaddset(&sigusr2mask, SIGUSR2);

    //czekanie na sigusr2
    sigset_t s;
    sigpending(&s);
    while (!sigismember(&s, SIGUSR2)) {
        sigpending(&s);
    }

    //unblock sigusr2
    sigprocmask(SIG_UNBLOCK, &sigusr2mask, NULL);

    printf("Argument w procesie potomnym to %i\n", argument);

    switch (argument)
    {
    case 0:
        sig_default();
        break;
    case 1:
        sig_ignore();
        break;
    case 2:
        sig_handle();
        break;
    case 3:
        sig_mask();
        break;
    default:
        printf("Nieosiaglany kod");
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
