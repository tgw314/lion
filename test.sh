#!/bin/bash
objname="tmp"
src_file="$(mktemp --suffix=.c)"
assert() {
    expected="$1"
    input="$2"

    echo -n "$input" > "$src_file"
    ./lion "$src_file" > tmp.s
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

// reference functions
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}

// my functions
int test() { return 4; }
int print() { printf("OK"); }
int *alloc4(int a, int b, int c, int d) {
    int *p = malloc(sizeof(int) * 4);
    p[0] = a;
    p[1] = b;
    p[2] = c;
    p[3] = d;
    return p;
}

EOS

# reference tests
assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 41 'int main() { return  12 + 34 - 5 ; }'
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4 'int main() { return (3+5)/2; }'
assert 10 'int main() { return -10+20; }'
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'

assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'

assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'

assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'

assert 3 'int main() { int a; a=3; return a; }'
assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'

assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'

assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'
assert 6 'int main() { int a; int b; a=b=3; return a+b; }'
assert 3 'int main() { int foo=3; return foo; }'
assert 8 'int main() { int foo123=3; int bar=5; return foo123+bar; }'

assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'

assert 55 'int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main() { for (;;) return 3; return 5; }'

assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'

assert 3 'int main() { {1; {2;} return 3;} }'
assert 5 'int main() { ;;; return 5; }'

assert 10 'int main() { int i=0; while(i<10) i=i+1; return i; }'
assert 55 'int main() { int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 5 'int main() { int x=3; int y=5; return *(&x-(-1)); }'
assert 5 'int main() { int x=3; int *y=&x; *y=5; return x; }'
assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
assert 7 'int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }'
assert 5 'int main() { int x=3; return (&x+2)-&x+3; }'
assert 8 'int main() { int x, y; x=3; y=5; return x+y; }'
assert 8 'int main() { int x=3, y=5; return x+y; }'

assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'

assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y=x; *y=0; return **x; }'
assert 1 'int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }'
assert 2 'int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }'
assert 3 'int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }'
assert 4 'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }'
assert 5 'int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }'

assert 3 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }'
assert 5 'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }'

assert 0 'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }'
assert 1 'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }'
assert 2 'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }'
assert 3 'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }'
assert 4 'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }'
assert 5 'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }'

assert 4 'int main() { int x; return sizeof(x); }'
assert 4 'int main() { int x; return sizeof x; }'
assert 8 'int main() { int *x; return sizeof(x); }'
assert 16 'int main() { int x[4]; return sizeof(x); }'
assert 48 'int main() { int x[3][4]; return sizeof(x); }'
assert 16 'int main() { int x[3][4]; return sizeof(*x); }'
assert 4 'int main() { int x[3][4]; return sizeof(**x); }'
assert 5 'int main() { int x[3][4]; return sizeof(**x) + 1; }'
assert 5 'int main() { int x[3][4]; return sizeof **x + 1; }'
assert 4 'int main() { int x[3][4]; return sizeof(**x + 1); }'
assert 4 'int main() { int x=1; return sizeof(x=2); }'
assert 1 'int main() { int x=1; sizeof(x=2); return x; }'

assert 0 'int x; int main() { return x; }'
assert 3 'int x; int main() { x=3; return x; }'
assert 7 'int x; int y; int main() { x=3; y=4; return x+y; }'
assert 7 'int x, y; int main() { x=3; y=4; return x+y; }'
assert 0 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }'
assert 1 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }'
assert 2 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }'
assert 3 'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }'

assert 4 'int x; int main() { return sizeof(x); }'
assert 16 'int x[4]; int main() { return sizeof(x); }'

assert 1 'int main() { char x=1; return x; }'
assert 1 'int main() { char x=1; char y=2; return x; }'
assert 2 'int main() { char x=1; char y=2; return y; }'

assert 1 'int main() { char x; return sizeof(x); }'
assert 10 'int main() { char x[10]; return sizeof(x); }'
assert 1 'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }'

assert 0 'int main() { return ""[0]; }'
assert 1 'int main() { return sizeof(""); }'

assert 97 'int main() { return "abc"[0]; }'
assert 98 'int main() { return "abc"[1]; }'
assert 99 'int main() { return "abc"[2]; }'
assert 0 'int main() { return "abc"[3]; }'
assert 4 'int main() { return sizeof("abc"); }'

assert 2 'int main() { /* return 1; */ return 2; }'
assert 2 'int main() { // return 1;
return 2; }'

assert 0 'int main() { return ({ 0; }); }'
assert 2 'int main() { return ({ 0; 1; 2; }); }'
assert 1 'int main() { ({ 0; return 1; 2; }); return 3; }'
assert 6 'int main() { return ({ 1; }) + ({ 2; }) + ({ 3; }); }'
assert 3 'int main() { return ({ int x=3; x; }); }'

assert 7 'int main() { return "\a"[0]; }'
assert 8 'int main() { return "\b"[0]; }'
assert 9 'int main() { return "\t"[0]; }'
assert 10 'int main() { return "\n"[0]; }'
assert 11 'int main() { return "\v"[0]; }'
assert 12 'int main() { return "\f"[0]; }'
assert 13 'int main() { return "\r"[0]; }'
assert 27 'int main() { return "\e"[0]; }'

assert 106 'int main() { return "\j"[0]; }'
assert 107 'int main() { return "\k"[0]; }'
assert 108 'int main() { return "\l"[0]; }'

assert 7 'int main() { return "\ax\ny"[0]; }'
assert 120 'int main() { return "\ax\ny"[1]; }'
assert 10 'int main() { return "\ax\ny"[2]; }'
assert 121 'int main() { return "\ax\ny"[3]; }'

assert 0 'int main() { return "\0"[0]; }'
assert 16 'int main() { return "\20"[0]; }'
assert 65 'int main() { return "\101"[0]; }'
assert 104 'int main() { return "\1500"[0]; }'
assert 0 'int main() { return "\x00"[0]; }'
assert 119 'int main() { return "\x77"[0]; }'
assert 165 'int main() { return "\xA5"[0]; }'
assert 255 'int main() { return "\x00ff"[0]; }'

assert 2 'int main() { int x=2; { int x=3; } return x; }'
assert 2 'int main() { int x=2; { int x=3; } { int y=4; return x; }}'
assert 3 'int main() { int x=2; { x=3; } return x; }'

# my tests
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
assert 81 "int main() { int a, b; a = 7; b = 9; return (a + 2) * b; }"
assert 3 "int main() { int a = 10, *b = &a, **c = &b; **c = **c - 5; return a - 2; }"
assert 23 "int main() { int a[2][2]; a[0][0] = 5; a[1][1] = 18; return **a + *(*(a + 1) + 1); }"
assert 56 "int a; int b[10]; int main(){ a = 7; b[9] = 8; return a * b[9]; }"
assert 56 "int a, b[10]; int main(){ a = 7; b[9] = 8; return a * b[9]; }"
assert 13 'int main() { int num = 13; printf("num: %d ", num); return num; }'

echo OK
