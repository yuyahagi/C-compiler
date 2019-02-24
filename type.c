#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "cc.h"

size_t get_typesize(const Type *type) {
    switch (type->ty) {
    case INT:
        return 4;
    case PTR:
        return 8;
    case ARRAY:
        return type->array_len * get_typesize(type->ptr_of);

    default:
        fprintf(stderr, "An unknown type id %d.\n", type->ty);
        exit(1);
    }
}

Type *deduce_type(Node *lhs, Node *rhs) {
    if (lhs)
        return lhs->type;
    return NULL;
}

