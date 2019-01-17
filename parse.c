#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include "cc.h"

// A buffer to store tokenized code and current position.
Vector *tokens;
size_t pos = 0;

// =============================================================================
// Tokenization.
// =============================================================================
Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

Node *new_node_ident(char name) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_IDENT;
    node->name = name;
    return node;
}

// A helper function to create and stroe a token.
static void push_token(int ty, char *input, int val) {
    Token *tok = calloc(1, sizeof(Token));
    tok->ty = ty;
    tok->input = input;
    tok->val = val;
    vec_push(tokens, (void *)tok);
}

// A helper function to retrieve a token at a given position.
static Token *get_token(int i) {
    return (Token *)tokens->data[i];
}

// Split input string and store to a buffer.
void tokenize(char *p) {
    tokens = new_vector();
    while (*p) {
        // Skip white spaces.
        if (isspace(*p)) {
            ++p;
            continue;
        }

        if (isdigit(*p)) {
            push_token(TK_NUM, p, strtol(p, &p, 10));
            continue;
        }

        // Identifiers.
        // Only suport single-letter lower-case aphabet for now.
        if ('a' <= *p && *p <= 'z') {
            push_token(TK_IDENT, p, 0);
            ++p;
            continue;
        }

        // Equality and nonequality operators.
        if (*p == '=' && *(p+1) == '=') {
            push_token(TK_EQUAL, p, 0);
            p += 2;
            continue;
        }
        if (*p == '!' && *(p+1) == '=') {
            push_token(TK_NOTEQUAL, p, 0);
            p += 2;
            continue;
        }

        // One-letter tokens.
        switch (*p) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '(':
        case ')':
        case ';':
        case '=':
            push_token(*p, p, 0);
            ++p;
            continue;
        }

        fprintf(stderr, "Cannot tokenize %s.\n", p);
        exit(1);
    }

    push_token(TK_EOF, p, 0);
}

// =============================================================================
// Parse tokens into abstract syntax trees.
// =============================================================================
// A buffer to store parsed statements (ASTs).
Vector *code;

// Private utility functions.
static bool consume(int ty) {
    if (get_token(pos)->ty == ty) {
        ++pos;
        return true;
    } else {
        return false;
    }
}

static void expect(int ty) {
    if (get_token(pos)->ty != ty) {
        fprintf(stderr, "A token of type %d expected but was %d.\n", ty, get_token(pos)->ty);
        exit(1);
    }
}

// A function to report parsing errors.
void error(const char *msg, size_t i) {
    fprintf(stderr, "%s \"%s\"\n", msg, get_token(i)->input);
    exit(1);
}

// Parse an expression to an abstract syntax tree.
// program: {statement}*
// statement: assign ";"
// assign: equal assign'
// assign': '' | "=" assign'
// equal: add equal'
// equal': '' | "==" equal | "!=" equal
// add: mul add'
// add': '' | "+" add' | "-" add'
// mul: term | term "*" mul | term "/" mul
// term: num | "(" add ")"
void program(void) {
    code = new_vector();
    pos = 0;
    while (get_token(pos)->ty != TK_EOF) {
        vec_push(code, (void *)statement());
    }
    vec_push(code, (void *)NULL);
    return;
}

Node *statement(void) {
    Node *node = assign();
    if (!consume(';'))
        error("A statement not terminated with ';'.", pos);
    return node;
}

Node *assign(void) {
    Node *lhs = equal();
    if (consume('='))
        return new_node('=', lhs, assign());
    return lhs;
}

Node *equal(void) {
    Node *lhs = add();
    if (get_token(pos)->ty == TK_EQUAL) {
        ++pos;
        return new_node(TK_EQUAL, lhs, equal());
    }
    if (get_token(pos)->ty == TK_NOTEQUAL) {
        ++pos;
        return new_node(TK_NOTEQUAL, lhs, equal());
    }
    return lhs;
}

Node *add(void) {
    Node *lhs = mul();
    if (consume('+')) {
        return new_node('+', lhs, add());
    }
    if (consume('-')) {
        return new_node('-', lhs, add());
    }

    return lhs;
}

// Parse a multiplicative expression.
Node *mul(void) {
    Node *lhs = term();
    switch(get_token(pos)->ty) {
    case '*':
        ++pos;
        return new_node('*', lhs, mul());
    case '/':
        ++pos;
        return new_node('/', lhs, mul());
    default:
        return lhs;
    }
}

// Parse a term (number or expression in pair of parentheses).
Node *term(void) {
    if (get_token(pos)->ty == TK_NUM)
        return new_node_num(get_token(pos++)->val);
    if (get_token(pos)->ty == TK_IDENT)
        return new_node_ident(get_token(pos++)->input[0]);

    if (!consume('('))
        error("A token neither a number nor an opening parenthesis.", pos);
    Node *node = add();
    if (!consume(')'))
        error("A closing parenthesis was expected but not found.", pos);
    return node;
}

