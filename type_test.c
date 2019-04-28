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

static void type_is_basic_type_test() {
    Type ty_char = (Type) { .ty = CHAR, .ptr_of = NULL, .array_len = 0 };
    Type ty_short = (Type) { .ty = SHORT, .ptr_of = NULL, .array_len = 0 };
    Type ty_int = (Type) { .ty = INT, .ptr_of = NULL, .array_len = 0 };
    Type ty_pchar = (Type) { .ty = PTR, .ptr_of = &ty_char, .array_len = 0 };
    Type ty_pshort = (Type) { .ty = PTR, .ptr_of = &ty_short, .array_len = 0 };
    Type ty_pint = (Type) { .ty = PTR, .ptr_of = &ty_int, .array_len = 0 };
    Type ty_archar = (Type) { .ty = ARRAY, .ptr_of = &ty_char, .array_len = 3 };
    Type ty_arshort = (Type) { .ty = ARRAY, .ptr_of = &ty_short, .array_len = 3 };
    Type ty_arint = (Type) { .ty = ARRAY, .ptr_of = &ty_int, .array_len = 3 };
    Type ty_struct = (Type) { .ty = STRUCT };

    expect(__LINE__, 1, is_basic_type(&ty_char));
    expect(__LINE__, 1, is_basic_type(&ty_short));
    expect(__LINE__, 1, is_basic_type(&ty_int));
    expect(__LINE__, 0, is_basic_type(&ty_pchar));
    expect(__LINE__, 0, is_basic_type(&ty_pshort));
    expect(__LINE__, 0, is_basic_type(&ty_pint));
    expect(__LINE__, 0, is_basic_type(&ty_archar));
    expect(__LINE__, 0, is_basic_type(&ty_arshort));
    expect(__LINE__, 0, is_basic_type(&ty_arint));
    expect(__LINE__, 0, is_basic_type(&ty_struct));

    fprintf(stderr, "Type basic type test OK\n");
}

static void type_getsize_test() {
    // Basic types.
    Type ty_char = (Type) { .ty = CHAR, .ptr_of = NULL, .array_len = 0 };
    expect(__LINE__, 1, get_typesize(&ty_char));

    Type ty_short = (Type) { .ty = SHORT, .ptr_of = NULL, .array_len = 0 };
    expect(__LINE__, 2, get_typesize(&ty_short));

    Type ty_int = (Type) { .ty = INT, .ptr_of = NULL, .array_len = 0 };
    expect(__LINE__, 4, get_typesize(&ty_int));

    // Pointer to basic types.
    Type ty_pchar = (Type) { .ty = PTR, .ptr_of = &ty_char, .array_len = 0 };
    expect(__LINE__, 8, get_typesize(&ty_pchar));

    Type ty_pshort = (Type) { .ty = PTR, .ptr_of = &ty_short, .array_len = 0 };
    expect(__LINE__, 8, get_typesize(&ty_pshort));

    Type ty_pint = (Type) { .ty = PTR, .ptr_of = &ty_int, .array_len = 0 };
    expect(__LINE__, 8, get_typesize(&ty_pint));

    // Pointer to pointer to basic types.
    Type ty_ppchar = (Type) { .ty = PTR, .ptr_of = &ty_pchar, .array_len = 0 };
    expect(__LINE__, 8, get_typesize(&ty_ppchar));

    Type ty_ppshort = (Type) { .ty = PTR, .ptr_of = &ty_pshort, .array_len = 0 };
    expect(__LINE__, 8, get_typesize(&ty_ppshort));

    Type ty_ppint = (Type) { .ty = PTR, .ptr_of = &ty_pint, .array_len = 0 };
    expect(__LINE__, 8, get_typesize(&ty_ppint));

    // Array of basic types.
    Type ty_archar = (Type) { .ty = ARRAY, .ptr_of = &ty_char, .array_len = 3 };
    expect(__LINE__, 3, get_typesize(&ty_archar));

    Type ty_arshort = (Type) { .ty = ARRAY, .ptr_of = &ty_short, .array_len = 3 };
    expect(__LINE__, 6, get_typesize(&ty_arshort));

    Type ty_arint = (Type) { .ty = ARRAY, .ptr_of = &ty_int, .array_len = 3 };
    expect(__LINE__, 12, get_typesize(&ty_arint));

    // Array of pointers.
    Type ty_arpint = (Type) { .ty = ARRAY, .ptr_of = &ty_pint, .array_len = 3 };
    expect(__LINE__, 24, get_typesize(&ty_arpint));

    // Structures.
    Map *member_types = new_map();
    Map *member_offsets = new_map();
    Type ty_struct = (Type) {
        .ty = STRUCT,
        .member_types = member_types,
        .member_offsets = member_offsets
    };
    expect(__LINE__, 0, get_typesize(&ty_struct));

    add_member(&ty_struct, "c1", &ty_char);
    expect(__LINE__, 8, get_typesize(&ty_struct));

    add_member(&ty_struct, "c2", &ty_char);
    add_member(&ty_struct, "i3", &ty_int);
    expect(__LINE__, 8, get_typesize(&ty_struct));

    add_member(&ty_struct, "i4", &ty_int);
    add_member(&ty_struct, "pi5", &ty_pint);
    add_member(&ty_struct, "i6", &ty_int);
    expect(__LINE__, 32, get_typesize(&ty_struct));

    add_member(&ty_struct, "c7", &ty_char);
    add_member(&ty_struct, "c8", &ty_char);
    add_member(&ty_struct, "arc3_9", &ty_archar);
    expect(__LINE__, 40, get_typesize(&ty_struct));

    add_member(&ty_struct, "ari3_10", &ty_arint);
    expect(__LINE__, 48, get_typesize(&ty_struct));

    // Offset alignment of struct members.
    expect(__LINE__, 0, get_member_offset(&ty_struct, "c1"));
    expect(__LINE__, 1, get_member_offset(&ty_struct, "c2"));
    expect(__LINE__, 4, get_member_offset(&ty_struct, "i3"));
    expect(__LINE__, 8, get_member_offset(&ty_struct, "i4"));
    expect(__LINE__, 16, get_member_offset(&ty_struct, "pi5"));
    expect(__LINE__, 24, get_member_offset(&ty_struct, "i6"));
    expect(__LINE__, 28, get_member_offset(&ty_struct, "c7"));
    expect(__LINE__, 29, get_member_offset(&ty_struct, "c8"));
    expect(__LINE__, 30, get_member_offset(&ty_struct, "arc3_9"));
    expect(__LINE__, 36, get_member_offset(&ty_struct, "ari3_10"));

    fprintf(stderr, "Type size test OK\n");
}

