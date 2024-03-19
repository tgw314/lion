#!/bin/bash
objname="tmp"
assert() {
    expected="$1"
    input="$2"

    ./lion "$input" > tmp.s
    cc -o tmp tmp.s $objname.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

cc -c -xc - -o $objname.o <<EOS
#include <stdio.h>
#include <stdlib.h>
int test() { return 4; }
int print() { printf("OK\n"); }
int add(int a, int b) { return a + b; }
int *alloc4(int a, int b, int c, int d) {
    int *p = malloc(sizeof(int) * 4);
    p[0] = a;
    p[1] = b;
    p[2] = c;
    p[3] = d;
    return p;
}
EOS

assert 0  "int main() { return 0; }"
assert 42 "int main() { return 42; }"
assert 21 "int main() { return 5+20-4; }"
assert 25 "int main() { return 7  + 24 - 6 ; }"
assert 47 "int main() { return 5+6*7; }"
assert 15 "int main() { return 5*(9-6); }"
assert 4  "int main() { return (3+5)/2; }"
assert 11 "int main() { return 3+(-10)+18; }"
assert 53 "int main() { return +20*+3+(-7); }"
assert 1  "int main() { return (1+3)*2 == 2*4; }"
assert 0  "int main() { return (1+3)*2 != 2*4; }"
assert 1  "int main() { return (1+3) < 2*5; }"
assert 0  "int main() { return (1+3) > 2*5; }"
assert 1  "int main() { return 3 >= 6/2; }"
assert 0  "int main() { return 3 <= 4/2; }"
assert 81 "int main() { int a; int b; a = 7; b = 9; return (a + 2) * b; }"
assert 81 "int main() { int num_a; int num_b; num_a = 7; num_b = 9; return (num_a + 2) * num_b; }"
assert 19 "int main() { 9; return 8*2+3; 9; }"
assert 30 "int main() { int a; if (1 < 2) a = 30; return a; }"
assert 40 "int main() { int a; if (1 > 2) a = 30; else a = 40; return a; }"
assert 45 "int main() { int a; if (1 > 2) a = 30; else if (2 == 2) a = 45; else a = 50; return a; }"
assert 10 "int main() { int i; i = 0; while (i < 10) i = i + 1; return i; }"
assert 10 "int main() { int i; int a; for (i = 0; i < 10; i = i + 1) a = 0; return i; }"
assert 10 "int main() { int i; i = 0; for (; i < 10;) i = i + 1; return i; }"
assert 3  "int main() { {1; {2;} return 3;} }"
assert 4  "int main() { return test(); }"
assert 0  "int main() { print(); return 0; }"
assert 25 "int main() { return add(10, 15); }"
assert 25 "int add_self(int a, int b) { return a + b; } int main() { return add_self(10, 15); }"
assert 233 "int fib(int n) { if (n <= 1) return n; return fib(n - 1) + fib(n - 2); } int main() { return fib(13); }"
assert 8 "int main() { int a; int *b; a = 10; b = &a; return *b - 2; }"
assert 0 "int main() { ;;;;;;;;;;;;;;;;;;return 0; }"
assert 3 "int main() { int a; int *b; int **c; a = 10; b = &a; c = &b; **c = **c - 5; return a - 2; }"
assert 8 "int main() { int *p; p = alloc4(1, 2, 4, 8); int *q; q = p + 3; return *(q); }"
assert 4 "int main() { int p; p = sizeof(1); return sizeof(p); }"
assert 8 "int main() { int p; p = sizeof(1); return sizeof(&p); }"
assert 40 "int main() { int a[10]; return sizeof(a); }"
assert 3 "int main() { int a[2]; *a = 1; *(a + 1) = 2; int *p; p = a; return *p + *(p + 1); }"
assert 12 "int main() { int a[2]; a[0] = 5; a[1] = 7; int *p; p = a; return p[0] + p[1]; }"

echo OK
