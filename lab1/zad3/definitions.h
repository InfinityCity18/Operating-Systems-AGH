#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <stdio.h>

#define BASE 10
#define TXTFILE "output.txt"

int is_conversion_valid(char* end, char* s, int converted);