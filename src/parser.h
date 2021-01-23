#ifndef parser_h
#define parser_h

#include "token.h"

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
        OP_INVERT,

        // two operands
        OP_SUM,
        OP_DIFFERENCE,
        OP_PRODUCT,
        OP_DIVISION,
        OP_AFFECT,
        OP_AND,
        OP_OR,
        OP_EQ,
        OP_LT,
        OP_LE,
} Operator;

typedef struct Node {
        struct Node* operands[2];
        Token token;
        Operator operator;
} Node;


void freeNode(Node* node);

// tokens[] is supposed to end with a TOKEN_EOF
Node* parse_statement(Token **const tokens);

#endif
