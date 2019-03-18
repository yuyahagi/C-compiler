#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "cc.h"

// A buffer to store tokenized code and current position.
Vector *tokens;
size_t pos = 0;


// =============================================================================
// Tokenization.
// =============================================================================
Map *globalvars = NULL;
static Map *localvars = NULL;

Node *new_node(int ty) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ty;
    return node;
}

Node *new_node_uop(int operator, Node *operand) {
    assert(operator == TK_INCREMENT
        || operator == TK_DECREMENT
        || operator == '*' 
        || operator == '&');
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_UEXPR;
    node->uop = operator;
    node->operand = operand;
    // Deduce type.
    Type *type = NULL;
    switch (operator) {
    case TK_INCREMENT:
    case TK_DECREMENT:
        type = operand->type;
        break;
    case '*':
        type = operand->type->ptr_of;
        break;
    case '&':
        type = calloc(1, sizeof(Type));
        type->ty = PTR;
        type->ptr_of = operand->type;
        break;
    }
    node->type = type;
    return node;
}

Node *new_node_binop(int ty, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    node->type = deduce_type(ty, lhs, rhs);
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    node->type = calloc(1, sizeof(Type));
    node->type->ty = INT;
    node->type->ptr_of = NULL;
    return node;
}

Node *new_node_string(const Token *tok) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_STRING;

    // Set type as a char array.
    Type *ty_char = calloc(1, sizeof(Type));
    ty_char->ty = CHAR;
    Type *ty_string = calloc(1, sizeof(Type));
    ty_string->ptr_of = ty_char;
    ty_string->array_len = tok->len;
    node->type = ty_string;

    // Store the literal.
    char *buf = malloc(tok->len + 1);
    strncpy(buf, tok->input, tok->len);
    buf[tok->len] = '\0';
    node->name = buf;
    map_put(strings, buf, (void *)strings->keys->len);
    return node;
}

Node *new_node_declaration(const Node* declarator, Type *type, Node *init) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_DECLARATION;
    // Copy identifier name.
    node->name = declarator->name;
    // Set type.
    node->type = type;
    node->declinit = init;
    return node;
}

// If type is NULL, this will look up its type from declarations.
Node *new_node_ident(const Token *tok, Type *type) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_IDENT;
    // Copy identifier name.
    size_t len = tok->len;
    node->name = malloc(len + 1);
    strncpy(node->name, tok->input, len);
    node->name[len] = '\0';

    // If type is not given from caller, look for local and global variables.
    // If this is a function identifier externally defined,
    // we may not know the return type at this time of development.
    Type *t = type;
    if (!t) {
        Node *n = (Node *)map_get(localvars, node->name);
        if (n) t = n->type;
    }
    if (!t) {
        Node *n = (Node *)map_get(globalvars, node->name);
        if (n) t = n->type;
    }
    node->type = t;
    return node;
}

Node *new_funcdef(const Token *tok) {
    Node *func = calloc(1, sizeof(Node));
    func->ty = ND_FUNCDEF;
    size_t len = tok->len;
    func->fname = malloc(len);
    strncpy(func->fname, tok->input, len);
    func->fname[len] = '\0';
    func->fargs = new_vector();
    return func;
}

// A helper function to create and stroe a token.
static void push_token(int ty, char *input, int val, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->ty = ty;
    tok->input = input;
    tok->val = val;
    tok->len = len;
    vec_push(tokens, (void *)tok);
}

// A helper function to retrieve a token at a given position.
static Token *get_token(int i) {
    return (Token *)tokens->data[i];
}

