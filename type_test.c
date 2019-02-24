#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "cc.h"

static void expect(int line, int expected, int actual) {
    if (expected == actual)
        return;
    fprintf(stderr, "Type test line %d: %d expected, but got %d\n",
            line, expected, actual);
    exit(1);
}

static void expect_type(int line, const Type *expected, const Type *actual) {
    if (!expected && !actual)
        return;

    if (!expected || !actual) {
        fprintf(stderr, "Type test line %d: Null type.\n", line);
        exit(1);
    }

    bool same = true;
    same &= expected->ty == actual->ty;
    same &= expected->array_len == actual->array_len;
    same &= !((expected->ptr_of == NULL) ^ (actual->ptr_of == NULL));
    if (expected->ptr_of)
        expect_type(line, expected->ptr_of, actual->ptr_of);

    if (same)
        return;

    fprintf(stderr, "Type test line %d: Expected type not obtained. Expected {%d, %s, %zu} but got {%d, %s, %zu}\n",
            line,
            expected->ty, expected->ptr_of ? "ptr" : "NULL", expected->array_len,
            actual->ty, actual->ptr_of ? "ptr" : "NULL", actual->array_len);
    exit(1);
}

static void type_getsize_test() {
    // Int.
    Type ty_int = (Type) { .ty = INT, .ptr_of = NULL, .array_len = 0 };
    expect(__LINE__, 4, get_typesize(&ty_int));

    // Pointer to int.
    Type ty_pint = (Type) { .ty = PTR, .ptr_of = &ty_int, .array_len = 0 };
    expect(__LINE__, 8, get_typesize(&ty_pint));

    // Pointer to pointer to int.
    Type ty_ppint = (Type) { .ty = PTR, .ptr_of = &ty_pint, .array_len = 0 };
    expect(__LINE__, 8, get_typesize(&ty_ppint));

    // Array of int.
    Type ty_arint = (Type) { .ty = ARRAY, .ptr_of = &ty_int, .array_len = 3 };
    expect(__LINE__, 12, get_typesize(&ty_arint));

    //// Array of pointers.
    //Type ty_arpint = (Type) { .ty = ARRAY, .ptr_of = &ty_pint, .array_len = 3 };
    //expect(__LINE__, 24, get_typesize(&ty_arint));

    fprintf(stderr, "Type size test OK\n");
}

static void type_deduction_test() {
    // Types.
    Type ty_int = (Type) { .ty = INT, .ptr_of = NULL, .array_len = 0 };
    Type ty_pint = (Type) { .ty = PTR, .ptr_of = &ty_int, .array_len = 0 };

    Node *lhs = calloc(1, sizeof(Node));
    Node *rhs = calloc(1, sizeof(Node));

    lhs->type = NULL;
    rhs->type = &ty_int;
    expect_type(__LINE__,
            NULL,
            deduce_type(lhs, rhs));

    lhs->type = &ty_int;
    rhs->type = NULL;
    expect_type(__LINE__,
            NULL,
            deduce_type(lhs, rhs));

    lhs->type = &ty_int;
    rhs->type = &ty_int;
    expect_type(__LINE__,
            &ty_int,
            deduce_type(lhs, rhs));

    lhs->type = &ty_pint;
    rhs->type = &ty_int;
    expect_type(__LINE__,
            &ty_pint,
            deduce_type(lhs, rhs));

    lhs->type = &ty_int;
    rhs->type = &ty_pint;
    expect_type(__LINE__,
            &ty_pint,
            deduce_type(lhs, rhs));

    fprintf(stderr, "Type deduction test OK\n");
}

void runtest_type() {
    type_getsize_test();
    type_deduction_test();
}
