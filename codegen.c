#include <assert.h>
#include <ctype.h>
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

static int decls_to_offsets(const Vector *code, Map *idents, int starting_offset) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    // Search for declarations and assign offsets.
    int offset = starting_offset;
    for (int i = 0; i < code->len; i++) {
        Node *node = (Node *)code->data[i];
        if (node->ty != ND_DECLARATION)
            continue;

        switch (node->type->ty) {
        case CHAR:
        case INT:
        case PTR:
            put_ident(idents, node->name, node->type, offset);
            offset -= 8;
            break;
        case ARRAY:
            offset -=
                get_typesize(node->type->ptr_of)
                * node->type->array_len;
            // Align to 8 bytes (assuming offset <= 0);
            offset -= offset & 7;
            put_ident(idents, node->name, node->type, offset + 8);
            assert(offset % 8 == 0);
            break;
        default:
            fprintf(stderr, "Unknown type id %d.\n", node->type->ty);
            exit(1);
        }
    }
    return offset;
}

// Count identifiers in a compound statement and assign offset.
static int idents_in_func(const Node *func, Map *idents) {
    int offset = -8;
    // First 6 function parameters are to be copied to the stack.
    int nargs = func->fargs->len;
    int nregargs = nargs <= 6 ? nargs : 6;
    int nstackargs = nargs - nregargs;
    for (int i = 0; i < nregargs; i++) {
        put_ident(
            idents,
            ((Node *)func->fargs->data[i])->name,
            make_type(INT, NULL),
            offset);
        offset -= 8;
    }
    // The rest of args are in stack. Store positive offsets,
    // skipping pushed rbp and the return address.
    for (int i = nstackargs - 1; i >= 0; i--) {
        put_ident(
            idents,
            ((Node *)func->fargs->data[i+6])->name,
            make_type(INT, NULL),
            8 * (i + 2));
    }
    // Count identifiers in the function body and assign offsets.
    int offset_end = decls_to_offsets(func->fbody->stmts, idents, offset);
    return offset_end + 8;
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

static void gen_typed_rax_dereference(const Type *type) {
    switch(get_typesize(type)) {
    case 1:
        printf("  movzx eax, byte ptr [rax]\n");
        return;
    case 4:
        printf("  mov eax, dword ptr [rax]\n");
        return;
    case 8:
        printf("  mov rax, qword ptr [rax]\n");
        return;
    default:
        fprintf(stderr, "An unpredicted type size %zu.\n", get_typesize(type));
        exit(1);
    }
}

static void gen_typed_mov_rax_to_ptr_rdi(const Type *type) {
    size_t siz = get_typesize(type);
    switch (siz) {
    case 1:
        printf("  mov byte ptr [rdi], al\n");
        return;
    case 4:
        printf("  mov dword ptr [rdi], eax\n");
        return;
    case 8:
        printf("  mov [rdi], rax\n");
        return;
    default:
        fprintf(stderr, "An unpredicted type size %zu.\n", siz);
        exit(1);
    }
}

static void gen_typed_cmp_rax_to_0(const Type *type) {
    size_t siz = get_typesize(type);
    switch (siz) {
    case 1:
        printf("  cmp al, 0\n");
        return;
    case 4:
        printf("  cmp eax, 0\n");
        return;
    case 8:
        printf("  cmp rax, 0\n");
        return;
    default:
        fprintf(stderr, "An unpredicted type size %zu.\n", siz);
        exit(1);
    }
}

static void gen_lval(Node* node, const Map *idents);
static void gen_add(int ty, Node *lhs, Node *rhs, const Map *idents);
static void gen(Node *node, const Map *idents);

static void gen_lval(Node *node, const Map *idents) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    switch (node->ty) {
    case ND_IDENT:
    case ND_DECLARATION:
    {
        Ident *ident = (Ident *)map_get(idents, node->name);
        if (ident) {
            // Local variable found.
            int offset = ident->offset;
            printf("  lea rax, [rbp%+d]\n", offset);
            return;
        }

        // Local variable not found. Look for a global.
        Node *var = (Node *)map_get(globalvars, node->name);
        Type *type = var->type;
        if (type) {
            printf("  lea rax, dword ptr %s[rip]\n", node->name);
            return;
        }

        fprintf(stderr, "An unknown identifier %s.\n", node->name);
        exit(1);
    }

    case ND_UEXPR:
        assert(node->uop == '*');
        if (node->operand->type->ty == ARRAY) {
            // An array. Keep address itself.
            gen_lval(node->operand, idents);
        } else {
            // A pointer. Take the content and treat it as an address.
            gen(node->operand, idents);
        }
        return;

    case '+':
    case '-':   // Fall through.
        gen_add(node->ty, node->lhs, node->rhs, idents);
        return;

    default:
        fprintf(stderr, "Attempted to generate an invalid node as an lvalue. %d.\n", node->ty);
        exit(1);
    }
}

