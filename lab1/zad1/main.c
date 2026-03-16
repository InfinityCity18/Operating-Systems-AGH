#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define M 5
#define BASE 10

int zmiennaGlobalna = 0;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        return 1;
    }

    char* end;
    long converted_n = strtol(argv[1], &end, BASE);
    if (end != argv[1] && errno != ERANGE && (converted_n >= INT_MIN || converted_n <= INT_MAX)) { 
        for (long i = 0; i < converted_n; i++) {
            pid_t child_pid = fork();

            if (child_pid == 0) {
                zmiennaGlobalna++;
                for (int j = 0; j < M; j++) {
                    printf("Potomek (PID: %d)\n", (int)getpid());
                    usleep(250000); //sleep(0.25) jest "niepoprawne"
                }
                _exit(0); 
                /* The child must not return from the current function or call exit(3) (which would have
                the effect of calling exit handlers established by the parent process and flushing the parent's stdio(3) buffers), 
                but may call _exit(2). - vfork(2) man page, w przypadku fork() jest to obojetne*/
            }
        }
        //waitpid(0, NULL, 0); w instrukcji na upelu jest napisane "oczekiwanie na każdego potomka", gdzie man page: 
        /*
        0      meaning wait for any child process whose process group ID
              is equal to that of the calling process at the time of the
              call to waitpid().
        Dlatego poniższy printf był wywoływany zanim każdy proces potomny zakończył się, kiedy waitpid(0, NULL, 0) był zamiast "while"
        */ 
        while(wait(NULL) > 0); // wait(NULL) ~ waitpid(-1, NULL, 0)
        printf("Rodzic (PID: %d) zmiennaGlobalna = %d\n", (int)getpid(), zmiennaGlobalna);

    } else {
        return 1;
    }
}