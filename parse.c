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
Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

Node *new_node_ident(const Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->ty = ND_IDENT;
    // Copy identifier name.
    size_t len = tok->len;
    node->name = malloc(len + 1);
    strncpy(node->name, tok->input, len);
    node->name[len] = '\0';
    return node;
}

FuncDef *new_funcdef(const Token *tok) {
    FuncDef *func = calloc(1, sizeof(FuncDef));
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
        fprintf(stderr, "A token of type %c expected but was %d.\n", ty, get_token(pos)->ty);
    else
        fprintf(stderr, "A token of type %d expected but was %d.\n", ty, get_token(pos)->ty);
    exit(1);
}

// A function to report parsing errors.
void error(const char *msg, size_t i) {
    fprintf(stderr, "%s \"%s\"\n", msg, get_token(i)->input);
    exit(1);
}

// Parse an expression to an abstract syntax tree.
// program: {funcdef}*
// funcdef: ident "(" parameter-list ")" compound
// compound: "{" {statement}* "}"
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
// mul: postfix | postfix "*" mul | postfix "/" mul
// postfix: term | term "(" ")"
// term: num | "(" add ")"
void program(void) {
    funcdefs = new_vector();
    pos = 0;
    while (get_token(pos)->ty != TK_EOF) {
        vec_push(funcdefs, (void *)funcdef());
    }
}

FuncDef *funcdef(void) {
    Token *tok = get_token(pos);
    if (tok->ty != TK_IDENT)
        error("A function definition expected but not found.\n", pos);
    ++pos;

    FuncDef *func = new_funcdef(tok);

    func->args = new_vector();
    if (!consume('('))
        error("'(' expected but not found.\n", pos);
    if (!consume(')')) {
        vec_push(func->args, new_node_ident(get_token(pos++)));
        while (consume(','))
            vec_push(func->args, new_node_ident(get_token(pos++)));
        expect(')');
    }

    func->body = compound();
    return func;
}

Node *compound(void) {
    if (!consume('{'))
        error("'{' expected but not found.\n", pos);
    
    Node *comp_stmt = new_node(ND_COMPOUND, NULL, NULL);
    Vector *code = new_vector();
    Token *tok = get_token(pos);
    while (tok->ty != TK_EOF && tok->ty != '}') {
        Node *stmt = statement();
        if (stmt)
            vec_push(code, (void *)stmt);
        tok = get_token(pos);
    }
    if (!consume('}'))
        error("A compound statement not terminated with '}'.", pos);

    comp_stmt->stmts = code;
    return comp_stmt;
}

Node *statement(void) {
    Token *tok = get_token(pos);
    Node *node = NULL;
    switch(tok->ty) {
    case ';':
        // Empty statement. Skip.
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
        node = new_node(ND_RETURN, NULL, rhs);
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
        return new_node('=', lhs, assign());
    return lhs;
}

Node *selection(void) {
    Node *node = new_node(ND_IF, NULL, NULL);
    expect('(');
    node->cond = assign();
    expect(')');
    node->then = statement();
    if (consume(TK_ELSE))
        node->els = statement();

    return node;
}

Node *iteration_while(void) {
    Node *node = new_node(ND_WHILE, NULL, NULL);
    expect('(');
    node->cond = assign();
    expect(')');
    node->then = statement();
    return node;
}

Node *iteration_for(void) {
    Node *node = new_node(ND_FOR, NULL, NULL);
    expect('(');
    if (get_token(pos)->ty != ';')
        node->init = assign();
    expect(';');
    if (get_token(pos)->ty != ';')
        node->cond = assign();
    expect(';');
    if (get_token(pos)->ty != ')')
        node->step = assign();
    expect(')');
    node->then = statement();
    return node;
}

Node *equal(void) {
    Node *lhs = relational();
    if (consume(TK_EQUAL))
        return new_node(TK_EQUAL, lhs, equal());
    if (consume(TK_NOTEQUAL))
        return new_node(TK_NOTEQUAL, lhs, equal());
    return lhs;
}

Node *relational(void) {
    Node *lhs = add();
    if (consume('<'))
        return new_node('<', lhs, relational());
    if (consume('>'))
        return new_node('>', lhs, relational());
    if (consume(TK_LESSEQUAL))
        return new_node(ND_LESSEQUAL, lhs, relational());
    if (consume(TK_GREATEREQUAL))
        return new_node(ND_GREATEREQUAL, lhs, relational());
    return lhs;
}

Node *add(void) {
    Node *lhs = mul();
    if (consume('+')) {
        return new_node('+', lhs, add());
    }
    if (consume('-')) {
        return new_node('-', lhs, add());
    }

    return lhs;
}

// Parse a multiplicative expression.
Node *mul(void) {
    Node *lhs = postfix();
    switch(get_token(pos)->ty) {
    case '*':
        ++pos;
        return new_node('*', lhs, mul());
    case '/':
        ++pos;
        return new_node('/', lhs, mul());
    default:
        return lhs;
    }
}

Node *postfix(void) {
    Node *node = term();
    if (!consume('('))
        return node;

    // Function call.
    node->ty = ND_CALL;
    node->args = new_vector();

    // Nullary function call.
    if (consume(')'))
        return node;

    // List arguments.
    vec_push(node->args, assign());
    while (consume(','))
        vec_push(node->args, assign());
    if (!consume(')')) {
        error("No closing parenthesis ')' for function call.", pos);
        exit(1);
    }

    return node;
}

// Parse a term (number or expression in pair of parentheses).
Node *term(void) {
    if (get_token(pos)->ty == TK_NUM)
        return new_node_num(get_token(pos++)->val);
    if (get_token(pos)->ty == TK_IDENT)
        return new_node_ident(get_token(pos++));

    if (!consume('('))
        error("A token neither a number nor an opening parenthesis.", pos);
    Node *node = add();
    if (!consume(')'))
        error("A closing parenthesis was expected but not found.", pos);
    return node;
}

