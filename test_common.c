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
int ext1 = 5;
int *ext2 = &ext1;
int ext3 = 7;
int ext_fn1(int x) { return x; }
int ext_fn2(int x) { return x; }