static void type_deduction_test() {
    // Types.
    Type ty_char = (Type) { .ty = CHAR, .ptr_of = NULL, .array_len = 0 };
    Type ty_int = (Type) { .ty = INT, .ptr_of = NULL, .array_len = 0 };
    Type ty_pchar = (Type) { .ty = PTR, .ptr_of = &ty_char, .array_len = 0 };
    Type ty_pint = (Type) { .ty = PTR, .ptr_of = &ty_int, .array_len = 0 };
    Type ty_ppint = (Type) { .ty = PTR, .ptr_of = &ty_pint, .array_len = 0 };

    Node *lhs = calloc(1, sizeof(Node));
    Node *rhs = calloc(1, sizeof(Node));

    // For non-operator-specific type deduction,
    // we specify a dummy operator '_' here.
    // When the type of one side is unknown, expect the type of the known side.
    // NOTE: This behavior should probably be removed later in the development.
    lhs->type = NULL;
    rhs->type = &ty_char;
    expect_type(__LINE__,
            &ty_char,
            deduce_type('_', lhs, rhs));

    lhs->type = &ty_int;
    rhs->type = NULL;
    expect_type(__LINE__,
            &ty_int,
            deduce_type('_', lhs, rhs));

    // When both sides have the same type.
    lhs->type = &ty_char;
    rhs->type = &ty_char;
    expect_type(__LINE__,
            &ty_char,
            deduce_type('_', lhs, rhs));

    lhs->type = &ty_int;
    rhs->type = &ty_int;
    expect_type(__LINE__,
            &ty_int,
            deduce_type('_', lhs, rhs));

    lhs->type = &ty_pchar;
    rhs->type = &ty_pchar;
    expect_type(__LINE__,
            &ty_pchar,
            deduce_type('_', lhs, rhs));

    lhs->type = &ty_pint;
    rhs->type = &ty_pint;
    expect_type(__LINE__,
            &ty_pint,
            deduce_type('_', lhs, rhs));

    lhs->type = &ty_ppint;
    rhs->type = &ty_ppint;
    expect_type(__LINE__,
            &ty_ppint,
            deduce_type('_', lhs, rhs));

    // Assignment expression. The type is that of the lhs.
    lhs->type = &ty_int;
    rhs->type = &ty_pchar;
    expect_type(__LINE__,
            &ty_int,
            deduce_type('=', lhs, rhs));

    lhs->type = &ty_pchar;
    rhs->type = &ty_int;
    expect_type(__LINE__,
            &ty_pchar,
            deduce_type('=', lhs, rhs));

    lhs->type = &ty_int;
    rhs->type = &ty_pint;
    expect_type(__LINE__,
            &ty_int,
            deduce_type('=', lhs, rhs));

    lhs->type = &ty_pint;
    rhs->type = &ty_int;
    expect_type(__LINE__,
            &ty_pint,
            deduce_type('=', lhs, rhs));

    // Pointer arithmatics. Pointer +/- integer makes another poitner.
    lhs->type = &ty_pchar;
    rhs->type = &ty_int;
    expect_type(__LINE__,
            &ty_pchar,
            deduce_type('+', lhs, rhs));

    lhs->type = &ty_char;
    rhs->type = &ty_pchar;
    expect_type(__LINE__,
            &ty_pchar,
            deduce_type('-', lhs, rhs));

    lhs->type = &ty_pint;
    rhs->type = &ty_int;
    expect_type(__LINE__,
            &ty_pint,
            deduce_type('+', lhs, rhs));

    lhs->type = &ty_int;
    rhs->type = &ty_pint;
    expect_type(__LINE__,
            &ty_pint,
            deduce_type('-', lhs, rhs));

    fprintf(stderr, "Type deduction test OK\n");
}

void runtest_type() {
    type_is_basic_type_test();
    type_getsize_test();
    type_deduction_test();
}
