#ifndef node_h
#define node_h

#include "headers/pipeline/token.h"

typedef enum {
        // no operand
        OP_VARIABLE,
        OP_INT,
        OP_BOOL,
        OP_FLOAT,
        OP_STR,
        OP_NONE,

        // one operand
        OP_UNARY_PLUS,
        OP_UNARY_MINUS,
        OP_INVERT,

        // two operands
        OP_SUM,
        OP_DIFFERENCE,
        OP_PRODUCT,
        OP_DIVISION,
        OP_AFFECT,
        OP_IADD,
        OP_ISUB,
        OP_IMUL,
        OP_IDIV,
        OP_AND,
        OP_OR,
        OP_EQ,
        OP_LT,
        OP_LE,

        OP_CALL,

        OP_BLOCK,
        OP_IFELSE,
        OP_WHILE,

        OP_NOP,

        LEN_OPERATORS // do NOT add anything below this line!
} Operator;

typedef struct Node {
        LocalizedToken token;
        Operator operator;
        /*note about the f.a.m.:
        For nodes that don't have a fixed number of children, the first element of this array MUST be cast to an uintptr_t that will indicate the number of children this array contains. In that case, the first child (if present) will be found at index 1.
        */
        struct Node* operands[];
} Node;

void freeNode(Node* node);

#endif
