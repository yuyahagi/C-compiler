#pragma once
#include <stdbool.h>
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
void runtest_util();

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
    TK_STRING_LITERAL,
    TK_TYPE_CHAR,   // Type specifier
    TK_TYPE_INT,
    TK_IDENT,       // Represents an identifier.
    TK_LESSEQUAL,   // "<=".
    TK_GREATEREQUAL,// ">=".
    TK_EQUAL,       // Equality operator "==".
    TK_NOTEQUAL,    // Nonequality operator "!=".
    TK_INCREMENT,   // "++".
    TK_DECREMENT,   // "--".
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
    ND_FUNCDEF,
    ND_DECLARATION,
    ND_NUM,
    ND_IDENT,
    ND_STRING,      // String literal.
    ND_UEXPR,
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


// =============================================================================
// Types.
// =============================================================================
struct Node;
typedef struct Type {
    enum { CHAR, INT, PTR, ARRAY } ty;
    struct Type *ptr_of;
    size_t array_len;
} Type;

size_t get_typesize(const Type *type);
bool is_basic_type(const Type *type);
Type *deduce_type(int operator, struct Node *lhs, struct Node *rhs);
void runtest_type(void);


typedef struct {
    Type *type;
    int offset;
} Ident;

typedef struct Node {
    int ty;             // Type of node.
    struct Node *lhs;
    struct Node *rhs;
    int val;            // Value of ND_NUM node.
    Type *type;         // For declarations and identifiers.
    char *name;         // For declarations and identifiers.
    Vector *args;       // For function calls.
    struct Node *body;  // Function body (compound statement).

    Vector *stmts;      // Compound statement.
    Map *localvars;     // Local variables in a compound statement.

    int uop;            // Unary operator.
    struct Node *operand;

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
extern Map *globalvars;
extern Map *strings;

Node *new_node(int ty);
Node *new_node_uop(int ty, Node *operand);
Node *new_node_binop(int ty, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node_ident(const Token *tok, Type *type);
Node *new_node_string(const Token *tok);
Node *new_funcdef(const Token *tok);

// Function to parse an expression to abstract syntax trees.
void program(void);
Node *funcdef(void);
Node *extern_declaration(void);
Node *declaration(Map *variables);
Node *direct_declarator(Type *type);
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
Node *unary(void);
Node *postfix(void);
Node *term(void);


// =============================================================================
// Assembly generation.
// =============================================================================
void gen_function(Node *func);
