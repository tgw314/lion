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
int test() { return 4; }
int print() { printf("OK\n"); }
int add(int a, int b) { return a + b; }
EOS

assert 0  "{ return 0; }"
assert 42 "{ return 42; }"
assert 21 "{ return 5+20-4; }"
assert 25 "{ return 7  + 24 - 6 ; }"
assert 47 "{ return 5+6*7; }"
assert 15 "{ return 5*(9-6); }"
assert 4  "{ return (3+5)/2; }"
assert 11 "{ return 3+(-10)+18; }"
assert 53 "{ return +20*+3+(-7); }"
assert 1  "{ return (1+3)*2 == 2*4; }"
assert 0  "{ return (1+3)*2 != 2*4; }"
assert 1  "{ return (1+3) < 2*5; }"
assert 0  "{ return (1+3) > 2*5; }"
assert 1  "{ return 3 >= 6/2; }"
assert 0  "{ return 3 <= 4/2; }"
assert 81 "{ a = 7; b = 9; return (a + 2) * b; }"
assert 81 "{ num_a = 7; num_b = 9; return (num_a + 2) * num_b; }"
assert 19 "{ 9; return 8*2+3; 9; }"
assert 30 "{ if (1 < 2) a = 30; return a; }"
assert 40 "{ if (1 > 2) a = 30; else a = 40; return a; }"
assert 45 "{ if (1 > 2) a = 30; else if (2 == 2) a = 45; else a = 50; return a; }"
assert 10 "{ i = 0; while (i < 10) i = i + 1; return i; }"
assert 10 "{ for (i = 0; i < 10; i = i + 1) a = 0; return i; }"
assert 10 "{ i = 0; for (; i < 10;) i = i + 1; return i; }"
assert 3  "{ {1; {2;} return 3;} }"
assert 4  "{ return test(); }"
assert 0  "{ print(); return 0; }"
assert 25 "{ return add(10, 15); }"

echo OK
