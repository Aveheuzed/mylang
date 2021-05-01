#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

#include "headers/pipeline/interpreter.h"
#include "headers/pipeline/node.h"
#include "headers/utils/error.h"
#include "headers/utils/builtins.h"


static Object interpretExpression(const Node* root, Namespace **const ns);
static int _interpretStatement(const Node* root, Namespace **const ns);

typedef Object (*ExprInterpretFn)(const Node* root, Namespace **const ns);
typedef int (*StmtInterpretFn)(const Node* root, Namespace **const ns);

static Object interpretVariable(const Node* root, Namespace **const ns) {
        Object* obj = ns_get_value(*ns, root->token.tok.source);
        if (obj == NULL) {
                RuntimeError(root->token);
                return ERROR;
        }
        else return *obj;
}
static Object interpretInt(const Node* root, Namespace **const ns) {
        return (Object) {.type=TYPE_INT, .intval=root->operands[0].obj.intval};
}
static Object interpretTrue(const Node* root, Namespace **const ns) {
        return OBJ_TRUE;
}
static Object interpretFalse(const Node* root, Namespace **const ns) {
        return OBJ_FALSE;
}
static Object interpretNone(const Node* root, Namespace **const ns) {
        return OBJ_NONE;
}
static Object interpretFloat(const Node* root, Namespace **const ns) {
        return (Object) {.type=TYPE_FLOAT, .floatval=root->operands[0].obj.floatval};
}
static Object interpretStr(const Node* root, Namespace **const ns) {
        return (Object) {.type=TYPE_STRING, .strval=root->operands[0].obj.strval};
}
static Object interpretUnaryPlus(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(operand);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_FLOAT:
                        return operand;
                case TYPE_STRING:
                        TypeError(root->token); return ERROR;
                default: TypeError(root->token); return ERROR;
        }
}
static Object interpretUnaryMinus(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(operand);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        operand.intval *= -1;
                        return operand;
                case TYPE_FLOAT:
                        operand.floatval *= -1;
                        return operand;
                case TYPE_STRING:
                        TypeError(root->token); return ERROR;
                default: TypeError(root->token); return ERROR;
        }
}
static Object interpretSum(const Node* root, Namespace **const ns) {
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

        Object opA = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(opA);
        Object opB = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(opB);
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
        TypeError(root->token); return ERROR;
}
static Object interpretDifference(const Node* root, Namespace **const ns) {
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

        Object opA = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(opA);
        Object opB = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(opB);
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
        TypeError(root->token); return ERROR;
}
static Object interpretProduct(const Node* root, Namespace **const ns) {
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

        Object opA = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(opA);
        Object opB = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(opB);
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
        TypeError(root->token); return ERROR;
}
static Object interpretDivision(const Node* root, Namespace **const ns) {
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

        Object opA = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(opA);
        Object opB = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(opB);
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
        TypeError(root->token); return ERROR;
}
static Object interpretAffect(const Node* root, Namespace **const ns) {
        Object obj = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(obj);
        ns_set_value(ns, root->operands[0].nd->token.tok.source, obj);
        return obj;
}
static Object interpretInvert(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(operand);
        switch (operand.type) {
                case TYPE_INT:
                        operand.type = TYPE_BOOL;
                case TYPE_BOOL:
                        operand.intval = !operand.intval;
                        return operand;
                case TYPE_FLOAT:
                        TypeError(root->token); return ERROR;
                case TYPE_STRING:
                        TypeError(root->token); return ERROR;
                default: TypeError(root->token); return ERROR;
        }
}
static Object interpretAnd(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(operand);
        Object op_is_true = tobool(1, &operand);
        if (op_is_true.type == TYPE_ERROR) {
                TypeError(root->token); return ERROR;
        }
        else if (!op_is_true.intval) return operand;
        else {
                operand = interpretExpression(root->operands[1].nd, ns);
                ERROR_GUARD(operand);
                if (tobool(1, &operand).type == TYPE_ERROR) {
                        TypeError(root->token); return ERROR;
                }
                return operand;
        }
}
static Object interpretOr(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(operand);
        Object op_is_true = tobool(1, &operand);
        if (op_is_true.type == TYPE_ERROR) {
                TypeError(root->token); return ERROR;
        }
        else if (op_is_true.intval) return operand;
        else {
                operand = interpretExpression(root->operands[1].nd, ns);
                ERROR_GUARD(operand);
                if (tobool(1, &operand).type == TYPE_ERROR) {
                        TypeError(root->token); return ERROR;
                }
                return operand;
        }
}
static Object interpretEq(const Node* root, Namespace **const ns) {
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

        Object opA = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(opA);
        Object opB = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(opB);
        {
                const void* handler = dispatcher[opA.type][opB.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        eq_int_int:
        return (Object) {.type=TYPE_BOOL, .intval=(opA.intval==opB.intval)};

        eq_string_string:
        if (opA.strval->len != opB.strval->len) return OBJ_FALSE;
        return (Object) {.type=TYPE_BOOL, .intval=!strcmp(opA.strval->value, opB.strval->value)};

        eq_none_none:
        return OBJ_TRUE;

        error:
        // two objects of incompatible types are different
        return OBJ_FALSE;
}
static Object interpretLt(const Node* root, Namespace **const ns) {
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

        Object opA = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(opA);
        Object opB = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(opB);
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
        TypeError(root->token); return ERROR;
}
static Object interpretLe(const Node* root, Namespace **const ns) {
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

        Object opA = interpretExpression(root->operands[0].nd, ns);
        ERROR_GUARD(opA);
        Object opB = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(opB);
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
        TypeError(root->token); return ERROR;
}
static Object interpretCall(const Node* root, Namespace **const ns) {
        Object funcnode = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(funcnode);
        if (funcnode.type != TYPE_NATIVEF) {
                TypeError(root->token);
                return ERROR;
        }


        const uintptr_t argc = root->operands[0].len-1;

        Object *const argv = alloca(argc*sizeof(Object));
        for (uintptr_t iarg=0; iarg<argc; iarg++) {
                argv[iarg] = interpretExpression(root->operands[iarg+2].nd, ns);
                ERROR_GUARD(argv[iarg]);
        }
        Object result = funcnode.natfunval(argc, argv);
        if (result.type == TYPE_ERROR) RuntimeError(root->token);
        return result;
}
static Object interpret_iadd(const Node* root, Namespace **const ns) {
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

        // we must isolate the current namespace, because we don't want to write into an enclosing one, juste like `=`.
        Namespace ** enclosing = (**ns).enclosing;
        (**ns).enclosing = NULL;

        Object* target = ns_get_value(*ns, root->operands[0].nd->token.tok.source);
        (**ns).enclosing = enclosing;
        if (target == NULL) {
                RuntimeError(root->token);
                return ERROR;
        }

        Object increment = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(increment);

        {
                const void* handler = dispatcher[target->type][increment.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        add_int_int:
        target->intval += increment.intval;
        return *target;

        add_int_float:
        target->floatval = target->intval + increment.floatval;
        target->type = TYPE_FLOAT;
        return *target;

        add_string_string:
        target->strval = concatenateStrings(target->strval, increment.strval);
        return *target;

        add_float_int:
        target->floatval += increment.intval;
        return *target;

        add_float_float:
        target->floatval += increment.floatval;
        return *target;


        // actually already done thanks to *pointers*
        // success:
        // ns_set_value(ns, root->operands[0].nd->token.tok.source, *target);

        error:
        TypeError(root->token); return ERROR;
}
static Object interpret_isub(const Node* root, Namespace **const ns) {
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

        Namespace ** enclosing = (**ns).enclosing;
        (**ns).enclosing = NULL;

        Object* target = ns_get_value(*ns, root->operands[0].nd->token.tok.source);
        (**ns).enclosing = enclosing;
        if (target == NULL) {
                RuntimeError(root->token);
                return ERROR;
        }

        Object increment = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(increment);

        {
                const void* handler = dispatcher[target->type][increment.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        sub_int_int:
        target->intval -= increment.intval;
        return *target;

        sub_int_float:
        target->floatval = target->intval - increment.floatval;
        target->type = TYPE_FLOAT;
        return *target;

        sub_float_int:
        target->floatval -= increment.intval;
        return *target;

        sub_float_float:
        target->floatval -= increment.floatval;
        return *target;


        // actually already done thanks to *pointers*
        // success:
        // ns_set_value(ns, root->operands[0].nd->token.tok.source, *target);

        error:
        TypeError(root->token); return ERROR;
}
static Object interpret_imul(const Node* root, Namespace **const ns) {
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

        Namespace ** enclosing = (**ns).enclosing;
        (**ns).enclosing = NULL;

        Object* target = ns_get_value(*ns, root->operands[0].nd->token.tok.source);
        (**ns).enclosing = enclosing;
        if (target == NULL) {
                RuntimeError(root->token);
                return ERROR;
        }

        Object increment = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(increment);

        {
                const void* handler = dispatcher[target->type][increment.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        mul_int_string:
        if (target->intval < 0) goto error;
        target->strval = multiplyString(increment.strval, target->intval);
        target->type = TYPE_STRING;
        return *target;

        mul_string_int:
        if (increment.intval < 0) goto error;
        target->strval = multiplyString(target->strval, increment.intval);
        return *target;

        mul_int_int:
        target->intval *= increment.intval;
        return *target;

        mul_int_float:
        target->floatval = target->intval * increment.floatval;
        target->type = TYPE_FLOAT;
        return *target;

        mul_float_int:
        target->floatval *= increment.intval;
        return *target;

        mul_float_float:
        target->floatval *= increment.floatval;
        return *target;


        // actually already done thanks to *pointers*
        // success:
        // ns_set_value(ns, root->operands[0].nd->token.tok.source, *target);

        error:
        TypeError(root->token); return ERROR;
}
static Object interpret_idiv(const Node* root, Namespace **const ns) {
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

        Namespace ** enclosing = (**ns).enclosing;
        (**ns).enclosing = NULL;

        Object* target = ns_get_value(*ns, root->operands[0].nd->token.tok.source);
        (**ns).enclosing = enclosing;
        if (target == NULL) {
                RuntimeError(root->token);
                return ERROR;
        }

        Object increment = interpretExpression(root->operands[1].nd, ns);
        ERROR_GUARD(increment);

        {
                const void* handler = dispatcher[target->type][increment.type];
                if (handler == NULL) goto error; // undefined array members are initialized to NULL (C99)
                else goto *handler;
        }

        div_int_int:
        target->intval /= increment.intval;
        return *target;

        div_int_float:
        target->floatval = target->intval / increment.floatval;
        target->type = TYPE_FLOAT;
        return *target;

        div_float_int:
        target->floatval /= increment.intval;
        return *target;

        div_float_float:
        target->floatval /= increment.floatval;
        return *target;


        // actually already done thanks to *pointers*
        // success:
        // ns_set_value(ns, root->operands[0].nd->token.tok.source, *target);

        error:
        TypeError(root->token); return ERROR;
}

static int interpretBlock(const Node* root, Namespace **const ns) {
        const uintptr_t nb_children = root->operands[0].len;

        for (uintptr_t i=1; i<=nb_children; i++) {
                if (!_interpretStatement(root->operands[i].nd, ns)) return 0;
        }
        return 1;
}
static int interpretIf(const Node* root, Namespace **const ns) {
        Object predicate = interpretExpression(root->operands[0].nd, ns);
        predicate = tobool(1, &predicate);
        if (predicate.type == TYPE_ERROR) return 0;
        if (predicate.intval) return _interpretStatement(root->operands[1].nd, ns);
        else if (root->operands[2].nd != NULL) return _interpretStatement(root->operands[2].nd, ns);
        else return 1;
}
static int interpretWhile(const Node* root, Namespace **const ns) {
        Object predicate;
        while (
                predicate = interpretExpression(root->operands[0].nd, ns),
                predicate = tobool(1, &predicate),
                predicate.type != TYPE_ERROR && predicate.intval
        ) {
                if (!_interpretStatement(root->operands[1].nd, ns)) {
                        return 0;
                }
        }
        if (predicate.type == TYPE_ERROR) return 0;
        else return 1;
}
static int interpretNop(const Node* root, Namespace **const ns) {
        return 1;
}

static Object interpretExpression(const Node* root, Namespace **const ns) {
        static const ExprInterpretFn interpreters[LEN_OPERATORS] = {
                [OP_VARIABLE] = interpretVariable,

                [OP_LITERAL_INT] = interpretInt,
                [OP_LITERAL_FLOAT] = interpretFloat,
                [OP_LITERAL_TRUE] = interpretTrue,
                [OP_LITERAL_FALSE] = interpretFalse,
                [OP_LITERAL_NONE] = interpretNone,
                [OP_LITERAL_STR] = interpretStr,

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
                [OP_IADD] = interpret_iadd,
                [OP_ISUB] = interpret_isub,
                [OP_IMUL] = interpret_imul,
                [OP_IDIV] = interpret_idiv,

                [OP_CALL] = interpretCall,
        };

        const ExprInterpretFn handler = interpreters[root->operator];
        if (handler == NULL) {
                RuntimeError(root->token);
                return ERROR;
        }
        return handler(root, ns);
}
static int _interpretStatement(const Node* root, Namespace **const ns) {
        static const StmtInterpretFn interpreters[LEN_OPERATORS] = {
                [OP_BLOCK] = interpretBlock,
                [OP_IFELSE] = interpretIf,
                [OP_WHILE] = interpretWhile,
                [OP_NOP] = interpretNop,
        };

        if (root == NULL) return 0;

        StmtInterpretFn interpreter = interpreters[root->operator];


        if (interpreter == NULL) {
                Object result = interpretExpression(root, ns);
                return result.type != TYPE_ERROR;
        }
        else {
                return interpreters[root->operator](root, ns);
        }
}

int interpretStatement(parser_info *const prsinfo, Namespace **const ns) {
        LOG("Interpreting a new statement");
        Node* root = parse_statement(prsinfo);
        return _interpretStatement(root, ns);
}
