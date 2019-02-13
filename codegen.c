#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "cc.h"

// Counter for generating labels.
static int nlabel = 0;

// =============================================================================
// Count identifiers in an AST.
// =============================================================================
static Type *make_type(int type_enum, Type *ptr_of) {
    Type *t = calloc(1, sizeof(Type));
    t->ty = type_enum;
    t->ptr_of = ptr_of;
    return t;
}

static void put_ident(Map *idents, char *name, Type *type, int offset) {
    Ident *ident = malloc(sizeof(Ident));
    ident->type = type;
    ident->offset = offset;
    map_put(idents, name, (void *)(ident));
}

static void decls_to_offsets(const Vector *code, Map *idents) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    // Search for declarations and assign offsets.
    for (int i = 0; i < code->len; i++) {
        Node *node = (Node *)code->data[i];
        if (node->ty != ND_DECLARATION)
            continue;

        put_ident(idents, node->name, node->type, -8 * (idents->keys->len+1));
    }
}

// Count identifiers in a compound statement and assign offset.
static void idents_in_func(const FuncDef *func, Map *idents) {
    // First 6 function parameters are to be copied to the stack.
    int nargs = func->args->len;
    int nregargs = nargs <= 6 ? nargs : 6;
    int nstackargs = nargs - nregargs;
    for (int i = 0; i < nregargs; i++) {
        put_ident(
            idents,
            ((Node *)func->args->data[i])->name,
            make_type(INT, NULL),
            -8 * (idents->keys->len+1));
    }
    // The rest of args are in stack. Store positive offsets.
    for (int i = nstackargs - 1; i >= 0; i--) {
        put_ident(
            idents,
            ((Node *)func->args->data[i+6])->name,
            make_type(INT, NULL),
            8 * (i + 1));
    }
    // Count identifiers in the function body and assign offsets.
    decls_to_offsets(func->body->stmts, idents);
}

// Track stack position for adjusting alignment.
static int stackpos = 0;

// =============================================================================
// Assembly generation from an AST.
// =============================================================================
static void push_imm32(int imm) {
    printf("  push %d\n", imm);
    stackpos += 8;
}

static void push(const char *reg) {
    printf("  push %s\n", reg);
    stackpos += 8;
}

static void pop(const char *reg) {
    printf("  pop %s\n", reg);
    stackpos -= 8;
    assert(stackpos >= 0);
}

void gen_lval(Node *node, const Map *idents) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    switch (node->ty) {
    case ND_IDENT:
    {
        Ident *ident = (Ident *)map_get(idents, node->name);
        int offset = ident->offset;
        if (offset == 0) {
            fprintf(stderr, "An unknown identifier %s.\n", node->name);
            exit(1);
        }
        printf("  lea rax, [rbp%+d]\n", offset);
        return;
    }

    case ND_UEXPR:
        assert(node->uop == '*');
        gen_lval(node->operand, idents);
        printf("  mov rax, [rax]\n");
        return;

    default:
        fprintf(stderr, "Attempted to generate an invalid node as an lvalue. %d.\n", node->ty);
        exit(1);
    }
}

