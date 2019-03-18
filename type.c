#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "cc.h"

size_t get_typesize(const Type *type) {
    switch (type->ty) {
    case CHAR:
        return 1;
    case INT:
        return 4;
    case PTR:
        return 8;
    case ARRAY:
        return type->array_len * get_typesize(type->ptr_of);
    case STRUCT:
        // TODO: Struct size is not fixed.
        return 8;

    default:
        fprintf(stderr, "An unknown type id %d.\n", type->ty);
        exit(1);
    }
}

bool is_basic_type(const Type *type) {
    return type->ty == CHAR || type->ty == INT;
}

Type *deduce_type(int operator, Node *lhs, Node *rhs) {
    // If one of the side has an unknown type, return the type of the other side.
    // This is necessary primarily because we don't check function signatures at
    // this point of development. This is to be removed later when we hvae proper
    // signature check in place.
    if (!lhs || !lhs->type)
        return rhs->type;
    if (!rhs || !rhs->type)
        return lhs->type;

    // If both sides have the same type, return it.
    if (lhs->type->ty == rhs->type->ty) {
        // TODO: A pointer of (or an array of) different types may be reported as
        // the same type. To be fixed when it becomes necessary.
        return lhs->type;
    }

    // Operator-specific type deductions.
    switch (operator) {
    case '=':
        // Assignment expression. Return type of lhs.
        return lhs->type;

    case '+':
    case '-':
        // Pointer addition/subtraction with an integer.
        // Pointer +/- integer makes a pointer.
        if (lhs->type->ty == PTR && is_basic_type(rhs->type))
            return lhs->type;
        if (is_basic_type(lhs->type) && rhs->type->ty == PTR)
            return rhs->type;
    }

    return lhs->type;
}

