#!/bin/bash
# Prepare external functions.
echo '
#include <stdio.h>
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

# Run all test cases (functions TESTCASE_[0-9].*) found in a C source.
# We use gcc preprocessor to expand macros.
gcc -E -P test/test.c > test/tmp_test.c
echo 'int main() {' >> test/tmp_test.c
grep -o 'TESTCASE_[0-9].*()' test/tmp_test.c | awk '{print $1";"}' >> test/tmp_test.c
echo 'printf("\n");' >> test/tmp_test.c
echo '}' >> test/tmp_test.c
./cc "$(cat test/tmp_test.c)" > test/tmp_test.s
rm -f tmp_test
gcc -o tmp_test test/tmp_test.s tmp_funcs.o
./tmp_test

if [ "$?" != 0 ]; then
    exit 1
fi

echo OK
