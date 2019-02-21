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

    // Global variables.
    printf(".bss\n");
    for (int i = 0; i < globalvars->keys->len; i++) {
        char *name = (char *)globalvars->keys->data[i];
        Ident *ident = (Ident *)globalvars->vals->data[i];
        // TODO: Currently only assuming an int type. Adjust size according to type.
        size_t siz = 4;
        printf(".global %s\n", name);
        printf("%s:\n", name);
        printf("  .zero %d\n", siz);
    }

    printf(".text\n");
    for (int i = 0; i < funcdefs->len; i++) {
        Node *func = (Node *)funcdefs->data[i];
        gen_function(func);
        ++func;
    }
    return 0;
}
