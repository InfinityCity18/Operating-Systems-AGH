#include <signal.h>

void sig_default() {
    signal(SIGUSR1, SIG_DFL);
}