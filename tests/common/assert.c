#include <stdio.h>
#include <stdlib.h>

int assert(int expected, int actual, char *code) {
    if (expected == actual)
        printf("%s => %d\n", code, actual);
    else {
        printf("%s => %d expected, but got %d\n", code, expected, actual);
        exit(1);
    }
    return 0;
}
