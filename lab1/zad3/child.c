#include "definitions.h"

int main(int argc, char* argv[]) {

    if (argc != 2) {
        exit(1);
    }

    char* end;
    long converted_m = strtol(argv[1], &end, BASE);
    int valid_m = is_conversion_valid(end, argv[1], converted_m);
    if (valid_m) { 

        FILE* f;
        if ((f = fopen(TXTFILE, "a+")) == NULL) {
            exit(1);
        }

        int fd = fileno(f);

        if (flock(fd, LOCK_EX)) {
            exit(1);
        }

        for (long i = 0; i < converted_m; i++) {

            // Wersja bez blokady zakomentowana

            // FILE* f;
            // if ((f = fopen(TXTFILE, "a+")) == NULL) {
            //     exit(1);
            // }

            // int fd = fileno(f);

            // if (flock(fd, LOCK_EX)) {
            //     exit(1);
            // }

            fprintf(f, "Potomek (PID: %d)\n", (int)getpid());
            fflush(f);

            // if (flock(fd, LOCK_UN)) {
            //     exit(1);
            // }

            usleep(250000); //sleep(0.25) jest "niepoprawne"
        }
        if (flock(fd, LOCK_UN)) {
                exit(1);
        }

    } else {
        exit(1);
    }
}

int is_conversion_valid(char* end, char* s, int converted) {
    return end != s && errno != ERANGE && (converted >= INT_MIN || converted <= INT_MAX);
}