void tokenize(char *p) {
    tokens = new_vector();
    while (*p) {
        // Skip white spaces.
        if (isspace(*p)) {
            ++p;
            continue;
        }

        if (isdigit(*p)) {
            const char *p0 = p;
            push_token(TK_NUM, p, strtol(p, &p, 10), p - p0);
            continue;
        }
        
        // String literals.
        if (*p == '"') {
            ++p;
            char *p0 = p;
            while (*p != '"')
                ++p;
            int len = p - p0;
            push_token(TK_STRING_LITERAL, p0, 0, len);
            ++p;
            continue;
        }

        // Identifiers or keywords.
        if (isalpha(*p) || *p == '_') {
            char *p0 = p;
            do
                ++p;
            while (isalpha(*p) || isdigit(*p) || *p == '_');

            int len = p - p0;
            if (strncmp(p0, "int", max(len, 3)) == 0) {
                push_token(TK_TYPE_INT, p0, 0, len);
                continue;
            }
            if (strncmp(p0, "char", max(len, 4)) == 0) {
                push_token(TK_TYPE_CHAR, p0, 0, len);
                continue;
            }
            if (strncmp(p0, "struct", max(len, 6)) == 0) {
                push_token(TK_STRUCT, p0, 0, len);
                continue;
            }
            if (strncmp(p0, "return", max(len, 6)) == 0) {
                push_token(TK_RETURN, p0, 0, len);
                continue;
            }
            if (strncmp(p0, "if", max(len, 2)) == 0) {
                push_token(TK_IF, p0, 0, len);
                continue;
            }
            if (strncmp(p0, "else", max(len, 4)) == 0) {
                push_token(TK_ELSE, p0, 0, len);
                continue;
            }
            if (strncmp(p0, "while", max(len, 5)) == 0) {
                push_token(TK_WHILE, p0, 0, len);
                continue;
            }
            if  (strncmp(p0, "for", max(len, 3)) == 0) {
                push_token(TK_FOR, p0, 0, len);
                continue;
            }
            push_token(TK_IDENT, p0, 0, len);
            continue;
        }

        // Equality and nonequality operators.
        if (strncmp(p, "==", 2) == 0) {
            push_token(TK_EQUAL, p, 0, 2);
            p += 2;
            continue;
        }
        if (strncmp(p, "!=", 2) == 0) {
            push_token(TK_NOTEQUAL, p, 0, 2);
            p += 2;
            continue;
        }
        
        // Two-letter relational operators (<= and >=).
        if (strncmp(p, "<=", 2) == 0) {
            push_token(TK_LESSEQUAL, p, 0, 2);
            p += 2;
            continue;
        }
        if (strncmp(p, ">=", 2) == 0) {
            push_token(TK_GREATEREQUAL, p, 0, 2);
            p += 2;
            continue;
        }
        if (strncmp(p, "++", 2) == 0) {
            push_token(TK_INCREMENT, p, 0, 2);
            p += 2;
            continue;
        }
        if (strncmp(p, "--", 2) == 0) {
            push_token(TK_DECREMENT, p, 0, 2);
            p += 2;
            continue;
        }

        // One-letter tokens.
        switch (*p) {
        case '&':
        case '(':
        case ')':
        case '*':
        case '+':
        case '.':
        case ',':
        case '-':
        case '/':
        case ';':
        case '<':
        case '=':
        case '>':
        case '[':
        case ']':
        case '{':
        case '}':
            push_token(*p, p, 0, 1);
            ++p;
            continue;
        }

        fprintf(stderr, "Cannot tokenize %s.\n", p);
        exit(1);
    }

    push_token(TK_EOF, p, 0, 0);
}

// =============================================================================
// Parse tokens into abstract syntax trees.
// =============================================================================
// A buffer to store parsed statements (ASTs).
Vector *funcdefs;

// We will store string literals and their indices as we parse.
Map *strings = NULL;

// Private utility functions.
static bool consume(int ty) {
    if (get_token(pos)->ty == ty) {
        ++pos;
        return true;
    } else {
        return false;
    }
}

static void expect(int ty) {
    if (get_token(pos)->ty == ty) {
        ++pos;
        return;
    }

    if (isprint(ty))
        fprintf(stderr, "A token of type %c expected but was %c.\n", ty, get_token(pos)->ty);
    else
        fprintf(stderr, "A token of type %d expected but was %d.\n", ty, get_token(pos)->ty);
    exit(1);
}

// A function to report parsing errors.
static void error(const char *msg, size_t i) {
    fprintf(stderr, "%s \"%s\"\n", msg, get_token(i)->input);
    exit(1);
}

