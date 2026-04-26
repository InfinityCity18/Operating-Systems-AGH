#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <time.h>

#define eprintf(error_msg) fprintf(stderr, error_msg)

#define LEFT_INTEGRAL_BOUNDARY 0.0
#define RIGHT_INTEGRAL_BOUNDARY 1.0

double f(double x) {
    return 4 / (1 + x * x);
}

void child_compute_integral(int pipefd, double left, double right, double rect_width) {
    double total = 0.0;

    long number_of_rectangles = (long)floor((right - left) / rect_width);
    double last_rectangle_width = (right - left) - (double)number_of_rectangles * rect_width;

    for (long i = 0; i < number_of_rectangles; i++) {
        double subleft = left + i * rect_width;
        double subright = subleft + rect_width;
        double mid = (subleft + subright) / 2.0;
        double integral = f(mid) * rect_width;
        total += integral;
    }
    double last_subleft = left + number_of_rectangles * rect_width;
    double last_subright = last_subleft + last_rectangle_width;
    double last_mid = (last_subleft + last_subright) / 2.0;
    total += f(last_mid) * last_rectangle_width;
    
    if (write(pipefd, &total, sizeof(total)) == -1) {
        eprintf("Nie udalo sie wyslac wyniku potokiem\n");
    }
    close(pipefd);
}

double compute_integral_k_children(long k, double rect_width) {
    double child_range_width = (RIGHT_INTEGRAL_BOUNDARY - LEFT_INTEGRAL_BOUNDARY) / (double)k;

    double total = 0.0;

    int* pipe_fds = malloc(k * sizeof(int)); 

    for (long i = 0; i < k; i++) {
        double left_child_range = LEFT_INTEGRAL_BOUNDARY + i * child_range_width;
        double right_child_range = left_child_range + child_range_width;

        int pipes[2];
        pipe(pipes);
        pipe_fds[i] = pipes[0];
        
        pid_t pid = fork();
        if (pid == 0) {
            close(pipes[0]);
            child_compute_integral(pipes[1], left_child_range, right_child_range, rect_width);
            _exit(0);
        } else if (pid == -1) {
            close(pipes[0]);
            close(pipes[1]);
            eprintf("Nie udalo sie stworzyc procesu potomnego\n");
        } else {
            close(pipes[1]);
        }
    }
    
    for (long i = 0; i < k; i++) {
        int r_pipefd = pipe_fds[i];
        double output = 0.0;
        while (read(r_pipefd, &output, sizeof(output)) != 0);
        total += output;
    }
    return total;
}




int main(int argc, char* argv[]) {
    if (argc < 3) {
        eprintf("Podano za mala liczba argumentow\n");
        exit(1);
    }

    double rect_width = strtod(argv[1], NULL);
    char* end;
    long n = strtol(argv[2], &end, 10);

    if (errno == ERANGE || end == argv[1] || rect_width > (RIGHT_INTEGRAL_BOUNDARY - LEFT_INTEGRAL_BOUNDARY) || rect_width <= 0.0 || n <= 0) {
        eprintf("Liczba wyszla poza zakres lub nie udalo sie jej przekonwertowac\n");
        exit(1);
    }

    for (long k = 1; k <= n; k++) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        double total = compute_integral_k_children(k, rect_width);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("Calkowita wartosc calki z k = %ld : %.15f, w czasie = %.15f sekund\n", k, total, time_taken);
    }
}
