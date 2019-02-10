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

try 0 'int main() {}'
try 0 'int main() {;}'
try 0 'int main() {;;;}'
try 0 'int main() { 42; }'
try 0 'int main() { return; }'
try 42 'int main() { return 42; }'
try 0 'int main() { 42; return; }'
try 0 'int main() { return; 42; }'
try 1 'int main() { return 1; 2; }'
try 1 'int main() { return 1; return 2; }'
try 21 'int main() { return 5+20-4; }'
try 21 ' int main ( ){return 5 +20 - 4  ;} '
try 14 'int main() { return 2+3*4; }'
try 10 'int main() { return 2*3+4; }'
try 14 'int main() { return 5*4-3*2; }'
try 5 'int main() { return 60/12; }'
try 5 'int main() { return 1+60/12-1; }'
try 20 'int main() { return (2+3)*4; }'
try 7 'int main() { 1; 2; return 3+4; }'
try 3 'int main() { int a; a=1; return a+2; }'
try 3 'int main() { int x; x = 1; x = x + 2; return x; }'
try 4 'int main() { int _long_variable_1_; _long_variable_1_ = 2; return _long_variable_1_ * 2; }'
try 5 'int main() { int foo; int bar; int baz; foo=1; bar=baz=foo+1; return foo+bar*baz; }'
try 1 'int main() { int foo; foo = 0; return (foo = foo + 3) == 3; }'
try 0 'int main() { return 2 == 2+1; }'
try 1 'int main() { return 2 != 2+1; }'
try 1 'int main() { return 3-1 == 2; }'
try 0 'int main() { return 3-1 != 2; }'
try 0 'int main() { return 4 < 3; }'
try 0 'int main() { return 3 < 3; }'
try 1 'int main() { return 2 < 3; }'
try 0 'int main() { return 2 > 3; }'
try 0 'int main() { return 3 > 3; }'
try 1 'int main() { return 4 > 3; }'
try 0 'int main() { return 4 <= 3; }'
try 1 'int main() { return 3 <= 3; }'
try 1 'int main() { return 2 <= 3; }'
try 0 'int main() { return 2 >= 3; }'
try 1 'int main() { return 3 >= 3; }'
try 1 'int main() { return 4 >= 3; }'
try 0 'int main() { return 1 < 10 != 10 > 1; }'
try 1 'int main() { return 1 < 10 == 10 > 1; }'
try 0 'int main() { return 1 <= 10 != 10 >= 1; }'
try 1 'int main() { return 1 <= 10 == 10 >= 1; }'
try 2 'int main() { int i; int j; i = j = 2+3*4 == 14; return i + j; }'
try 0 'int main() { int i; int j; i = j = 2+3*4 != 14; return i + j; }'
try 2 'int main() { return two(); }'
try 6 'int main() { return 1 + (two)() + 3; }'
try 10 'int main() { return 2 * (two() + 3); }'
try 21 'int main() { return func2(1, 2); }'
try 21 'int main() { return func2(3-2, 8/4); }'
try 21 'int main() { int x; x = 2; return func2(3-x, 4/x); }'
try 0 'int main() { return func8(1, 2, 3, 4, 5, 6, 7, 8) - 87654321; }'
try 0 'int main() { int x; x = 2; return func8(1, (x), x+1, x*2, 5, 2*(x+1), 3*x+1, 8) - 87654321; }'
try 3 'int foo() { return 3; } int main() { return foo(); }'
try 7 'int foo() { return 3; } int bar() { return 4; } int main() { return foo() + bar(); }'
try 5 'int foo() { return two()+1; } int main() { return foo() + two(); }'
try 43 'int foo() { return 3; } int bar() { return 4; } int main() { return func2(foo(), bar()); }'
try 3 'int foo(int x0, int x1) { return x0 + x1; } int main() { return foo(1, 2); }'

try 0 '
int foo(int x0, int x1, int x2, int x3, int x4, int x5) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5;
}
int main() { return foo(1, 2, 3, 4, 5, 6) - 654321; }'

try 14 '
int foo(int x) { int a; a = 3; return a * x; }
int main() { int b; b = 2; return foo(4) + b; }'

try 0 '
int foo(int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5 + 1000000*x6 + 10000000*x7;
}
int main() {
    return foo(1, 2, 3, 4, 5, 6, 7, 8) - 87654321;
}'

try 0 'int main() { if (0) return 1; return 0; }'
try 1 'int main() { if (1) return 1; return 0; }'
try 0 'int main() { if (0) return 1; else return 0; return 127; }'
try 0 'int main() { int x; int y; x = 3; if (x + 1 != 2 * 2) y = 1; else y = 0; if (y) return 1; else return 0; }'
try 1 'int main() { int x; int y; x = 3; if (x + 1 == 2 * 2) y = 1; else y = 0; if (y) return 1; else return 0; }'

try 15 '
int add_upto(int x) { if (x == 1) return x; else return x + add_upto(x-1); }
int main() { return add_upto(5); }'

try 2 'int main() { int x; x = 0; if (1) { x = 1; x = x * 2; } return x; }'

try 2 '
int main() {
    int x;
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
int foo(int x) {
    if (x == 2) return 1;
    if (x == 1) return 1;
    return foo(x-1);
}
int main() { return  foo(3); }
'

try 13 '
int fib(int x) {
    if (x == 2) return 1;
    if (x == 1) return 1;
    return fib(x-1) + fib(x-2);
}
int main() { return fib(7); }'

try 1 'int main() { int x; x = 1; while (0) x = 0; return x; }'
try 5 'int main() { int i; i = 0; while (i < 5) i = i + 1; return i; }'
try 10 'int main() { int i; int sum; i = 0; sum = 0; while (i < 5) { sum = sum + i; i = i + 1; } return sum; }'
try 5 'int main() { int i; i = 0; while ((i = i + 1) < 5) ; return i;}'

try 10 'int main() { int sum; int i; sum = 0; for (i = 0; i < 5; i = i + 1) sum = sum + i; return sum; }'
try 10 'int main() { int sum; int i; sum = 0; for (i = 4; i >= 0; i = i - 1) sum = sum + i; return sum; }'
try 5 'int main() { int i; i = 0; for ( ; i < 5; i = i + 1) ; return i; }'
try 5 'int main() { int i; i = 5; for ( ; i < 5; i = i + 1) ; return i; }'
try 5 'int main() { int i; i = 0; for ( ; i < 5; ) i = i + 1; return i; }'
try 34 '
int main() {
    int sum; int prod; int i;
    sum = 0;
    prod = 1;
    for (i = 1; i < 5; i = i + 1) {
        sum = sum + i;
        prod = prod * i;
    }
    return sum + prod;
}'

echo OK
