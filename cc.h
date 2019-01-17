#pragma once
#include <stdlib.h>
#include "cc.h"

// =============================================================================
// Data structures.
// =============================================================================
typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *new_vector();
void vec_push(Vector *vec, void *elem);
void runtest();

typedef struct {
    Vector *keys;
    Vector *vals;
} Map;

Map *new_map();
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);


// =============================================================================
// Tokenization.
// =============================================================================
// Token types.
// Tokens defined as a single letter is directly expressed as its ASCII code.
enum {
    TK_NUM = 256,   // Represents a number.
    TK_IDENT,       // Represents an identifier.
    TK_EQUAL,       // Equality operator "==".
    TK_NOTEQUAL,    // Nonequality operator "!=".
    TK_EOF,         // Represents end of input.
};

// Structure for token information.
typedef struct {
    int ty;         // Token type.
    char *input;    // Token string.
    int val;        // Only for TK_NUM. Value of token.
} Token;

// A buffer to store tokenized code and current position.
extern Vector *tokens;
extern size_t pos;

void tokenize(char *p);


// =============================================================================
// Parse tokens into abstract syntax trees.
// =============================================================================
// Node types.
// Tokens defined as a single letter is expressed with its ASCII code.
enum {
    ND_NUM = 256,
    ND_IDENT,
    ND_EQUAL,
    ND_NOTEQUAL,
};

typedef struct Node {
    int ty;             // Type of node.
    struct Node *lhs;
    struct Node *rhs;
    int val;            // Value of ND_NUM node.
    char name;          // Only for TK_IDENT.
} Node;

// A buffer to store parsed statements (ASTs).
extern Vector *code;

Node *new_node(int ty, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_ident(char name);

// Function to parse an expression to abstract syntax trees.
void program(void);
Node *statement(void);
Node *assign(void);
Node *equal(void);
Node *add(void);
Node *mul(void);
Node *term(void);


// =============================================================================
// Assembly generation.
// =============================================================================
void gen(Node *node);
void gen_lval(Node *node);
