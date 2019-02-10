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
void *map_get(const Map *map, const char *key);

int max(int x0, int x1);


// =============================================================================
// Tokenization.
// =============================================================================
// Token types.
// Tokens defined as a single letter is directly expressed as its ASCII code.
enum {
    TK_NUM = 256,   // Represents a number.
    TK_TYPE_INT,    // Type specifier, int
    TK_IDENT,       // Represents an identifier.
    TK_LESSEQUAL,   // "<=".
    TK_GREATEREQUAL,// ">=".
    TK_EQUAL,       // Equality operator "==".
    TK_NOTEQUAL,    // Nonequality operator "!=".
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
    TK_RETURN,
    TK_EOF,         // Represents end of input.
};

// Structure for token information.
typedef struct {
    int ty;         // Token type.
    char *input;    // Token string.
    int val;        // Only for TK_NUM. Value of token.
    int len;        // Length of the token string.
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
    ND_BLANK = 256, // Blank statement.
    ND_DECLARATION,
    ND_NUM,
    ND_IDENT,
    ND_LESSEQUAL,
    ND_GREATEREQUAL,
    ND_EQUAL,
    ND_NOTEQUAL,
    ND_IF,
    ND_WHILE,
    ND_FOR,
    ND_RETURN,
    ND_CALL,
    ND_COMPOUND,    // Compound statement.
};

struct Node;

typedef struct {
    char *name;
    Vector *args;
    struct Node *body;
} FuncDef;

typedef struct Node {
    int ty;             // Type of node.
    struct Node *lhs;
    struct Node *rhs;
    int val;            // Value of ND_NUM node.
    char *name;         // For identifiers.
    Vector *args;       // For function calls.

    Vector *stmts;      // Compound statement.

    // Selection statement.
    // cond is also used by iteration statements.
    struct Node *cond;
    struct Node *then;
    struct Node *els;

    // For statement. Use then for the iteration body.
    struct Node *init;
    struct Node *step;
} Node;

// A buffer to store parsed functions.
extern Vector *funcdefs;

Node *new_node(int ty, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_ident(const Token *tok);
FuncDef *new_funcdef(const Token *tok);

// Function to parse an expression to abstract syntax trees.
void program(void);
FuncDef *funcdef(void);
Node *declaration(void);
Node *compound(void);
Node *statement(void);
Node *assign(void);
Node *selection(void);
Node *iteration_while(void);
Node *iteration_for(void);
Node *equal(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *postfix(void);
Node *term(void);


// =============================================================================
// Assembly generation.
// =============================================================================
void gen(Node *node, const Map *idents);
void gen_lval(Node *node, const Map *idents);
void gen_function(FuncDef *func);