void gen(Node *node, const Map *idents) {
    switch (node->ty) {
    case ND_BLANK:
        return;

    case ND_DECLARATION:
        return;

    case ND_NUM:
        printf("  mov rax, %d\n", node->val);
        return;

    case ND_IDENT:
        gen_lval(node, idents);
        printf("  mov rax, [rax]\n");
        return;

    case ND_UEXPR:
        switch (node->uop) {
        case '&':
            gen_lval(node->operand, idents);
            break;
        case '*':
            gen_lval(node, idents);
            printf("  mov rax, [rax]\n");
            break;
        default:
            fprintf(stderr, "Unknown unary operator %c.\n", node->uop);
            exit(1);
        }
        return;

    case ND_CALL:
    {
        int nargs = node->args->len;
        int nregargs = nargs <= 6 ? nargs : 6;
        int nstackargs = nargs - nregargs;
        char *regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };

        // Align stack pointer to 16 bytes.
        int orig_stackpos = stackpos;
        bool align_stack = (stackpos + 8 * nstackargs) % 16 != 0;
        if (align_stack) {
            printf("  sub rsp, 8\n");
            stackpos += 8;
        }

        // Evaluate argument expressions.
        for (int i = nargs - 1; i >= 0; i--) {
            gen(node->args->data[i], idents);
            push("rax");
        }

        // Assign first 6 args to registers. Leave the rest on the stack.
        for (int i = 0; i < nregargs; i++)
            pop(regs[i]);

        printf("  xor rax, rax\n");
        printf("  call %s\n", node->name);

        // Remove stack-passed args.
        if (nstackargs > 0) {
            printf("  sub rsp, %d\n", 8 * nstackargs);
            stackpos -= 8 * nstackargs;
        }

        if (align_stack) {
            printf("  add rsp, 8\n");
            stackpos -= 8;
        }
        assert(stackpos == orig_stackpos);

        return;
    }

    case ND_COMPOUND:
    {
        for (int i = 0; i < node->stmts->len; i++) {
            // The value of the entire expression is at the top of the stack.
            // Pop it to keep the correct counting.
            Node *currentNode = (Node *)node->stmts->data[i];
            gen(currentNode, idents);
        }
        return;
    }

    case ND_IF:
    {
        int lbl_else = nlabel++;
        int lbl_last = nlabel++;

        gen(node->cond, idents);
        printf("  cmp rax, 0\n");
        printf("  je .L%d\n", lbl_else);

        gen(node->then, idents);
        printf("  jmp .L%d\n", lbl_last);

        printf(".L%d:\n", lbl_else);
        if (node->els != NULL) {
            gen(node->els, idents);
        }
        printf(".L%d:\n", lbl_last);
        return;
    }

    case ND_WHILE:
    {
        int lbl_beg = nlabel++;
        int lbl_end = nlabel++;

        printf(".L%d:\n", lbl_beg);
        // Condition check.
        gen(node->cond, idents);
        printf("  cmp rax, 0\n");
        printf("  je .L%d\n", lbl_end);

        gen(node->then, idents);

        printf("  jmp .L%d\n", lbl_beg);
        printf(".L%d:\n", lbl_end);
        return;
    }

    case ND_FOR:
    {
        int lbl_beg = nlabel++;
        int lbl_end = nlabel++;

        gen(node->init, idents);
        printf(".L%d:\n", lbl_beg);

        gen(node->cond, idents);
        printf("  cmp rax, 0\n");
        printf("  je .L%d\n", lbl_end);

        gen(node->then, idents);

        gen(node->step, idents);
        printf("  jmp .L%d\n", lbl_beg);
        printf(".L%d:\n", lbl_end);
        return;
    }

    case ND_RETURN:
        if (node->rhs) {
            gen(node->rhs, idents);
        }
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;

    case '=':
        gen_lval(node->lhs, idents);
        push("rax");
        gen(node->rhs, idents);

        pop("rdi");
        printf("  mov [rdi], rax\n");
        return;
    }

    // Binary operators.
    gen(node->lhs, idents);
    push("rax");
    gen(node->rhs, idents);
    push("rax");

    pop("rdi");
    pop("rax");

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
    case '<':
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case '>':
        printf("  cmp rax, rdi\n");
        printf("  setg al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LESSEQUAL:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_GREATEREQUAL:
        printf("  cmp rax, rdi\n");
        printf("  setge al\n");
        printf("  movzb rax, al\n");
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
}

void gen_function(FuncDef *func) {
    stackpos = 0;
    printf("%s:\n", func->name);
    push("rbp");
    printf("  mov rbp, rsp\n");

    // Count number of used identifiers (including function parameters) and
    // allocate stack for local variables. If an identifier gets redefined,
    // it may count it twice or more but it's ok.
    Map *idents = new_map();
    idents_in_func(func, idents);
    printf("  sub rsp, %d\n", 8 * idents->keys->len);
    stackpos += 8 * idents->keys->len;

    // First 6 function parameters are in registers. Copy them to stack.
    const char *regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
    int nargs = func->args->len;
    int nregargs = nargs <= 6 ? nargs : 6;
    for (int i = 0; i < nregargs; i++) {
        char *param_name = ((Node *)func->args->data[i])->name;
        Ident *ident = (Ident *)map_get(idents, param_name);
        printf("  mov [rbp%+d], %s\n", ident->offset, regs[i]);
    }

    // Generate assembly from the ASTs.
    gen(func->body, idents);

    // End of function. Return default int.
    // This will likely emit a redundant function epilogue after a return statement.
    // It is at least functionally ok since this will not be executed.
    printf("  xor rax, rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}

