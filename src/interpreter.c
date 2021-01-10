#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter.h"
#include "builtins.h"

#define NOTIMPLEMENTED() (puts("Operation not implemented."), exit(-1))
#define TYPEERROR() (puts("Attempt to perform an operation on incompatible types"), exit(-1))

typedef Object (*InterpretFn)(const Node* root);

static Object interpretVariable(const Node* root) {
        NOTIMPLEMENTED();
}
static Object interpretInt(const Node* root) {
        return (Object) {.type=TYPE_INT, .intval=atoll(root->token.source)};
}
static Object interpretBool(const Node* root) {
        NOTIMPLEMENTED();

}
static Object interpretFloat(const Node* root) {
        return (Object) {.type=TYPE_FLOAT, .floatval=atof(root->token.source)};
}
static Object interpretStr(const Node* root) {
        return (Object) {.type=TYPE_CONTAINER, .payload=makeString(root->token.source+1, root->token.length-2)};
}
static Object interpretUnaryPlus(const Node* root) {
        Object operand = interpret(root->operands[0]);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_FLOAT:
                        return operand;
                case TYPE_CONTAINER:
                        TYPEERROR();
                default: TYPEERROR();
        }
}
static Object interpretUnaryMinus(const Node* root) {
        Object operand = interpret(root->operands[0]);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        operand.intval *= -1;
                        return operand;
                case TYPE_FLOAT:
                        operand.floatval *= -1;
                        return operand;
                case TYPE_CONTAINER:
                        TYPEERROR();
                default: TYPEERROR();
        }
}
static Object interpretGroup(const Node* root) {
        return interpret(root->operands[0]);
}
static Object interpretSum(const Node* root) {
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
                                case TYPE_CONTAINER:
                                        TYPEERROR();
                                default: TYPEERROR();
                        }
                case TYPE_CONTAINER:
                        switch (opA.payload->type) {
                                case CONTENT_STRING:
                                        switch (opB.type) {
                                                case TYPE_INT:
                                                case TYPE_BOOL:
                                                case TYPE_FLOAT:
                                                        TYPEERROR();
                                                case TYPE_CONTAINER:
                                                        switch (opB.payload->type) {
                                                                case CONTENT_STRING:
                                                                        return (Object) {.type=TYPE_CONTAINER, .payload=concatenateStrings(opA.payload, opB.payload)};
                                                                default: TYPEERROR();
                                                        }
                                                default: TYPEERROR();
                                        }
                                default: TYPEERROR();
                        }

                case TYPE_FLOAT:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval+opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval+opB.floatval)};
                                case TYPE_CONTAINER:
                                        TYPEERROR();
                                default: TYPEERROR();
                        }
                default: TYPEERROR();
        }

}
static Object interpretDifference(const Node* root) {
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
                                case TYPE_CONTAINER:
                                        TYPEERROR();
                                default: TYPEERROR();
                        }
                case TYPE_CONTAINER:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                case TYPE_FLOAT:
                                case TYPE_CONTAINER:
                                        TYPEERROR();
                                default: TYPEERROR();
                        }
                case TYPE_FLOAT:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval-opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval-opB.floatval)};
                                case TYPE_CONTAINER:
                                        TYPEERROR();
                                default: TYPEERROR();
                        }
                default: TYPEERROR();
        }

}
static Object interpretProduct(const Node* root) {
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
                                case TYPE_CONTAINER:
                                        switch (opB.payload->type) {
                                                case CONTENT_STRING:
                                                        if (opA.intval < 0) TYPEERROR();
                                                        return (Object) {.type=TYPE_CONTAINER, .payload=multiplyString(opB.payload, opA.intval)};
                                                default: TYPEERROR();
                                        }
                                default: TYPEERROR();

                        }
                case TYPE_CONTAINER:
                        switch (opA.payload->type) {
                                case CONTENT_STRING:
                                        switch (opB.type) {
                                                case TYPE_INT:
                                                case TYPE_BOOL:
                                                        if (opB.intval < 0) TYPEERROR();
                                                        return (Object) {.type=TYPE_CONTAINER, .payload=multiplyString(opA.payload, opB.intval)};
                                                case TYPE_FLOAT:
                                                case TYPE_CONTAINER:
                                                        TYPEERROR();
                                                default: TYPEERROR();
                                        }
                                default: TYPEERROR();
                        }
                case TYPE_FLOAT:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval*opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval*opB.floatval)};
                                case TYPE_CONTAINER:
                                        TYPEERROR();
                                default: TYPEERROR();
                        }
                default: TYPEERROR();
        }

}
static Object interpretDivision(const Node* root) {
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
                                case TYPE_CONTAINER:
                                        TYPEERROR();
                                default: TYPEERROR();
                        }
                case TYPE_CONTAINER:
                        switch (opA.payload->type) {
                                case CONTENT_STRING:
                                        switch (opB.type) {
                                                case TYPE_INT:
                                                case TYPE_BOOL:
                                                case TYPE_FLOAT:
                                                case TYPE_CONTAINER:
                                                        TYPEERROR();
                                                default: TYPEERROR();
                                        }
                                default: TYPEERROR();
                        }
                case TYPE_FLOAT:
                        switch (opB.type) {
                                case TYPE_INT:
                                case TYPE_BOOL:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval/opB.intval)};
                                case TYPE_FLOAT:
                                        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval/opB.floatval)};
                                case TYPE_CONTAINER:
                                        TYPEERROR();
                                default: TYPEERROR();
                        }
                default: TYPEERROR();
        }

}
static Object interpretAffect(const Node* root) {
        NOTIMPLEMENTED();
}
static Object interpretSemicolon(const Node* root) {
        interpret(root->operands[0]);
        return interpret(root->operands[1]);
}

Object interpret(const Node* root) {
        static const InterpretFn interpreters[] = {
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
        return interpreters[root->operator](root);
}

#undef TYPEERROR
#undef NOTIMPLEMENTED
