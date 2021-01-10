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
        static const void* dispatcher[LEN_OBJTYPES][LEN_OBJTYPES] =  {
                [TYPE_INT] = {
                        [TYPE_INT] = &&add_int_int,
                        [TYPE_BOOL] = &&add_int_int,
                        [TYPE_FLOAT] = &&add_int_float,
                },
                [TYPE_BOOL] = {
                        [TYPE_INT] = &&add_int_int,
                        [TYPE_BOOL] = &&add_int_int,
                        [TYPE_FLOAT] = &&add_int_float,
                },
                [TYPE_FLOAT] = {
                        [TYPE_INT] = &&add_float_int,
                        [TYPE_BOOL] = &&add_float_int,
                        [TYPE_FLOAT] = &&add_float_float,
                },
                [TYPE_CONTAINER] = {
                        [TYPE_CONTAINER] = &&add_container_container,
                },
        };

        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        {
                const void* handler = dispatcher[opA.type][opB.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        add_int_int:
        return (Object) {.type=TYPE_INT, .intval=(opA.intval+opB.intval)};

        add_int_float:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval+opB.floatval)};

        add_container_container:
        if ((opA.payload->type == CONTENT_STRING) && (opB.payload->type == CONTENT_STRING)) {
                return (Object) {.type=TYPE_CONTAINER, .payload=concatenateStrings(opA.payload, opB.payload)};
        }
        else goto error;

        add_float_int:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval+opB.intval)};

        add_float_float:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval+opB.floatval)};

        error:
        TYPEERROR();
}
static Object interpretDifference(const Node* root) {
        static const void* dispatcher[LEN_OBJTYPES][LEN_OBJTYPES] =  {
                [TYPE_INT] = {
                        [TYPE_INT] = &&sub_int_int,
                        [TYPE_BOOL] = &&sub_int_int,
                        [TYPE_FLOAT] = &&sub_int_float,
                },
                [TYPE_BOOL] = {
                        [TYPE_INT] = &&sub_int_int,
                        [TYPE_BOOL] = &&sub_int_int,
                        [TYPE_FLOAT] = &&sub_int_float,
                },
                [TYPE_FLOAT] = {
                        [TYPE_INT] = &&sub_float_int,
                        [TYPE_BOOL] = &&sub_float_int,
                        [TYPE_FLOAT] = &&sub_float_float,
                },
        };

        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        {
                const void* handler = dispatcher[opA.type][opB.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        sub_int_int:
        return (Object) {.type=TYPE_INT, .intval=(opA.intval-opB.intval)};

        sub_int_float:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval-opB.floatval)};

        sub_float_int:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval-opB.intval)};

        sub_float_float:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval-opB.floatval)};

        error:
        TYPEERROR();
}
static Object interpretProduct(const Node* root) {
        static const void* dispatcher[LEN_OBJTYPES][LEN_OBJTYPES] =  {
                [TYPE_INT] = {
                        [TYPE_INT] = &&mul_int_int,
                        [TYPE_BOOL] = &&mul_int_int,
                        [TYPE_FLOAT] = &&mul_int_float,
                        [TYPE_CONTAINER] = &&mul_int_container,
                },
                [TYPE_BOOL] = {
                        [TYPE_INT] = &&mul_int_int,
                        [TYPE_BOOL] = &&mul_int_int,
                        [TYPE_FLOAT] = &&mul_int_float,
                        [TYPE_CONTAINER] = &&mul_int_container,
                },
                [TYPE_FLOAT] = {
                        [TYPE_INT] = &&mul_float_int,
                        [TYPE_BOOL] = &&mul_float_int,
                        [TYPE_FLOAT] = &&mul_float_float,
                },
                [TYPE_CONTAINER] = {
                        [TYPE_INT] = &&mul_container_int,
                        [TYPE_BOOL] = &&mul_container_int,
                },
        };

        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        {
                const void* handler = dispatcher[opA.type][opB.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        mul_int_int:
        return (Object) {.type=TYPE_INT, .intval=(opA.intval*opB.intval)};

        mul_int_float:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval*opB.floatval)};

        mul_int_container:
        if ((opB.payload->type == CONTENT_STRING) && (opA.intval >= 0)) {
                return (Object) {.type=TYPE_CONTAINER, .payload=multiplyString(opB.payload, opA.intval)};
        }
        else goto error;

        mul_container_int:
        if ((opA.payload->type == CONTENT_STRING) && (opB.intval >= 0)) {
                return (Object) {.type=TYPE_CONTAINER, .payload=multiplyString(opA.payload, opB.intval)};
        }
        else goto error;

        mul_float_int:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval*opB.intval)};

        mul_float_float:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval*opB.floatval)};

        error:
        TYPEERROR();
}
static Object interpretDivision(const Node* root) {
        static const void* dispatcher[LEN_OBJTYPES][LEN_OBJTYPES] =  {
                [TYPE_INT] = {
                        [TYPE_INT] = &&div_int_int,
                        [TYPE_BOOL] = &&div_int_int,
                        [TYPE_FLOAT] = &&div_int_float,
                },
                [TYPE_BOOL] = {
                        [TYPE_INT] = &&div_int_int,
                        [TYPE_BOOL] = &&div_int_int,
                        [TYPE_FLOAT] = &&div_int_float,
                },
                [TYPE_FLOAT] = {
                        [TYPE_INT] = &&div_float_int,
                        [TYPE_BOOL] = &&div_float_int,
                        [TYPE_FLOAT] = &&div_float_float,
                },
        };

        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        {
                const void* handler = dispatcher[opA.type][opB.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        div_int_int:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval/(double)opB.intval)};

        div_int_float:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.intval/opB.floatval)};

        div_float_int:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval/opB.intval)};

        div_float_float:
        return (Object) {.type=TYPE_FLOAT, .floatval=(opA.floatval/opB.floatval)};

        error:
        TYPEERROR();
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
