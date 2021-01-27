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
        return (Object) {.type=TYPE_BOOL, .intval=(root->token.type == TOKEN_TRUE)};

}
static Object interpretFloat(const Node* root) {
        return (Object) {.type=TYPE_FLOAT, .floatval=atof(root->token.source)};
}
static Object interpretStr(const Node* root) {
        return (Object) {.type=TYPE_STRING, .strval=makeString(root->token.source+1, root->token.length-2)};
}
static Object interpretUnaryPlus(const Node* root) {
        Object operand = interpret(root->operands[0]);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_FLOAT:
                        return operand;
                case TYPE_STRING:
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
                case TYPE_STRING:
                        TYPEERROR();
                default: TYPEERROR();
        }
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
                [TYPE_STRING] = {
                        [TYPE_STRING] = &&add_string_string,
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

        add_string_string:
        return (Object) {.type=TYPE_STRING, .strval=concatenateStrings(opA.strval, opB.strval)};

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
                        [TYPE_STRING] = &&mul_int_string,
                },
                [TYPE_BOOL] = {
                        [TYPE_INT] = &&mul_int_int,
                        [TYPE_BOOL] = &&mul_int_int,
                        [TYPE_FLOAT] = &&mul_int_float,
                        [TYPE_STRING] = &&mul_int_string,
                },
                [TYPE_FLOAT] = {
                        [TYPE_INT] = &&mul_float_int,
                        [TYPE_BOOL] = &&mul_float_int,
                        [TYPE_FLOAT] = &&mul_float_float,
                },
                [TYPE_STRING] = {
                        [TYPE_INT] = &&mul_string_int,
                        [TYPE_BOOL] = &&mul_string_int,
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

        mul_int_string:
        if (opA.intval >= 0) return (Object) {.type=TYPE_STRING, .strval=multiplyString(opB.strval, opA.intval)};
        else goto error;

        mul_string_int:
        if (opB.intval >= 0) return (Object) {.type=TYPE_STRING, .strval=multiplyString(opA.strval, opB.intval)};
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
static Object interpretInvert(const Node* root) {
        Object operand = interpret(root->operands[0]);
        switch (operand.type) {
                case TYPE_INT:
                        operand.type = TYPE_BOOL;
                case TYPE_BOOL:
                        operand.intval = !operand.intval;
                        return operand;
                case TYPE_FLOAT:
                        TYPEERROR();
                case TYPE_STRING:
                        TYPEERROR();
                default: TYPEERROR();
        }
}
static Object interpretAnd(const Node* root) {
        Object operand = interpret(root->operands[0]);
        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        if (!operand.intval) return operand;
                        break;
                case TYPE_FLOAT:
                        TYPEERROR();
                case TYPE_STRING:
                        if (!operand.strval->len) return operand;
                        break;
                case TYPE_NONE:
                        return operand;
                default: TYPEERROR();
        }
        operand = interpret(root->operands[1]);
        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_STRING:
                case TYPE_NONE:
                        return operand;
                case TYPE_FLOAT:
                        TYPEERROR();
                default: TYPEERROR();
        }
}
static Object interpretOr(const Node* root) {
        Object operand = interpret(root->operands[0]);
        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        if (operand.intval) return operand;
                        break;
                case TYPE_FLOAT:
                        TYPEERROR();
                case TYPE_STRING:
                        if (operand.strval->len) return operand;
                        break;
                case TYPE_NONE:
                        break;
                default: TYPEERROR();
        }
        operand = interpret(root->operands[1]);
        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_STRING:
                case TYPE_NONE:
                        return operand;
                case TYPE_FLOAT:
                        TYPEERROR();
                default: TYPEERROR();
        }
}
static Object interpretEq(const Node* root) {
        static const void* dispatcher[LEN_OBJTYPES][LEN_OBJTYPES] = {
                [TYPE_INT] = {
                        [TYPE_INT] = &&eq_int_int,
                        [TYPE_BOOL] = &&eq_int_int,
                },
                [TYPE_BOOL] = {
                        [TYPE_INT] = &&eq_int_int,
                        [TYPE_BOOL] = &&eq_int_int,
                },
                [TYPE_STRING] = {
                        [TYPE_STRING] = &&eq_string_string,
                },
                [TYPE_NONE] = {
                        [TYPE_NONE] = && eq_none_none,
                },
        };

        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        {
                const void* handler = dispatcher[opA.type][opB.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        eq_int_int:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.intval==opB.intval)};

        eq_string_string:
        if (opA.strval->len != opB.strval->len) return (Object) {.type=TYPE_BOOL, .intval=0};
        return (Object) {.type=TYPE_BOOL, .intval=!strcmp(opA.strval->value, opB.strval->value)};

        eq_none_none:
        return (Object) {.type=TYPE_BOOL, .intval=1};

        error:
        // two objects of incompatible types are different
        return (Object) {.type=TYPE_BOOL, .intval=0};
}
static Object interpretLt(const Node* root) {
        static const void* dispatcher[LEN_OBJTYPES][LEN_OBJTYPES] = {
                [TYPE_INT] = {
                        [TYPE_INT] = &&lt_int_int,
                        [TYPE_BOOL] = &&lt_int_int,
                        [TYPE_FLOAT] = &&lt_int_float,
                },
                [TYPE_BOOL] = {
                        [TYPE_INT] = &&lt_int_int,
                        [TYPE_BOOL] = &&lt_int_int,
                        [TYPE_FLOAT] = &&lt_int_float,
                },
                [TYPE_FLOAT] = {
                        [TYPE_INT] = &&lt_float_int,
                        [TYPE_BOOL] = &&lt_float_int,
                        [TYPE_FLOAT] = &&lt_float_float,
                },
        };

        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        {
                const void* handler = dispatcher[opA.type][opB.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        lt_int_int:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.intval<opB.intval)};

        lt_int_float:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.intval<opB.floatval)};

        lt_float_int:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.floatval<opB.intval)};

        lt_float_float:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.floatval<opB.floatval)};

        error:
        TYPEERROR();
}
static Object interpretLe(const Node* root) {
        static const void* dispatcher[LEN_OBJTYPES][LEN_OBJTYPES] = {
                [TYPE_INT] = {
                        [TYPE_INT] = &&le_int_int,
                        [TYPE_BOOL] = &&le_int_int,
                        [TYPE_FLOAT] = &&le_int_float,
                },
                [TYPE_BOOL] = {
                        [TYPE_INT] = &&le_int_int,
                        [TYPE_BOOL] = &&le_int_int,
                        [TYPE_FLOAT] = &&le_int_float,
                },
                [TYPE_FLOAT] = {
                        [TYPE_INT] = &&le_float_int,
                        [TYPE_BOOL] = &&le_float_int,
                        [TYPE_FLOAT] = &&le_float_float,
                },
        };

        Object opA = interpret(root->operands[0]);
        Object opB = interpret(root->operands[1]);
        {
                const void* handler = dispatcher[opA.type][opB.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        le_int_int:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.intval<=opB.intval)};

        le_int_float:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.intval<=opB.floatval)};

        le_float_int:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.floatval<=opB.intval)};

        le_float_float:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.floatval<=opB.floatval)};

        error:
        TYPEERROR();
}
static Object interpretNone(const Node* root) {
        return (Object) {.type=TYPE_NONE};
}

Object interpret(const Node* root) {
        static const InterpretFn interpreters[] = {
                [OP_VARIABLE] = interpretVariable,

                [OP_INT] = interpretInt,
                [OP_BOOL] = interpretBool,
                [OP_FLOAT] = interpretFloat,
                [OP_STR] = interpretStr,
                [OP_NONE] = interpretNone,

                [OP_UNARY_PLUS] = interpretUnaryPlus,
                [OP_UNARY_MINUS] = interpretUnaryMinus,
                [OP_INVERT] = interpretInvert,

                [OP_SUM] = interpretSum,
                [OP_DIFFERENCE] = interpretDifference,
                [OP_PRODUCT] = interpretProduct,
                [OP_DIVISION] = interpretDivision,
                [OP_AFFECT] = interpretAffect,
                [OP_AND] = interpretAnd,
                [OP_OR] = interpretOr,
                [OP_EQ] = interpretEq,
                [OP_LT] = interpretLt,
                [OP_LE] = interpretLe,
        };
        return interpreters[root->operator](root);
}

#undef TYPEERROR
#undef NOTIMPLEMENTED
