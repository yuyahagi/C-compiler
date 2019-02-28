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
#include <stdlib.h>
int two() { return 2; }
int func2(int x0, int x1) { return x0 + 10*x1; }
int func8(int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5 + 1000000*x6 + 10000000*x7;
}
void alloc4(int **p, int x0, int x1, int x2, int x3) {
    *p = malloc(4 * sizeof(int));
    (*p)[0] = x0; (*p)[1] = x1; (*p)[2] = x2; (*p)[3] = x3;
}
void allocptr4(int ***pp, int x0, int x1, int x2, int x3) {
    int *p = malloc(4 * sizeof(int));
    p[0] = x0; p[1] = x1; p[2] = x2; p[3] = x3;
    *pp = malloc(4 * sizeof(int*));
    (*pp)[0] = &p[0]; (*pp)[1] = &p[1]; (*pp)[2] = &p[2]; (*pp)[3] = &p[3];
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

# Local variables.
try 3 'int main() { int a; a=1; return a+2; }'
try 3 'int main() { int x; x = 1; x = x + 2; return x; }'
try 4 'int main() { int _long_variable_1_; _long_variable_1_ = 2; return _long_variable_1_ * 2; }'
try 5 'int main() { int foo; int bar; int baz; foo=1; bar=baz=foo+1; return foo+bar*baz; }'
try 1 'int main() { int foo; foo = 0; return (foo = foo + 3) == 3; }'

# Pointers.
try 3 'int main() { int x; int *p; p = &x; x = 3; return *p; }'
try 3 'int main() { int x; int *p; p = &x; *p = 3; return x; }'
try 3 'int main() { int x; int *p; int **pp; pp = &p; p = &x; x = 3; return **pp; }'
try 3 'int main() { int x; int *p; int **pp; pp = &p; p = &x; **pp = 3; return x; }'
try 3 'int main() { int x; int y; int *p; x = 2; p = &x; y = *p + 1; p = &y; return *p; }'
try 3 'int main() { int x; int *p; p = &x; *p = 2; *p = 2**p-1; return x; }'

try 4 '
int main() {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int *q;
    q = p + 2;
    return *q;
}'

try 4 '
int main() {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    return *(2 + p);
}'

try 3 '
int main() {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int *q;
    q = p + 1;
    *(p + 1) = 3;
    return *q;
}'

try 2 '
int main() {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int offset;
    offset = 1;
    int *q;
    q = two() + p - offset;
    return *q;
}'

try 2 '
int main() {
    int *p;
    alloc4(&p, 1, 2, 4, 8);
    int offset;
    offset = 1;
    return *(two() + p - offset);
}'

try 4 '
int main() {
    int **pp;
    allocptr4(&pp, 1, 2, 4, 8);
    pp = pp + 2;
    return **pp;
}'

# Arrays.
try 0 'int main() { int ar[9]; }'
try 0 'int main() { int ar[3]; int *p; p = ar; }'
try 1 'int main() { int ar[3]; *ar=1; return *ar; }'
try 2 'int main() { int ar[3]; int *p; p = ar; *(p+1) = 2; return *(ar+1); }'
try 7 'int main() { int ar[3]; int *p; p = ar; *p = 1; *(p+1) = 2; *(p+2) = 4; return *ar + *(ar+1) + *(ar+2); }'
try 7 'int main() { int ar[3]; *ar=1; *(ar+1) = 2; *(ar+2) = 4; return *ar + *(ar+1) + *(ar+2); }'
try 7 'int main() { int ar[3]; *ar = 1; *(ar+1) = 2; *(ar+2) = 4; return ar[0] + ar[1] + ar[2]; }'
try 7 'int main() { int ar[3]; ar[0] = 1; ar[1] = 2; ar[2] = 4; return *ar + *(ar+1) + *(ar+2); }'
try 2 'int main() { int ar[3]; int i; i = 1; ar[i] = 2; return ar[i]; }'
try 2 'int main() { int ar[2]; ar[1] = 2; ar[0] = 1; return ar[1]; }'
try 7 'int main() { int ar[3]; ar[2] = 4; ar[1] = 2; ar[0] = 1; return ar[0] + ar[1] + ar[2]; }'

# Relations.
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

# Global variables.
try 3 '
int x;
int set_x(int val) { x = val; }
int main() { set_x(3); return x; }'
try 3 '
int x;
int setlocal(int val) { int x; x = val; }
int setglobal(int val) { x = val; }
int main() { setglobal(3); setlocal(2); return x; }'
try 3 '
int *p;
int x;
int main() { p = &x; x = 1; *p = *p + 2; return x; }'
try 7 '
int x[3];
int main() { x[0] = 1; x[1] = 2; x[2] = 4; return x[0] + x[1] + x[2]; }'
try 7 '
int x[3];
int main() { int *p; p = x; *p = 1; *(p+1) = 2; *(p+2) = 4; return x[0] + x[1] + x[2]; }'
try 15 '
int x[3];
int y;
int main() { y = 8; x[2] = 4; x[1] = 2; x[0] = 1; return x[0] + x[1] + x[2] + y; }'

# Functions.
try 2 'int main() { return two(); }'
try 6 'int main() { return 1 + (two)() + 3; }'
try 10 'int main() { return 2 * (two() + 3); }'
try 21 'int main() { return func2(1, 2); }'
try 21 'int main() { return func2(3-2, 8/4); }'
try 21 'int main() { int x; x = 2; return func2(3-x, 4/x); }'
try 1 'int main() { return func8(1, 2, 3, 4, 5, 6, 7, 8) == 87654321; }'
try 1 'int main() { int x; x = 2; return func8(1, (x), x+1, x*2, 5, 2*(x+1), 3*x+1, 8) == 87654321; }'
try 3 'int foo() { return 3; } int main() { return foo(); }'
try 7 'int foo() { return 3; } int bar() { return 4; } int main() { return foo() + bar(); }'
try 5 'int foo() { return two()+1; } int main() { return foo() + two(); }'
try 43 'int foo() { return 3; } int bar() { return 4; } int main() { return func2(foo(), bar()); }'
try 3 'int foo(int x0, int x1) { return x0 + x1; } int main() { return foo(1, 2); }'

try 1 '
int foo(int x0, int x1, int x2, int x3, int x4, int x5) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5;
}
int main() { return foo(1, 2, 3, 4, 5, 6) == 654321; }'

try 14 '
int foo(int x) { int a; a = 3; return a * x; }
int main() { int b; b = 2; return foo(4) + b; }'

try 1 '
int foo(int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7) {
    return x0 + 10*x1 + 100*x2 + 1000*x3 + 10000*x4 + 100000*x5 + 1000000*x6 + 10000000*x7;
}
int main() {
    return foo(1, 2, 3, 4, 5, 6, 7, 8) == 87654321;
}'

# Selection and iteration statements.
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
try 13 '
int main() {
    int fib[7]; int i;
    fib[0] = 1;
    fib[1] = 1;
    for (i = 2; i < 7; i = i + 1)
        fib[i] = fib[i-1] + fib[i-2];
    return fib[6];
}'

echo OK
