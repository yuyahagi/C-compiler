#include <stdio.h>
#include "cc.h"

// =============================================================================
// Count identifiers in an AST.
// =============================================================================
static void idents_in_statement(const Node *node, Map *idents) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    if (node->ty == '=' && node->lhs && node->lhs->ty == ND_IDENT) {
        map_put(idents, node->lhs->name, (void *)(-8 * (idents->keys->len+1)));
    }
    if (node->lhs) idents_in_statement(node->lhs, idents);
    if (node->rhs) idents_in_statement(node->rhs, idents);
}

Map *idents_in_code(const Vector *code) {
    Map *idents = new_map();
    Node **current = (Node **)code->data;
    while (*current) {
        idents_in_statement(*current, idents);
        ++current;
    }
    return idents;
}

// Generate assembly from an abstract syntax tree.
void gen_lval(Node *node, const Map *idents) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    if (node->ty != ND_IDENT) {
        fprintf(stderr, "Not an identifier.\n");
        exit(1);
    }

    int offset = (int)map_get(idents, node->name);
    printf("  lea rax, [rbp+%d]\n", offset);
    printf("  push rax\n");
}

void gen(Node *node, const Map *idents) {
    switch (node->ty) {
    case ND_NUM:
        printf("  push %d\n", node->val);
        return;

    case ND_IDENT:
        gen_lval(node, idents);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;

    case ND_CALL:
        printf("  xor rax, rax\n");
        printf("  call %s\n", node->name);
        printf("  push rax\n");
        return;

    case '=':
        gen_lval(node->lhs, idents);
        gen(node->rhs, idents);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    }

    gen(node->lhs, idents);
    gen(node->rhs, idents);

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
    case ND_EQUAL:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_NOTEQUAL:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    default:
        fprintf(stderr, "An unexpected operator type %d during assembly generation.\n",
                node->ty);
        exit(1);
    }

    printf("  push rax\n");
}
