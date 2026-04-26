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
#include <fcntl.h>

#define eprintf(error_msg) fprintf(stderr, error_msg)

double f(double x) {
    return 4 / (1 + x * x);
}

#define FIFO_FILENAME "integral_fifofile"

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
        eprintf("RECEIVER : Nie udalo sie wyslac wyniku potokiem\n");
    }
    close(pipefd);
}

double compute_integral_k_children(long k, double rect_width, double left_boundary, double right_boundary) {
    double child_range_width = (right_boundary - left_boundary) / (double)k;

    double total = 0.0;

    int* pipe_fds = malloc(k * sizeof(int)); 

    for (long i = 0; i < k; i++) {
        double left_child_range = left_boundary + i * child_range_width;
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
            eprintf("RECEIVER : Nie udalo sie stworzyc procesu potomnego\n");
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

    if (argc < 4) {
        eprintf("RECEIVER : Podano za mala liczba argumentow\n");
        exit(1);
    }

    double rect_width = strtod(argv[1], NULL);
    char* end1;
    char* end2;
    long n = strtol(argv[2], &end1, 10);
    double left_boundary = 0.0;
    double right_boundary = 0.0;
    pid_t ppid = strtol(argv[3], &end2, 10);

    int fd = open(FIFO_FILENAME, O_RDWR);
    ssize_t result = read(fd, &left_boundary, sizeof(left_boundary));
    if (result == -1) {
        perror("RECEIVER : Blad odczytu");
    }
    
    result = read(fd, &right_boundary, sizeof(right_boundary));
    if (result == -1) {
        perror("RECEIVER : Blad odczytu");
    }

    if (errno == ERANGE || end1 == argv[2] || end2 == argv[3] || rect_width > (right_boundary - left_boundary) || rect_width <= 0.0 || n <= 0) {
        eprintf("RECEIVER : Liczba wyszla poza zakres lub nie udalo sie jej przekonwertowac\n");
        exit(1);
    }

    double total = compute_integral_k_children(n, rect_width, left_boundary, right_boundary);

    if (write(fd, &total, sizeof(total)) == -1) {
        fprintf(stderr, "RECEIVER : Zapis do potoku nieudany\n");
    }

    close(fd);

    if (kill(ppid, SIGUSR1) == -1) {
        perror("RECEIVER : Blad sygnalu ");
    }
}
