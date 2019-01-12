#!/bin/bash
try() {
    expected="$1"
    input="$2"

    ./cc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "FAIL: \"$input\""
        echo "expected $expected but got $actual"
        exit 1
    fi
}

try 0 '0;'
try 42 '42;'
try 21 '5+20-4;'
try 21 ' 5 +20 - 4  ;'
try 14 '2+3*4;'
try 10 '2*3+4;'
try 14 '5*4-3*2;'
try 5 '60/12;'
try 5 '1+60/12-1;'
try 20 '(2+3)*4;'
try 7 '1; 2; 3+4;'

echo OK
