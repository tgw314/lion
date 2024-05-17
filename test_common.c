#include <stdarg.h>
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

int false_fn() { return 512; }
int true_fn() { return 513; }
int char_fn() { return (2 << 8) + 3; }
int short_fn() { return (2 << 16) + 5; }

int add_all(int n, ...) {
    va_list ap;
    va_start(ap, n);

    int sum = 0;
    for (int i = 0; i < n; i++) sum += va_arg(ap, int);
    return sum;
}
