#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Token types.
// Tokens defined as a single letter is directly expressed as its ASCII code.
enum {
    TK_NUM = 256,   // Represents a number.
    TK_IDENT,       // Represents an identifier.
    TK_EOF,         // Represents end of input.
};

// Structure for token information.
typedef struct {
    int ty;         // Token type.
    char *input;    // Token string.
    int val;        // Only for TK_NUM. Value of token.
} Token;

// A buffer to store tokenized code and current position.
Token tokens[100];
size_t pos = 0;

// Node types.
// Tokens defined as a single letter is expressed with its ASCII code.
enum {
    ND_NUM = 256,
    ND_IDENT,
};

typedef struct Node {
    int ty;             // Type of node.
    struct Node *lhs;
    struct Node *rhs;
    int val;            // Value of ND_NUM node.
    char name;          // Only for TK_IDENT.
} Node;

// A buffer to store parsed statements (ASTs).
Node *code[100];

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

// Parse an expression to an abstract syntax tree.
// program: {assign}*
// assign: add assign' ";"
// assign': '' | "=" assign'
// add: mul add'
// add': '' | "+" add' | "-" add'
// mul: term | term "*" mul | term "/" mul
// term: num | "(" add ")"
void program(void);
Node *assign(void);
Node *add(void);
Node *mul(void);
Node *term(void);

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

// Generate assembly from an abstract syntax tree.
void gen_lval(Node *node) {
    if (node->ty != ND_IDENT) {
        fprintf(stderr, "Not an identifier.\n");
        exit(1);
    }

    int offset = -8 * (node->name - 'a' + 1);

    printf("  lea rax, [rbp+%d]\n", offset);
    printf("  push rax\n");
}

void gen(Node *node) {
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    if (node->ty == ND_IDENT) {
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }

    if (node->ty == '=') {
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
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
    case '*':
        printf("  mul rdi\n");
        break;
    case '/':
        printf("  xor rdx, rdx\n");
        printf("  div rdi\n");
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
    program();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    // Space for 26 local variables (== 208 bytes).
    printf("  sub rsp, 208\n");

    // Generate assembly from the ASTs.
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // The value of the entire expression is at the top of the stack.
        // Pop it to keep the correct counting.
        printf("  pop rax\n");
    }

    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
