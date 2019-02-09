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

try 0 'main() {}'
try 0 'main() {;}'
try 0 'main() {;;;}'
try 0 'main() { 42; }'
try 0 'main() { return; }'
try 42 'main() { return 42; }'
try 0 'main() { 42; return; }'
try 0 'main() { return; 42; }'
try 1 'main() { return 1; 2; }'
try 1 'main() { return 1; return 2; }'
try 21 'main() { return 5+20-4; }'
try 21 ' main ( ){return 5 +20 - 4  ;} '
try 14 'main() { return 2+3*4; }'
try 10 'main() { return 2*3+4; }'
try 14 'main() { return 5*4-3*2; }'
try 5 'main() { return 60/12; }'
try 5 'main() { return 1+60/12-1; }'
try 20 'main() { return (2+3)*4; }'
try 7 'main() { 1; 2; return 3+4; }'
try 3 'main() { a=1; return a+2; }'
try 3 'main() { x = 1; x = x + 2; return x; }'
try 4 'main() { _long_variable_1_ = 2; return _long_variable_1_ * 2; }'
try 5 'main() { foo=1; bar=baz=foo+1; return foo+bar*baz; }'
try 0 'main() { return 2 == 2+1; }'
try 1 'main() { return 2 != 2+1; }'
try 1 'main() { return 3-1 == 2; }'
try 0 'main() { return 3-1 != 2; }'
try 0 'main() { return 4 < 3; }'
try 0 'main() { return 3 < 3; }'
try 1 'main() { return 2 < 3; }'
try 0 'main() { return 2 > 3; }'
try 0 'main() { return 3 > 3; }'
try 1 'main() { return 4 > 3; }'
try 0 'main() { return 4 <= 3; }'
try 1 'main() { return 3 <= 3; }'
try 1 'main() { return 2 <= 3; }'
try 0 'main() { return 2 >= 3; }'
try 1 'main() { return 3 >= 3; }'
try 1 'main() { return 4 >= 3; }'
try 0 'main() { return 1 < 10 != 10 > 1; }'
try 1 'main() { return 1 < 10 == 10 > 1; }'
try 0 'main() { return 1 <= 10 != 10 >= 1; }'
try 1 'main() { return 1 <= 10 == 10 >= 1; }'
try 2 'main() { i = j = 2+3*4 == 14; return i + j; }'
try 0 'main() { i = j = 2+3*4 != 14; return i + j; }'
try 2 'main() { return two(); }'
try 6 'main() { return 1 + (two)() + 3; }'
try 10 'main() { return 2 * (two() + 3); }'
try 21 'main() { return func2(1, 2); }'
try 21 'main() { return func2(3-2, 8/4); }'
try 21 'main() { x = 2; return func2(3-x, 4/x); }'
try 0 'main() { return func8(1, 2, 3, 4, 5, 6, 7, 8) - 87654321; }'
try 0 'main() { x = 2; return func8(1, (x), x+1, x*2, 5, 2*(x+1), 3*x+1, 8) - 87654321; }'
try 3 'foo() { return 3; } main() { return foo(); }'
try 7 'foo() { return 3; } bar() { return 4; } main() { return foo() + bar(); }'
try 5 'foo() { return two()+1; } main() { return foo() + two(); }'
try 43 'foo() { return 3; } bar() { return 4; } main() { return func2(foo(), bar()); }'
try 3 'foo(x0, x1) { return x0 + x1; } main() { return foo(1, 2); }'
try 0 'foo(x0, x1, x2, x3, x4, x5) { return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5; } main() { return foo(1, 2, 3, 4, 5, 6) - 654321; }'

try 14 '
foo(x) { a = 3; return a * x; }
main() { b = 2; return foo(4) + b; }'

try 0 '
foo(x0, x1, x2, x3, x4, x5, x6, x7) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5 + 1000000*x6 + 10000000*x7;
}
main() {
    return foo(1, 2, 3, 4, 5, 6, 7, 8) - 87654321;
}'

try 0 'main() { if (0) return 1; return 0; }'
try 1 'main() { if (1) return 1; return 0; }'
try 0 'main() { if (0) return 1; else return 0; return 127; }'
try 0 'main() { x = 3; if (x + 1 != 2 * 2) y = 1; else y = 0; if (y) return 1; else return 0; }'
try 1 'main() { x = 3; if (x + 1 == 2 * 2) y = 1; else y = 0; if (y) return 1; else return 0; }'

try 15 '
add_upto(x) { if (x == 1) return x; else return x + add_upto(x-1); }
main() { return add_upto(5); }'

try 2 'main() { x = 0; if (1) { x = 1; x = x * 2; } return x; }'

try 2 '
main() {
    x = 0;
    if (0) {
        x = 1;
        x = x * 2;
    } else {
        x = 0;
        if (x) x = 1; else x = 2;
    }
    return x;
}'

try 1 '
foo(x) {
    if (x == 2) return 1;
    if (x == 1) return 1;
    return foo(x-1);
}
main() { return  foo(3); }
'

try 13 '
fib(x) {
    if (x == 2) return 1;
    if (x == 1) return 1;

    return fib(x-1) + fib(x-2);
}
main() { return fib(7); }'

try 1 'main() { x = 1; while (0) x = 0; return x; }'
try 5 'main() { i = 0; while (i != 5) i = i + 1; return i; }'
try 10 'main() { i = 0; sum = 0; while (i != 5) { sum = sum + i; i = i + 1; } return sum; }'

echo OK
