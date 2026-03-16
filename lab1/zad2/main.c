#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define BASE 10

int is_conversion_valid(char* end, char* s, int converted);

int main(int argc, char* argv[]) {

    if (argc != 3) {
        return 1;
    }

    char* end_n;
    long converted_n = strtol(argv[1], &end_n, BASE);
    int is_valid_n = is_conversion_valid(end_n, argv[1], converted_n);
    if (is_valid_n) { 
        for (long i = 0; i < converted_n; i++) {
            pid_t child_pid = fork();
            if (child_pid == 0) {
                if (execlp("./child", "child", argv[2], NULL) == -1) {
                    _exit(1);
                }
            }
        }
        while(wait(NULL) > 0); 
    } else {
        return 1;
    }
}

int is_conversion_valid(char* end, char* s, int converted) {
    return end != s && errno != ERANGE && (converted >= INT_MIN || converted <= INT_MAX);
}