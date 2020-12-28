#ifndef parser_h
#define parser_h

#include "token.h"

typedef enum Precedence {
        PREC_NONE,
        PREC_GROUPING,
        PREC_SEMICOLON,
        PREC_AFFECT,
        PREC_ADD,
        PREC_MUL,
} Precedence;

typedef enum {
        // no operand
        OP_VARIABLE,
        OP_LITERAL,

        // one operand
        OP_MATH_UNARY,
        OP_GROUP,

        // two operands
        OP_MATH_BINARY,
        OP_AFFECT,

        // up to two operands
        OP_SEMICOLON,
} Operator;

typedef struct Node {
        struct Node* operands[2];
        Token token;
        Operator operator;
} Node;

typedef Node* (*UnaryParseFn)(Token ** tokens);
typedef Node* (*BinaryParseFn)(Token ** tokens, Node* const root);

Node* allocateNode(Node *const root, Node *const operand, Token token, Operator operator);
void freeNode(Node* node);

// tokens[] is supposed to end with a TOKEN_EOF
Node* parse(Token ** tokens, const Precedence precedence);

#endif
