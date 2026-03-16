#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define BASE 10

int main(int argc, char* argv[]) {

    if (argc != 2) {
        return 1;
    }

    char* end;
    long converted_m = strtol(argv[1], &end, BASE);
    if (end != argv[1] && errno != ERANGE && (converted_m >= INT_MIN || converted_m <= INT_MAX)) { 
        for (long i = 0; i < converted_m; i++) {
            printf("Potomek (PID: %d)\n", (int)getpid());
            usleep(250000); //sleep(0.25) jest "niepoprawne"
        }

    } else {
        return 1;
    }
}