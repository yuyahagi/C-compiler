#include <stdio.h>
#include "cc.h"

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
