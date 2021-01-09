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
        PREC_UNARY,
} Precedence;

typedef enum {
        // no operand
        OP_VARIABLE,
        OP_INT,
        OP_BOOL,
        OP_FLOAT,
        OP_STR,

        // one operand
        OP_UNARY_PLUS,
        OP_UNARY_MINUS,
        OP_GROUP,

        // two operands
        OP_SUM,
        OP_DIFFERENCE,
        OP_PRODUCT,
        OP_DIVISION,
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
