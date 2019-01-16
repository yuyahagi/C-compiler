#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "cc.h"

static void expect(int line, int expected, int actual) {
    if (expected == actual)
        return;
    fprintf(stderr, "%d: %d expected, but got %d\n",
            line, expected, actual);
    exit(1);
}

void runtest() {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    Vector *vec = new_vector();

    expect(__LINE__, false, vec->data == NULL);
    expect(__LINE__, 16, vec->capacity);
    expect(__LINE__, 0, vec->len);

    for (int i = 0; i < 16; i++)
        vec_push(vec, (void *)-i);

    expect(__LINE__, 16, vec->capacity);
    expect(__LINE__, 16, vec->len);
    expect(__LINE__, 0, (int)vec->data[0]);
    expect(__LINE__, -15, (int)vec->data[15]);

    vec_push(vec, (void *)500);

    expect(__LINE__, 32, vec->capacity);
    expect(__LINE__, 17, vec->len);
    expect(__LINE__, 500, (int)vec->data[16]);
    
    fprintf(stderr, "Vector test OK\n");
}
