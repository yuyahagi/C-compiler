#include <assert.h>
#include <string.h>
#include "cc.h"

Vector *new_vector() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector *vec, const void *elem) {
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

const void *vec_peek_last(Vector *vec) {
    assert(vec->len > 0);
    return vec->data[vec->len-1];
}

Map *new_map() {
    Map *map = malloc(sizeof(Map));
    map->keys = new_vector();
    map->vals = new_vector();
    return map;
}

void map_put(Map *map, const char *key, const void *val) {
    vec_push(map->keys, (void *)key);
    vec_push(map->vals, val);
}

const void *map_get(const Map *map, const char *key) {
    for (int i = map->keys->len - 1; i >= 0; i--)
        if (strcmp(map->keys->data[i], key) == 0)
            return map->vals->data[i];
    return NULL;
}

int max(int x0, int x1) {
    return x0 > x1 ? x0 : x1;
}