static void gen_add(int ty, Node *lhs, Node *rhs, const Map *idents) {
    assert(ty == '+' || ty == '-');
    assert(lhs->type);
    assert(rhs->type);

    bool lhs_is_ptr = lhs->type->ty == PTR || lhs->type->ty == ARRAY;
    bool rhs_is_ptr = rhs->type->ty == PTR || rhs->type->ty == ARRAY;
    if (lhs_is_ptr && rhs_is_ptr) {
        fprintf(stderr, "Pointer +/- pointer operation not supported (yet).\n");
        exit(1);
    }

    // Integer type or pointer arithmatic.
    gen(lhs, idents);
    if (rhs_is_ptr) {
        size_t ptrsize = get_typesize(rhs->type->ptr_of);
        printf("  mov rdi, %zu\n", ptrsize);
        printf("  mul rdi\n");
    }
    push("rax");
    gen(rhs, idents);
    if (lhs_is_ptr) {
        size_t ptrsize = get_typesize(lhs->type->ptr_of);
        printf("  mov rdi, %zu\n", ptrsize);
        printf("  mul rdi\n");
    }
    push("rax");
    pop("rdi");
    pop("rax");

    if (ty == '+')
        printf("  add rax, rdi\n");
    else 
        printf("  sub rax, rdi\n");
}

