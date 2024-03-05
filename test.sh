#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./lion "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 25 " 7  + 24 - 6 ;"
assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"
assert 11 "3+(-10)+18;"
assert 53 "+20*+3+(-7);"
assert 1 "(1+3)*2 == 2*4;"
assert 0 "(1+3)*2 != 2*4;"
assert 1 "(1+3) < 2*5;"
assert 0 "(1+3) > 2*5;"
assert 1 "3 >= 6/2;"
assert 0 "3 <= 4/2;"
assert 81 "a = 7; b = 9; (a + 2) * b;"
assert 81 "numa = 7; numb = 9; (numa + 2) * numb;"
assert 19 "9; return 8*2+3; 9;"
assert 30 "if (1 < 2) a = 30; return a;"
assert 40 "if (1 > 2) a = 30; else a = 40; return a;"
assert 45 "if (1 > 2) a = 30; else if (2 == 2) a = 45; else a = 50; return a;"
assert 10 "i = 0; while (i < 10) i = i + 1; return i;"
assert 10 "for (i = 0; i < 10; i = i + 1) a = 0; return i;"
assert 10 "i = 0; for (; i < 10;) i = i + 1; return i;"

echo OK
