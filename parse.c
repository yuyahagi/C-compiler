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

Type *deduce_type(Node *lhs, Node *rhs) {
    if (lhs)
        return lhs->type;
    return NULL;
}

Node *new_node_uop(int operator, Node *operand) {
    assert(operator == '*' || operator == '&');
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_UEXPR;
    node->uop = operator;
    node->operand = operand;
    // Deduce type.
    Type *type = NULL;
    switch (operator) {
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
    node->type = deduce_type(lhs, rhs);
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

Node *new_node_declaration(const Node* declarator, Type *type) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_DECLARATION;
    // Copy identifier name.
    node->name = declarator->name;
    // Set type.
    node->type = type;
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
    if (!t) t = (Type *)map_get(localvars, node->name);
    if (!t) t = (Type *)map_get(globalvars, node->name);
    node->type = t;
    return node;
}

Node *new_funcdef(const Token *tok) {
    Node *func = calloc(1, sizeof(Node));
    func->ty = ND_FUNCDEF;
    size_t len = tok->len;
    func->name = malloc(len);
    strncpy(func->name, tok->input, len);
    func->name[len] = '\0';
    func->args = new_vector();
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

// Split input string and store to a buffer.
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
        if (*p == '=' && *(p+1) == '=') {
            push_token(TK_EQUAL, p, 0, 2);
            p += 2;
            continue;
        }
        if (*p == '!' && *(p+1) == '=') {
            push_token(TK_NOTEQUAL, p, 0, 2);
            p += 2;
            continue;
        }
        
        // Two-letter relational operators (<= and >=).
        if (*p == '<' && *(p+1) == '=') {
            push_token(TK_LESSEQUAL, p, 0, 2);
            p += 2;
            continue;
        }
        if (*p == '>' && *(p+1) == '=') {
            push_token(TK_GREATEREQUAL, p, 0, 2);
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
// declaration: "int" {"*"}* direct_declarator
// direct_declarator: ident | direct_declarator "[" num "]"
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
// unary: postfix | '*' unary | '&' unary
// postfix: term | postfix "(" {assign}* ")" | postfix "[" assign "]"
// term: num | "(" assign ")"
static Type *read_type(Type *inner) {
    if (!inner) {
        expect(TK_TYPE_INT);
        inner = calloc(1, sizeof(Type));
        inner->ty = INT;
        inner->ptr_of = NULL;
    }

    // If '*' follows, make a pointer of a type that has been parsed so far.
    if (get_token(pos)->ty == '*') {
        ++pos;
        Type *inner_ = inner;
        inner = calloc(1, sizeof(Type));
        inner->ty = PTR;
        inner->ptr_of = inner_;
        return read_type(inner);
    }
    return inner;
}

void program(void) {
    funcdefs = new_vector();
    globalvars = new_map();
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
    Type *type = read_type(NULL);
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
    Type *type = read_type(NULL);
    Node *node = new_node_ident(get_token(pos++), type);
    map_put(localvars, node->name, type);
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
        vec_push(func->args, parse_func_param());
        while (consume(',')) {
            vec_push(func->args, parse_func_param());
        }
        expect(')');
    }

    func->body = compound();
    return func;
}

Node *declaration(Map *variables) {
    // Read type before identifier, e.g., "int **".
    Type *type = read_type(NULL);
    // Declarator after identifier may alter the type.
    Node *declarator = direct_declarator(type);
    Node *node = new_node_declaration(declarator, declarator->type);
    map_put(variables, node->name, declarator->type);
    expect(';');
    return node;
}

Node *direct_declarator(Type *type) {
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
        if (tok->ty == TK_TYPE_INT)
            decl_or_stmt = declaration(localvars);
        else
            decl_or_stmt = statement();
        vec_push(code, (void *)decl_or_stmt);
        tok = get_token(pos);
    }
    if (!consume('}'))
        error("A compound statement not terminated with '}'.", pos);

    Node *comp_stmt = new_node_binop(ND_COMPOUND, NULL, NULL);
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
        node = new_node_binop(ND_BLANK, NULL, NULL);
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
        node = new_node_binop(ND_RETURN, NULL, rhs);
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
    Node *node = new_node_binop(ND_IF, NULL, NULL);
    expect('(');
    node->cond = assign();
    expect(')');
    node->then = statement();
    if (consume(TK_ELSE))
        node->els = statement();

    return node;
}

Node *iteration_while(void) {
    Node *node = new_node_binop(ND_WHILE, NULL, NULL);
    expect('(');
    node->cond = assign();
    expect(')');
    node->then = statement();
    return node;
}

Node *iteration_for(void) {
    Node *node = new_node_binop(ND_FOR, NULL, NULL);
    expect('(');
    if (get_token(pos)->ty != ';')
        node->init = assign();
    else
        node->init = new_node_binop(ND_BLANK, NULL, NULL);
    expect(';');
    if (get_token(pos)->ty != ';')
        node->cond = assign();
    else
        node->cond = new_node_binop(ND_BLANK, NULL, NULL);
    expect(';');
    if (get_token(pos)->ty != ')')
        node->step = assign();
    else
        node->step = new_node_binop(ND_BLANK, NULL, NULL);
    expect(')');
    node->then = statement();
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
    if (tok->ty == '*' || tok->ty == '&') {
        ++pos;
        Node *operand = unary();
        return new_node_uop(tok->ty, operand);
    } else {
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
        node->args = new_vector();

        // Set return type.
        // For now, we assume all functions return an int.
        node->type = calloc(1, sizeof(Type));
        node->type->ty = INT;
        node->type->ptr_of = NULL;

        // Nullary function call.
        if (consume(')'))
            return node;

        // List arguments.
        vec_push(node->args, assign());
        while (consume(','))
            vec_push(node->args, assign());
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
    }

    return node;
}

// Parse a term (number or expression in pair of parentheses).
Node *term(void) {
    if (get_token(pos)->ty == TK_NUM)
        return new_node_num(get_token(pos++)->val);
    if (get_token(pos)->ty == TK_IDENT)
        return new_node_ident(get_token(pos++), NULL);

    if (!consume('('))
        error("A token neither a number nor an opening parenthesis.", pos);
    Node *node = assign();
    if (!consume(')'))
        error("A closing parenthesis was expected but not found.", pos);
    return node;
}

