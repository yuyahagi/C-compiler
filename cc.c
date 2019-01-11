#include <stdio.h>
#include <stdlib.h>

// Token types.
// Tokens defined as a single letter is directly expressed as its ASCII code.
enum {
    TK_NUM = 256,   // Represents an integer.
    TK_EOF,         // Represents end of input.
};

// Structure for token information.
typedef struct {
    int ty;         // Token type.
    char *input;    // Token string.
    int val;        // Value of TK_NUM token.
} Token;

// A buffer to store tokenized code.
Token tokens[100];

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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments.\n");
        return 1;
    }

    // Tokenize.
    tokenize(argv[1]);

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // The first token should be a number.
    printf("  mov rax, %d\n", tokens[0].val);

    // Consume "+ <NUMBER>" or "- <NUMBER>" token sequences and generate assembly.
    size_t i = 1;
    while (tokens[i].ty != TK_EOF) {
        switch (tokens[i].ty) {
        case '+':
            ++i;
            if (tokens[i].ty != TK_NUM) error(i);
            printf("  add rax, %d\n", tokens[i].val);
            ++i;
            break;

        case '-':
            ++i;
            if (tokens[i].ty != TK_NUM) error(i);
            printf("  sub rax, %d\n", tokens[i].val);
            ++i;
            break;

        default:
            error(i);
        }
    }

    printf("  ret\n");
    return 0;
}
