#include <stdio.h>
#include <stdlib.h>

// Token types.
// Tokens defined as a single letter is directly expressed as its ASCII code.
enum {
    TK_NUM = 256,   // Represents a number.
    TK_EOF,         // Represents end of input.
};

// Structure for token information.
typedef struct {
    int ty;         // Token type.
    char *input;    // Token string.
    int val;        // Value of TK_NUM token.
} Token;

// A buffer to store tokenized code and current position.
Token tokens[100];
size_t pos = 0;

// Node types.
// Tokens defined as a single letter is expressed with its ASCII code.
enum {
    ND_NUM = 256,
};

typedef struct Node {
    int ty;             // Type of node.
    struct Node *lhs;
    struct Node *rhs;
    int val;            // Value of ND_NUM node.
} Node;

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

// Split input string and store to a buffer.
tokenize(char *p) {
    int i = 0;
    while (*p) {
        // Skip white spaces.
        if (isspace(*p)) {
            ++p;
            continue;
        }

        if (*p == '+' || *p == '-') {
            tokens[i].ty = *p;
            tokens[i].input = p;
            ++i;
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

        fprintf(stderr, "Cannot tokenize %s.\n", p);
        exit(1);
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;
}

// A function to report errors.
void error(size_t i) {
    fprintf(stderr, "An unexpeced token: %s.\n", tokens[i].input);
    exit(1);
}

// Parse an expression to an abstract syntax tree.
Node *expr() {
    Node *lhs = new_node_num(tokens[pos++].val);
    if (tokens[pos].ty == '+') {
        ++pos;
        return new_node('+', lhs, expr());
    }
    if (tokens[pos].ty == '-') {
        ++pos;
        return new_node('-', lhs, expr());
    }
    return lhs;
}

// Generate assembly from an abstract syntax tree.
void gen(Node *node) {
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->ty) {
    case '+':
        printf("  add rax, rdi\n");
        break;
    case '-':
        printf("  sub rax, rdi\n");
        break;
    default:
        fprintf(stderr, "An unexpected operator type during assembly generation.\n");
    }

    printf("  push rax\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments.\n");
        return 1;
    }

    // Tokenize and parse to abstract syntax tree.
    tokenize(argv[1]);
    Node *node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Generate assembly from the AST.
    gen(node);

    // The value of the entire expression should have been pushed at the top of the stack.
    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
