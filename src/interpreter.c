#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOTIMPLEMENTED() (puts("Operation not implemented."), exit(-1))
#define TYPEERROR() (puts("Attempt to perform an operation on incompatible types"), exit(-1))

Object concatenateStrings(char* strA, char* strB) {
        // WARNING : hidden malloc()
        const size_t len_A = strlen(strA);
        char *const dest = malloc(len_A + strlen(strB) + 1);
        strcpy(dest, strA);
        strcpy(dest+len_A, strB);
        return (Object) {.type=TYPE_STR, .strval=dest};
}
Object multiplyString(char* str, intmax_t amount) {
        // WARNING : hidden malloc()
        if (amount < 0) TYPEERROR();

        const size_t len = strlen(str);
        char *const dest = malloc(len*amount+1);
        for (char* i=dest; amount>0; (amount--, i+=len)) {
                strcpy(i, str);
        }
        return (Object) {.type=TYPE_STR, .strval=dest};
}

Object interpretVariable(const Node* root) {
        NOTIMPLEMENTED();
}
Object interpretInt(const Node* root) {
        return (Object) {.type=TYPE_INT, .intval=atoll(root->token.source)};
}
Object interpretBool(const Node* root) {
        NOTIMPLEMENTED();

}
Object interpretFloat(const Node* root) {
        return (Object) {.type=TYPE_FLOAT, .floatval=atof(root->token.source)};
}
Object interpretStr(const Node* root) {
        // WARNING : hidden malloc()
        return (Object) {.type=TYPE_STR, .strval=strndup(root->token.source+1, root->token.length-2)};
}
Object interpretUnaryPlus(const Node* root) {
        Object operand = interpret(root->operands[0]);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_FLOAT:
                        return operand;
                case TYPE_STR:
                        TYPEERROR();
        }
}
Object interpretUnaryMinus(const Node* root) {
        Object operand = interpret(root->operands[0]);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        operand.intval *= -1;
                        return operand;
                case TYPE_FLOAT:
                        operand.floatval *= -1;
                        return operand;
                case TYPE_STR:
                        TYPEERROR();
        }
}
Object interpretGroup(const Node* root) {
        return interpret(root->operands[0]);
}
Object interpretSum(const Node* root) {
        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        switch (opA.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_INT, .intval=(opA.intval+opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval+opB.floatval)};
                                case TYPE_STR:
                                        TYPEERROR();
                        }
                case TYPE_STR:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                case TYPE_FLOAT:
                                        TYPEERROR();
                                case TYPE_STR:
                                        return concatenateStrings(opA.strval, opB.strval);
                        }
                case TYPE_FLOAT:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval+opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval+opB.floatval)};
                                case TYPE_STR:
                                        TYPEERROR();
                        }
        }

}
Object interpretDifference(const Node* root) {
        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        switch (opA.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_INT, .intval=(opA.intval-opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval-opB.floatval)};
                                case TYPE_STR:
                                        TYPEERROR();
                        }
                case TYPE_STR:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                case TYPE_FLOAT:
                                case TYPE_STR:
                                        TYPEERROR();
                        }
                case TYPE_FLOAT:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval-opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval-opB.floatval)};
                                case TYPE_STR:
                                        TYPEERROR();
                        }
        }

}
Object interpretProduct(const Node* root) {
        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        switch (opA.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_INT, .intval=(opA.intval*opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval*opB.floatval)};
                                case TYPE_STR:
                                        return multiplyString(opB.strval, opA.intval);

                        }
                case TYPE_STR:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return multiplyString(opA.strval, opB.intval);
                                case TYPE_FLOAT:
                                case TYPE_STR:
                                        TYPEERROR();
                        }
                case TYPE_FLOAT:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval*opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval*opB.floatval)};
                                case TYPE_STR:
                                        TYPEERROR();
                        }
        }

}
Object interpretDivision(const Node* root) {
        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        switch (opA.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval/(double)opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval*opB.floatval)};
                                case TYPE_STR:
                                        TYPEERROR();
                        }
                case TYPE_STR:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                case TYPE_FLOAT:
                                case TYPE_STR:
                                        TYPEERROR();
                        }
                case TYPE_FLOAT:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval/opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval/opB.floatval)};
                                case TYPE_STR:
                                        TYPEERROR();
                        }
        }

}
Object interpretAffect(const Node* root) {
        NOTIMPLEMENTED();
}
Object interpretSemicolon(const Node* root) {
        interpret(root->operands[0]);
        return interpret(root->operands[1]);
}

const InterpretFn interpreters[] = {
        [OP_VARIABLE] = interpretVariable,

        [OP_INT] = interpretInt,
        [OP_BOOL] = interpretBool,
        [OP_FLOAT] = interpretFloat,
        [OP_STR] = interpretStr,

        [OP_UNARY_PLUS] = interpretUnaryPlus,
        [OP_UNARY_MINUS] = interpretUnaryMinus,
        [OP_GROUP] = interpretGroup,

        [OP_SUM] = interpretSum,
        [OP_DIFFERENCE] = interpretDifference,
        [OP_PRODUCT] = interpretProduct,
        [OP_DIVISION] = interpretDivision,
        [OP_AFFECT] = interpretAffect,

        [OP_SEMICOLON] = interpretSemicolon,
};

Object interpret(const Node* root) {
        return interpreters[root->operator](root);
}
