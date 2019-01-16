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
