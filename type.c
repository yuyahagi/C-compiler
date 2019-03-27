#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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
    {
        assert(type->member_types);
        int totalsize = 0;
        for (int i = 0; i < type->member_types->keys->len; i++) {
            // Offset of this member. Align to its size boundary.
            int siz = get_typesize((Type *)type->member_types->vals->data[i]);
            if (siz > 1)
                totalsize += (-(totalsize & (siz-1)) & (siz-1));
            totalsize += siz;
        }
        // Align to 8 bytes (assuming totalsize >= 0);
        totalsize += (-(totalsize & 7)) & 7;
        assert(totalsize % 8 == 0);
        return totalsize;
    }

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

void add_member(Type *struct_type, const char *member_name, Type *member_type) {
    Map *member_types = struct_type->member_types;
    Map *member_offsets = struct_type->member_offsets;
    map_put(member_types, member_name, member_type);
    // Store the offset.
    // Offset of last member before this one.
    int n = member_offsets->keys->len;
    //offset = 4 * n;
    int offset = 0;
    if (n > 0) {
        Type *last_member = (Type *)member_types->vals->data[n-1];
        int last = (int)member_offsets->vals->data[n-1];
        offset = last + (int)get_typesize(last_member);
    }
    int siz = get_typesize(member_type);
    if (siz > 1)
        offset += (-(offset & (siz-1)) & (siz-1));
    map_put(member_offsets, member_name, offset);
}

int get_member_offset(const Type *type, const char *member_name) {
    return (int)map_get(type->member_offsets, member_name);
}