static void gen(Node *node, const Map *idents) {
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    switch (node->ty) {
    case ND_BLANK:
        return;

    case ND_DECLARATION:
        if (node->declinit) {
            gen_lval(node, idents);
            push("rax");
            gen(node->declinit, idents);
            pop("rdi");
            gen_typed_mov_rax_to_ptr_rdi(node->type);
        }
        return;

    case ND_NUM:
        printf("  mov rax, %d\n", node->val);
        return;

    case ND_IDENT:
        gen_lval(node, idents);
        // If the identifier is an array, we take its address as its value.
        // Otherwise, we take the value at its address.
        if (node->type->ty != ARRAY) {
            gen_typed_rax_dereference(node->type);
        }
        return;

    case ND_STRING:
        printf("  mov qword ptr [rsp-8], offset flat:.LC%d\n", (int)map_get(strings, node->name));
        printf("  mov rax, qword ptr [rsp-8]\n");
        return;

    case ND_UEXPR:
        switch (node->uop) {
        case TK_INCREMENT:
            gen_lval(node->operand, idents);
            push("rax");
            gen_add(
                '+',
                node->operand,
                &(Node) {
                    .ty = ND_NUM,
                    .val = 1,
                    .type = &(Type) {
                        .ty = INT,
                        .ptr_of = NULL,
                        .array_len = 0
                    }
                },
                idents);
            pop("rdi");
            gen_typed_mov_rax_to_ptr_rdi(node->type);
            break;

        case TK_DECREMENT:
            gen_lval(node->operand, idents);
            push("rax");
            gen_add(
                '-',
                node->operand,
                &(Node) {
                    .ty = ND_NUM,
                    .val = 1,
                    .type = &(Type) {
                        .ty = INT,
                        .ptr_of = NULL,
                        .array_len = 0
                    }
                },
                idents);
            pop("rdi");
            gen_typed_mov_rax_to_ptr_rdi(node->type);
            break;

        case '&':
            gen_lval(node->operand, idents);
            break;
        case '*':
            gen_lval(node, idents);
            gen_typed_rax_dereference(node->type);
            break;
        default:
            if (isprint(node->uop))
                fprintf(stderr, "Unknown unary operator %c.\n", node->uop);
            else
                fprintf(stderr, "Unknown unary operator %d.\n", node->uop);
            exit(1);
        }
        return;

    case ND_CALL:
    {
        int nargs = node->fargs->len;
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
            gen(node->fargs->data[i], idents);
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
        gen_typed_cmp_rax_to_0(node->cond->type);
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
        gen(node->itercond, idents);
        gen_typed_cmp_rax_to_0(node->itercond->type);
        printf("  je .L%d\n", lbl_end);

        gen(node->iterbody, idents);

        printf("  jmp .L%d\n", lbl_beg);
        printf(".L%d:\n", lbl_end);
        return;
    }

    case ND_FOR:
    {
        int lbl_beg = nlabel++;
        int lbl_end = nlabel++;

        gen(node->iterinit, idents);
        printf(".L%d:\n", lbl_beg);

        gen(node->itercond, idents);
        gen_typed_cmp_rax_to_0(node->itercond->type);
        printf("  je .L%d\n", lbl_end);

        gen(node->iterbody, idents);

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
        gen_typed_mov_rax_to_ptr_rdi(node->lhs->type);
        return;

    case '+':
    case '-':   // Fall through.
        gen_add(node->ty, node->lhs, node->rhs, idents);
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
    case '*':
        printf("  mul rdi\n");
        break;
    case '/':
        printf("  xor rdx, rdx\n");
        printf("  div rdi\n");
        break;
    case '<':
        printf("  cmp eax, edi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case '>':
        printf("  cmp eax, edi\n");
        printf("  setg al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LESSEQUAL:
        printf("  cmp eax, edi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_GREATEREQUAL:
        printf("  cmp eax, edi\n");
        printf("  setge al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_EQUAL:
        printf("  cmp eax, edi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_NOTEQUAL:
        printf("  cmp eax, edi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    default:
        fprintf(stderr, "An unexpected operator type %d during assembly generation.\n",
                node->ty);
        exit(1);
    }
}

void gen_function(Node *func) {
    stackpos = 0;
    printf("%s:\n", func->fname);
    push("rbp");
    printf("  mov rbp, rsp\n");

    // Count number of used identifiers (including function parameters) and
    // allocate stack for local variables. If an identifier gets redefined,
    // it may count it twice or more but it's ok.
    Map *idents = new_map();
    int stack_offset = idents_in_func(func, idents);
    assert(stack_offset <= 0);
    printf("  sub rsp, %d\n", -stack_offset);
    stackpos += -stack_offset;

    // First 6 function parameters are in registers. Copy them to stack.
    const char *regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
    int nargs = func->fargs->len;
    int nregargs = nargs <= 6 ? nargs : 6;
    for (int i = 0; i < nregargs; i++) {
        char *param_name = ((Node *)func->fargs->data[i])->name;
        Ident *ident = (Ident *)map_get(idents, param_name);
        printf("  mov [rbp%+d], %s\n", ident->offset, regs[i]);
    }

    // Generate assembly from the ASTs.
    gen(func->fbody, idents);

    // End of function. Return default int.
    // This will likely emit a redundant function epilogue after a return statement.
    // It is at least functionally ok since this will not be executed.
    printf("  xor rax, rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}

