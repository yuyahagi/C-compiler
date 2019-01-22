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
echo '
int two() { return 2; }
int func2(int x0, int x1) { return x0 + 10*x1; }
int func8(int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5 + 1000000*x6 + 10000000*x7;
}
' | gcc -xc -c -o tmp_funcs.o -

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
try 21 'func2(1, 2);'
try 21 'func2(3-2, 8/4);'
try 21 'x = 2; func2(3-x, 4/x);'
try 0 'func8(1, 2, 3, 4, 5, 6, 7, 8) - 87654321;'
try 0 'x = 2; func8(1, (x), x+1, x*2, 5, 2*(x+1), 3*x+1, 8) - 87654321;'

echo OK