// Parse an expression to an abstract syntax tree.
// program: {funcdef}* | declaration
// funcdef: ident "(" parameter-list ")" compound
// compound: "{" {declaration}* {statement}* "}"
// declaration: "int" {"*"}* declarator
// init_declarator: declarator | declarator "=" assign
// declarator: ident | declarator "[" num "]"
// statement: assign ";" | selection | iteration | "return" ";" | "return" assign ";"
// assign: equal assign'
// assign': '' | "=" assign'
// selection: "if" "(" assign ")" statement | "if" "(" assign ")" statement "else" statement
// iteration: "while" "(" assign ")" statement
// iteration: "for" "(" assign ";" assign ";" assign ")" statement
// equal: relational equal'
// equal': '' | "==" equal | "!=" equal
// relational: add relational'
// relational': '' | "<" relational | ">" relational | "<=" relational | ">=" relational
// add: mul add'
// add': '' | "+" add' | "-" add'
// mul: unary | unary "*" mul | unary "/" mul
// unary: postfix | '++' unary | '--' unary | '*' unary | '&' unary
// postfix: term | postfix "(" {assign}* ")" | postfix "[" assign "]" | postfix "." ident
// term: num | "(" assign ")"

static Type *decl_specifier() {
    Token *tok = get_token(pos++);
    if (tok->ty != TK_TYPE_INT
            && tok->ty != TK_TYPE_CHAR
            && tok->ty != TK_STRUCT) {
        fprintf(stderr, "A type specifier of int, char, or struct was expected but got token %d.\n", tok->ty);
        exit(1);
    }
    Type *type = calloc(1, sizeof(Type));
    switch (tok->ty) {
    case TK_TYPE_INT:
        type->ty = INT;
        break;
    case TK_TYPE_CHAR:
        type->ty = CHAR;
        break;
    case TK_STRUCT:
    {
        type->ty = STRUCT;
        expect('{');
        Map *members = new_map();
        while (!consume('}')) {
            struct_declaration(members);
        }
        type->members = members;
        break;
    }
    }
    type->ptr_of = NULL;
    return type;
}

void program(void) {
    funcdefs = new_vector();
    globalvars = new_map();
    strings = new_map();
    pos = 0;
    while (get_token(pos)->ty != TK_EOF) {
        Node *funcdef_or_globalvar = extern_declaration();
        if (funcdef_or_globalvar->ty == ND_FUNCDEF)
            vec_push(funcdefs, (void *)funcdef_or_globalvar);
    }
}

Node *extern_declaration() {
    // In order to distinguish funcdefs and globals, we need to parse until an
    // identifier and one token past ('(' then funcdef, global otherwise).
    // We simply parse past the identifier, discard parse results, reset the
    // token position, and simply parse again.
    size_t pos0 = pos;

    // Parse till identifier and discard.
    decl_specifier();
    while(consume('*'))
        ;   // NOP.
    if (get_token(pos)->ty != TK_IDENT)
        error("A function definition expected but not found.\n", pos);
    ++pos;
    Token *tok_after_ident = get_token(pos);

    // Reset token position and parse again.
    pos = pos0;
    if (tok_after_ident->ty == '(')
        return funcdef();
    else
        return declaration(globalvars);
}

static Node *parse_func_param() {
    if (get_token(pos)->ty != TK_TYPE_INT)
        error("Missing type specifier for a function parameter.\n", pos);
    Type *type = decl_specifier();
    Node *node = new_node_ident(get_token(pos++), type);
    map_put(localvars, node->name, node);
    return node;
}

Node *funcdef(void) {
    if (!consume(TK_TYPE_INT))
        error("Missing return type of a function definition.\n", pos);

    Token *tok = get_token(pos);
    if (tok->ty != TK_IDENT)
        error("A function definition expected but not found.\n", pos);
    ++pos;

    // Prepare a new set of local variables.
    localvars = new_map();
    Node *func = new_funcdef(tok);

    if (!consume('('))
        error("'(' expected but not found.\n", pos);
    if (!consume(')')) {
        vec_push(func->fargs, parse_func_param());
        while (consume(',')) {
            vec_push(func->fargs, parse_func_param());
        }
        expect(')');
    }

    func->fbody = compound();
    return func;
}

Node *declaration(Map *variables) {
    // Read type before identifier, e.g., "int **".
    Type *type = decl_specifier();
    // Declarator after identifier may alter the type.
    Node *node = init_declarator(type);
    map_put(variables, node->name, node);
    expect(';');
    return node;
}

