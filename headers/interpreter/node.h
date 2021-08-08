#ifndef node_h
#define node_h

#include "interpreter/token.h"
#include "interpreter/object.h"

typedef enum {
        // no operand
        OP_LITERAL_FUNCTION,
        OP_LITERAL_INT,
        OP_LITERAL_FLOAT,
        OP_LITERAL_TRUE,
        OP_LITERAL_FALSE,
        OP_LITERAL_NONE,
        OP_LITERAL_STR,

        LAST_OP_LITERAL=OP_LITERAL_STR, // not actually an operator

        OP_VARIABLE,

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
        OP_RETURN,

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
        For nodes that don't have a fixed number of children, the first element of this array is an uintptr_t that will indicate the number of children this array contains. In that case, the first child (if present) will be found at index 1.
        */
        union {
                struct Node* nd;
                uintptr_t len;
                ObjectCore obj;
        } operands[];
} Node;

void freeNode(Node* node);

#endif
