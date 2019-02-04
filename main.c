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

    for (int i = 0; i < funcdefs->len; i++) {
        FuncDef *func = (FuncDef *)funcdefs->data[i];
        gen_function(func);
        ++func;
    }
    return 0;
}
