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

assert 0 0
assert 42 42
assert 21 "5+20-4"
assert 25 " 7  + 24 - 6 "
assert 47 "5+6*7"
assert 15 "5*(9-6)"
assert 4 "(3+5)/2"
assert 11 "3+(-10)+18"
assert 53 "+20*+3+(-7)"
assert 1 "(1+3)*2 == 2*4"
assert 0 "(1+3)*2 != 2*4"
assert 1 "(1+3) < 2*5"
assert 0 "(1+3) > 2*5"
assert 1 "3 >= 6/2"
assert 0 "3 <= 4/2"

echo OK
