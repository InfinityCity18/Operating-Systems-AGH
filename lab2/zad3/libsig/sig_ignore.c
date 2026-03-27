#include <signal.h>

void sig_ignore() {
    signal(SIGUSR1, SIG_IGN);
}