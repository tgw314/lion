#include <stdio.h>
#include <stdlib.h>

void assert(int expected, int actual, char *code) {
    if (expected == actual) {
        printf("OK: %s => %d\n", code, actual);
    } else {
        printf("NG: %s => %d expected but got %d\n", code, expected, actual);
        exit(1);
    }
}

static int static_fn(void) { return 10; }
