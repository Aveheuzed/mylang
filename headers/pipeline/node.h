#ifndef node_h
#define node_h

#include <stdint.h>

#include "pipeline/token.h"
#include "utils/runtime_types.h"

typedef enum {
        // no operand
        OP_VARIABLE,
        OP_INT,
        OP_STR,

        OP_DECLARE,

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

        OP_IFELSE,

        OP_BLOCK,

        OP_NOP,

        LEN_OPERATORS // do NOT add anything below this line!
} Operator;

typedef struct Node {
        LocalizedToken token;
        Operator operator;
        RuntimeType type;

        /*note about the f.a.m.:
        For nodes that don't have a fixed number of children, the first element of this array is an uintptr_t that will indicate the number of children this array contains. In that case, the first child (if present) will be found at index 1.
        */
        union {
                struct Node* nd;
                uintptr_t len;
        } operands[];
} Node;

void freeNode(Node* node);

#endif
