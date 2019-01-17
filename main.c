#include <stdio.h>
#include <string.h>
#include "cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments.\n");
        return 1;
    }

    // Test.
    if (strcmp(argv[1], "-test") == 0) {
        runtest();
        return 0;
    }

    // Tokenize and parse to abstract syntax tree.
    tokenize(argv[1]);
    program();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    
    // Count number of used identifiers and allocate stack for local variables.
    // If an identifier gets redefined, it may count it twice or more but it's ok.
    Map *idents = idents_in_code(code);
    printf("  sub rsp, %d\n", 8 * idents->keys->len);

    // Generate assembly from the ASTs.
    Node **currentNode = (Node **)code->data;
    while (*currentNode) {
        gen(*currentNode++, idents);

        // The value of the entire expression is at the top of the stack.
        // Pop it to keep the correct counting.
        printf("  pop rax\n");
    }

    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
