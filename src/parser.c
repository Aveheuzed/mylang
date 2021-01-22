#include <stdlib.h>
#include <stdio.h>

#include "parser.h"

#define CONSUME(tkptrptr) (*(*tkptrptr)++)

typedef Node* (*UnaryParseFn)(Token **const tokens);
typedef Node* (*BinaryParseFn)(Token **const tokens, Node* const root);

static Node* allocateNode(Node *const root, Node *const operand, Token token, Operator operator) {
        Node *const node = malloc(sizeof(Node));
        *node = (Node){
                .operands = {root, operand},
                .token = token,
                .operator = operator,
        };
        return node;
}
void freeNode(Node* node) {
        if (node == NULL) return;
        freeNode(node->operands[0]);
        freeNode(node->operands[1]);
        free(node);
}

// --------------------- prefix parse functions --------------------------------

static Node* prefixParseError(Token **const tokens) {
        const Token tk = **tokens;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        return allocateNode(NULL, NULL, tk, OP_PARSE_ERROR);
}
static Node* unary_plus(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        return allocateNode(parse(tokens, PREC_UNARY), NULL, operator, OP_UNARY_PLUS);
}
static Node* unary_minus(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        return allocateNode(parse(tokens, PREC_UNARY), NULL, operator, OP_UNARY_MINUS);
}
static Node* integer(Token **const tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_INT);
}
static Node* boolean(Token **const tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_BOOL);
}
static Node* fpval(Token **const tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_FLOAT);
}
static Node* string(Token **const tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_STR);
}
static Node* grouping(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* node = allocateNode(parse(tokens, PREC_GROUPING), NULL, operator, OP_GROUP);
        if ((*tokens)->type == TOKEN_PCLOSE) CONSUME(tokens);
        else prefixParseError(tokens);
        return node;
}
static Node* semicolon(Token **const tokens) {
        CONSUME(tokens);
        return NULL;
}

// --------------------- infix parse functions ---------------------------------

static Node* infixParseError(Token **const tokens, Node *const root) {
        const Token tk = **tokens;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        return allocateNode(root, NULL, tk, OP_PARSE_ERROR);
}
static Node* binary_plus(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        return allocateNode(root, parse(tokens, PREC_ADD), operator, OP_SUM);
}
static Node* binary_minus(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        return allocateNode(root, parse(tokens, PREC_ADD), operator, OP_DIFFERENCE);
}
static Node* binary_star(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        return allocateNode(root, parse(tokens, PREC_MUL), operator, OP_PRODUCT);
}
static Node* binary_slash(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        return allocateNode(root, parse(tokens, PREC_MUL), operator, OP_DIVISION);
}

// ------------------ end parse functions --------------------------------------

Node* parse(Token **const tokens, const Precedence precedence) {
        static const struct {UnaryParseFn prefix; BinaryParseFn infix; Precedence precedence;} rules[] = {
                [TOKEN_PLUS] = {unary_plus, binary_plus, PREC_ADD},
                [TOKEN_MINUS] = {unary_minus, binary_minus, PREC_ADD},
                [TOKEN_STAR] = {prefixParseError, binary_star, PREC_MUL},
                [TOKEN_SLASH] = {prefixParseError, binary_slash, PREC_MUL},
                [TOKEN_POPEN] = {grouping, infixParseError, PREC_NONE},
                [TOKEN_PCLOSE] = {prefixParseError, infixParseError, PREC_NONE},

                [TOKEN_EQUAL] = {prefixParseError, infixParseError, PREC_AFFECT},
                [TOKEN_SEMICOLON] = {semicolon, infixParseError, PREC_NONE},

                [TOKEN_IDENTIFIER] = {prefixParseError, infixParseError, PREC_NONE},
                [TOKEN_INT] = {integer, infixParseError, PREC_NONE},
                [TOKEN_FLOAT] = {fpval, infixParseError, PREC_NONE},
                [TOKEN_STR] = {string, infixParseError, PREC_NONE},

                [TOKEN_ERROR] = {prefixParseError, infixParseError, PREC_NONE},
                [TOKEN_EOF] = {prefixParseError, infixParseError, PREC_NONE},
        };

        Node* root;

        root = rules[(*tokens)->type].prefix(tokens);
        if (root->operator == OP_PARSE_ERROR) return root;

        while (precedence <= rules[(*tokens)->type].precedence) {
                root = rules[(*tokens)->type].infix(tokens, root);
                if (root->operator == OP_PARSE_ERROR) break;
                if ((*tokens)->type == TOKEN_SEMICOLON) {
                        CONSUME(tokens);
                        break;
                }
        }

        return root;
}

#undef CONSUME
