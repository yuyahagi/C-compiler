#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "cc.h"

char *read_file(char *path) {
    FILE *pf = fopen(path, "r");
    if (!pf) {
        fprintf(stderr, "Could not open %s: %s\n", path, strerror(errno));
        exit(1);
    }

    if (fseek(pf, 0, SEEK_END   ) == -1) {
        fprintf(stderr, "Could not seek to the end of %s: %s\n", path, strerror(errno));
        exit(1);
    }
    size_t filesize = ftell(pf);
    if (fseek(pf, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Could not seek to the beginning of %s: %s\n", path, strerror(errno));
        exit(1);
    }

    char *buf = malloc(filesize + 1);
    fread(buf, filesize, 1, pf);
    buf[filesize] = '\0';
    fclose(pf);

    return buf;
}

int main(int argc, char **argv) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments.\n");
        return 1;
    }

    // Test.
    if (strcmp(argv[1], "-test") == 0) {
        runtest_util();
        runtest_type();
        return 0;
    }
    
    char *src = read_file(argv[1]);

    // Tokenize and parse to abstract syntax tree.
    tokenize(src);
    program();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");

    // Global variables.
    for (int i = 0; i < globalvars->keys->len; i++) {
        char *name = (char *)globalvars->keys->data[i];
        Node *var = (Node *)globalvars->vals->data[i];
        Type *type = var->type;
        size_t siz = get_typesize(type);
        if (var->declinit) {
            assert(var->declinit->ty == ND_NUM);
            assert(var->declinit->type->ty == INT);
            printf(".data\n");
            printf(".global %s\n", name);
            printf("%s:\n", name);
            printf("  .long %d\n", var->declinit->val);
        }
        else {
            printf(".bss\n");
            printf(".global %s\n", name);
            printf("%s:\n", name);
            printf("  .zero %zu\n", siz);
        }
    }

    // String literals.
    printf(".section .rodata\n");
    for (int i = 0; i < strings->keys->len; i++) {
        printf(".LC%d:\n", (int)strings->vals->data[i]);
        printf("  .string \"%s\"\n", (char *)strings->keys->data[i]);
    }

    printf(".text\n");
    for (int i = 0; i < funcdefs->len; i++) {
        Node *func = (Node *)funcdefs->data[i];
        gen_function(func);
        ++func;
    }
    return 0;
}