Node *struct_declaration(Map *members) {
    // Read type before identifier, e.g., "int **".
    Type *type = decl_specifier();
    // Declarator after identifier may alter the type.
    Node *node = declarator(type);
    map_put(members, node->name, node);
    expect(';');
    return node;
}

Node *init_declarator(Type *type) {
    Node *decl = declarator(type);
    Node *init = NULL;
    if (consume('=')) {
        init = assign();
    }
    Node *node = new_node_declaration(decl, decl->type, init);
    return node;
}

Node *declarator(Type *type) {
    // If '*'s are found, make a pointer of a type.
    while (consume('*')) {
        Type *inner = type;
        type = calloc(1, sizeof(Type));
        type->ty = PTR;
        type->ptr_of = inner;
    }

    if (type->ty == ARRAY)
        error("Recursive declarator. Not implemented yet.\n", pos);
    if (get_token(pos)->ty != TK_IDENT)
        error("An identifier is expected but not found.\n", pos);

    Node *node = new_node_ident(get_token(pos++), type);

    if (consume('[')) {
        // Array declaration. Parse and set an array type.
        Token *tok = get_token(pos++);
        if (tok->ty != TK_NUM)
            error("Array length must be specified with an integer literal.\n", pos);
        expect(']');

        Type *artype = calloc(1, sizeof(Type));
        artype->ty = ARRAY;
        artype->ptr_of = type;
        artype->array_len = tok->val;

        node->type = artype;
    }

    return node;
}

Node *compound(void) {
    if (!consume('{'))
        error("'{' expected but not found.\n", pos);
    
    Vector *code = new_vector();
    Token *tok = get_token(pos);
    while (tok->ty != TK_EOF && tok->ty != '}') {
        Node *decl_or_stmt = NULL;
        if (tok->ty == TK_TYPE_INT
                || tok->ty == TK_TYPE_CHAR
                || tok->ty == TK_STRUCT)
            decl_or_stmt = declaration(localvars);
        else
            decl_or_stmt = statement();
        vec_push(code, (void *)decl_or_stmt);
        tok = get_token(pos);
    }
    if (!consume('}'))
        error("A compound statement not terminated with '}'.", pos);

    Node *comp_stmt = new_node(ND_COMPOUND);
    comp_stmt->stmts = code;
    comp_stmt->localvars = localvars;
    return comp_stmt;
}

Node *statement(void) {
    Token *tok = get_token(pos);
    Node *node = NULL;
    switch(tok->ty) {
    case ';':
        // Empty statement.
        node = new_node(ND_BLANK);
        break;
    case '{':
        // Compound statement.
        node = compound();
        return node;
    case TK_IF:
        ++pos;
        node = selection();
        return node;
    case TK_WHILE:
        ++pos;
        node = iteration_while();
        return node;
    case TK_FOR:
        ++pos;
        node = iteration_for();
        return node;

    case TK_RETURN:
        ++pos;
        Node *rhs = NULL;
        if (get_token(pos)->ty == ';')
            rhs = new_node_num(0);
        else
            rhs = assign();
        node = new_node(ND_RETURN);
        node->rhs = rhs;
        break;
    default:
        node = assign();
        break;
    }
    if (!consume(';'))
        error("A statement not terminated with ';'.", pos);
    return node;
}

Node *assign(void) {
    Node *lhs = equal();
    if (consume('='))
        return new_node_binop('=', lhs, assign());
    return lhs;
}

Node *selection(void) {
    Node *node = new_node(ND_IF);
    expect('(');
    node->cond = assign();
    expect(')');
    node->then = statement();
    if (consume(TK_ELSE))
        node->els = statement();

    return node;
}

Node *iteration_while(void) {
    Node *node = new_node(ND_WHILE);
    expect('(');
    node->itercond = assign();
    expect(')');
    node->iterbody = statement();
    return node;
}

Node *iteration_for(void) {
    Node *node = new_node(ND_FOR);
    expect('(');
    if (get_token(pos)->ty != ';')
        node->iterinit = assign();
    else
        node->iterinit = new_node(ND_BLANK);
    expect(';');
    if (get_token(pos)->ty != ';')
        node->itercond = assign();
    else
        node->itercond = new_node(ND_BLANK);
    expect(';');
    if (get_token(pos)->ty != ')')
        node->step = assign();
    else
        node->step = new_node(ND_BLANK);
    expect(')');
    node->iterbody = statement();
    return node;
}

