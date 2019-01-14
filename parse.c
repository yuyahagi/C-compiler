#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include "cc.h"

// A buffer to store tokenized code and current position.
Token tokens[100];
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

// Split input string and store to a buffer.
void tokenize(char *p) {
    int i = 0;
    while (*p) {
        // Skip white spaces.
        if (isspace(*p)) {
            ++p;
            continue;
        }

        if (isdigit(*p)) {
            tokens[i].ty = TK_NUM;
            tokens[i].input = p;
            tokens[i].val = strtol(p, &p, 10);
            ++i;
            continue;
        }

        // Identifiers.
        // Only suport single-letter lower-case aphabet for now.
        if ('a' <= *p && *p <= 'z') {
            tokens[i].ty = TK_IDENT;
            tokens[i].input = p;
            ++i;
            ++p;
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
            tokens[i].ty = *p;
            tokens[i].input = p;
            ++i;
            ++p;
            continue;
        }

        fprintf(stderr, "Cannot tokenize %s.\n", p);
        exit(1);
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;
}

// =============================================================================
// Parse tokens into abstract syntax trees.
// =============================================================================
// A buffer to store parsed statements (ASTs).
Node *code[100];

// Private utility functions.
bool consume(int ty) {
    if (tokens[pos].ty == ty) {
        ++pos;
        return true;
    } else {
        return false;
    }
}

void expect(int ty) {
    if (tokens[pos].ty != ty) {
        fprintf(stderr, "A token of type %d expected but was %d.\n", ty, tokens[pos].ty);
        exit(1);
    }
}

// A function to report parsing errors.
void error(const char *msg, size_t i) {
    fprintf(stderr, "%s \"%s\"\n", msg, tokens[i].input);
    exit(1);
}

// Parse an expression to an abstract syntax tree.
// program: {assign}*
// assign: add assign' ";"
// assign': '' | "=" assign'
// add: mul add'
// add': '' | "+" add' | "-" add'
// mul: term | term "*" mul | term "/" mul
// term: num | "(" add ")"
void program(void) {
    int i = 0;
    while(tokens[pos].ty != TK_EOF) {
        code[i++] = assign();
    }
    return;
}

Node *assign(void) {
    Node *lhs = add();
    if (consume('='))
        return new_node('=', lhs, assign());
    if (consume(';'))
        return lhs;
    error("A statement not terminated with ';'.", pos);
    return NULL;
}

// Parse an expression.
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
    switch(tokens[pos].ty) {
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
    if (tokens[pos].ty == TK_NUM)
        return new_node_num(tokens[pos++].val);
    if (tokens[pos].ty == TK_IDENT)
        return new_node_ident(tokens[pos++].input[0]);

    if (!consume('('))
        error("A token neither a number nor an opening parenthesis.", pos);
    Node *node = add();
    if (!consume(')'))
        error("A closing parenthesis was expected but not found.", pos);
    return node;
}

