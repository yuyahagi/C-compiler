#!/bin/bash
try() {
    expected="$1"
    input="$2"

    ./cc "$input" > tmp.s
    gcc -o tmp tmp.s tmp_funcs.o
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

# Prepare a nullary function.
echo 'int two() { return 2; }' | gcc -xc -c -o tmp_funcs.o -

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
try 3 'a=1;a+2;'
try 4 '_long_variable_1_ = 2; _long_variable_1_ * 2;'
try 5 'foo=1; bar=baz=foo+1; foo+bar*baz;'
try 0 '2 == 2+1;'
try 1 '2 != 2+1;'
try 1 '3-1 == 2;'
try 0 '3-1 != 2;'
try 2 'i = j = 2+3*4 == 14; i + j;'
try 0 'i = j = 2+3*4 != 14; i + j;'
try 2 'two();'
try 6 '1 + (two)() + 3;'
try 10 '2 * (two() + 3);'

echo OK