Node *equal(void) {
    Node *lhs = relational();
    if (consume(TK_EQUAL))
        return new_node_binop(ND_EQUAL, lhs, equal());
    if (consume(TK_NOTEQUAL))
        return new_node_binop(ND_NOTEQUAL, lhs, equal());
    return lhs;
}

Node *relational(void) {
    Node *lhs = add();
    if (consume('<'))
        return new_node_binop('<', lhs, relational());
    if (consume('>'))
        return new_node_binop('>', lhs, relational());
    if (consume(TK_LESSEQUAL))
        return new_node_binop(ND_LESSEQUAL, lhs, relational());
    if (consume(TK_GREATEREQUAL))
        return new_node_binop(ND_GREATEREQUAL, lhs, relational());
    return lhs;
}

Node *add(void) {
    Node *lhs = mul();
    if (consume('+')) {
        return new_node_binop('+', lhs, add());
    }
    if (consume('-')) {
        return new_node_binop('-', lhs, add());
    }

    return lhs;
}

// Parse a multiplicative expression.
Node *mul(void) {
    Node *lhs = unary();
    switch(get_token(pos)->ty) {
    case '*':
        ++pos;
        return new_node_binop('*', lhs, mul());
    case '/':
        ++pos;
        return new_node_binop('/', lhs, mul());
    default:
        return lhs;
    }
}

Node *unary(void) {
    Token *tok = get_token(pos);
    switch(tok->ty) {
    case TK_INCREMENT:
    case TK_DECREMENT:
    case '*':
    case '&':
        ++pos;
        Node *operand = unary();
        return new_node_uop(tok->ty, operand);
    default:
        return postfix();
    }
}

Node *postfix(void) {
    Node *node = term();
    Token *tok = get_token(pos);

    switch (tok->ty) {
    case '(':
        // Function call.
        ++pos;
        node->ty = ND_CALL;
        node->fargs = new_vector();

        // Set return type.
        // For now, we assume all functions return an int.
        node->type = calloc(1, sizeof(Type));
        node->type->ty = INT;
        node->type->ptr_of = NULL;

        // Nullary function call.
        if (consume(')'))
            return node;

        // List arguments.
        vec_push(node->fargs, assign());
        while (consume(','))
            vec_push(node->fargs, assign());
        if (!consume(')'))
            error("No closing parenthesis ')' for function call.", pos);
        break;

    case '[':
    {
        // Array accessor. Convert "ar[i]" as "*(ar+i)".
        ++pos;
        Node *lhs = node;
        Node *rhs = assign();
        Node *binop = new_node_binop('+', lhs, rhs);
        node = new_node_uop('*', binop);
        if (!consume(']'))
            error("No closing bracket for array index.", pos);
        break;
    }

    case '.':
    {
        // Struct member access.
        // Copy member name and type.
        ++pos;
        Token *tok = get_token(pos++);

        Node *member_of = node;
        node = new_node(ND_MEMBER);
        node->member_of = member_of;
        char *mname = malloc(tok->len + 1);
        strncpy(mname, tok->input, tok->len);
        mname[tok->len] = '\0';
        node->mname = mname;
        assert(member_of->type);
        assert(member_of->type->members);
        node->type = ((Node *)map_get(member_of->type->members, mname))->type;
        break;
    }
    }

    return node;
}

// Parse a term (number or expression in pair of parentheses).
Node *term(void) {
    if (get_token(pos)->ty == TK_NUM)
        return new_node_num(get_token(pos++)->val);
    if (get_token(pos)->ty == TK_IDENT)
        return new_node_ident(get_token(pos++), NULL);
    if (get_token(pos)->ty == TK_STRING_LITERAL)
        return new_node_string(get_token(pos++));

    if (!consume('('))
        error("A token neither a number nor an opening parenthesis.", pos);
    Node *node = assign();
    if (!consume(')'))
        error("A closing parenthesis was expected but not found.", pos);
    return node;
}

