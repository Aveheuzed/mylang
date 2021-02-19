#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <alloca.h>

#include "headers/pipeline/interpreter.h"
#include "headers/pipeline/node.h"
#include "headers/utils/error.h"

#define SUCCESS 1
#define FAILURE 0

Object interpretExpression(const Node* root, Namespace **const ns);
int _interpretStatement(parser_info *const prsinfo, const Node* root, Namespace **const ns);

typedef Object (*ExprInterpretFn)(const Node* root, Namespace **const ns);
typedef int (*StmtInterpretFn)(parser_info *const prsinfo, const Node* root, Namespace **const ns);

static Object interpretVariable(const Node* root, Namespace **const ns) {
        Object* obj = ns_get_value(*ns, root->token.source);
        if (obj == NULL) {
                RuntimeError(root->token);
                return OBJ_NONE;
        }
        else return *obj;
}
static Object interpretInt(const Node* root, Namespace **const ns) {
        return (Object) {.type=TYPE_INT, .intval=atoll(root->token.source)};
}
static Object interpretBool(const Node* root, Namespace **const ns) {
        return (Object) {.type=TYPE_BOOL, .intval=(root->token.type == TOKEN_TRUE)};

}
static Object interpretFloat(const Node* root, Namespace **const ns) {
        return (Object) {.type=TYPE_FLOAT, .floatval=atof(root->token.source)};
}
static Object interpretStr(const Node* root, Namespace **const ns) {
        return (Object) {.type=TYPE_STRING, .strval=makeString(root->token.source+1, root->token.length-2)};
}
static Object interpretUnaryPlus(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0], ns);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_FLOAT:
                        return operand;
                case TYPE_STRING:
                        TypeError(root->token); return OBJ_NONE;
                default: TypeError(root->token); return OBJ_NONE;
        }
}
static Object interpretUnaryMinus(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0], ns);

        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        operand.intval *= -1;
                        return operand;
                case TYPE_FLOAT:
                        operand.floatval *= -1;
                        return operand;
                case TYPE_STRING:
                        TypeError(root->token); return OBJ_NONE;
                default: TypeError(root->token); return OBJ_NONE;
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

        Object opA = interpretExpression(root->operands[0], ns);
        Object opB = interpretExpression(root->operands[1], ns);
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
        TypeError(root->token); return OBJ_NONE;
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

        Object opA = interpretExpression(root->operands[0], ns);
        Object opB = interpretExpression(root->operands[1], ns);
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
        TypeError(root->token); return OBJ_NONE;
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

        Object opA = interpretExpression(root->operands[0], ns);
        Object opB = interpretExpression(root->operands[1], ns);
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
        TypeError(root->token); return OBJ_NONE;
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

        Object opA = interpretExpression(root->operands[0], ns);
        Object opB = interpretExpression(root->operands[1], ns);
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
        TypeError(root->token); return OBJ_NONE;
}
static Object interpretAffect(const Node* root, Namespace **const ns) {
        Object obj = interpretExpression(root->operands[1], ns);
        ns_set_value(ns, root->operands[0]->token.source, obj);
        return obj;
}
static Object interpretInvert(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0], ns);
        switch (operand.type) {
                case TYPE_INT:
                        operand.type = TYPE_BOOL;
                case TYPE_BOOL:
                        operand.intval = !operand.intval;
                        return operand;
                case TYPE_FLOAT:
                        TypeError(root->token); return OBJ_NONE;
                case TYPE_STRING:
                        TypeError(root->token); return OBJ_NONE;
                default: TypeError(root->token); return OBJ_NONE;
        }
}
static Object interpretAnd(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0], ns);
        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        if (!operand.intval) return operand;
                        break;
                case TYPE_FLOAT:
                        TypeError(root->token); return OBJ_NONE;
                case TYPE_STRING:
                        if (!operand.strval->len) return operand;
                        break;
                case TYPE_NONE:
                        return operand;
                default: TypeError(root->token); return OBJ_NONE;
        }
        operand = interpretExpression(root->operands[1], ns);
        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_STRING:
                case TYPE_NONE:
                        return operand;
                case TYPE_FLOAT:
                        TypeError(root->token); return OBJ_NONE;
                default: TypeError(root->token); return OBJ_NONE;
        }
}
static Object interpretOr(const Node* root, Namespace **const ns) {
        Object operand = interpretExpression(root->operands[0], ns);
        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                        if (operand.intval) return operand;
                        break;
                case TYPE_FLOAT:
                        TypeError(root->token); return OBJ_NONE;
                case TYPE_STRING:
                        if (operand.strval->len) return operand;
                        break;
                case TYPE_NONE:
                        break;
                default: TypeError(root->token); return OBJ_NONE;
        }
        operand = interpretExpression(root->operands[1], ns);
        switch (operand.type) {
                case TYPE_INT:
                case TYPE_BOOL:
                case TYPE_STRING:
                case TYPE_NONE:
                        return operand;
                case TYPE_FLOAT:
                        TypeError(root->token); return OBJ_NONE;
                default: TypeError(root->token); return OBJ_NONE;
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

        Object opA = interpretExpression(root->operands[0], ns);
        Object opB = interpretExpression(root->operands[1], ns);
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

        Object opA = interpretExpression(root->operands[0], ns);
        Object opB = interpretExpression(root->operands[1], ns);
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
        TypeError(root->token); return OBJ_NONE;
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

        Object opA = interpretExpression(root->operands[0], ns);
        Object opB = interpretExpression(root->operands[1], ns);
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
        TypeError(root->token); return OBJ_NONE;
}
static Object interpretNone(const Node* root, Namespace **const ns) {
        return OBJ_NONE;
}
static Object interpretCall(const Node* root, Namespace **const ns) {
        Object funcnode = interpretExpression(root->operands[1], ns);
        if (funcnode.type != TYPE_NATIVEF) {
                TypeError(root->token);
                return OBJ_NONE;
        }


        const uintptr_t argc = (uintptr_t) root->operands[0]-1;

        Object *const argv = alloca(argc*sizeof(Object));
        for (uintptr_t iarg=0; iarg<argc; iarg++) {
                argv[iarg] = interpretExpression(root->operands[iarg+2], ns);
        }
        return funcnode.natfunval(argc, argv);
}

static int interpretBlock(parser_info *const prsinfo, const Node* root, Namespace **const ns) {
        const uintptr_t nb_children = (uintptr_t) root->operands[0];

        Namespace * new_ns = allocateNamespace(ns);

        int exit_code = SUCCESS;

        for (uintptr_t i=1; i<=nb_children; i++) {
                if (!_interpretStatement(prsinfo, root->operands[i], &new_ns)) {
                        exit_code = FAILURE;
                        break;
                }
        }

        freeNamespace(new_ns);
        return exit_code;
}
static int interpretIf(parser_info *const prsinfo, const Node* root, Namespace **const ns) {
        Object predicate = interpretExpression(root->operands[0], ns);
        bool branch;
        switch (predicate.type) {
                case TYPE_INT:
                        branch = (predicate.intval != 0); break;
                case TYPE_BOOL:
                        branch = (predicate.intval != 0); break;
                case TYPE_NONE:
                        branch = false; break;
                case TYPE_FLOAT:
                        TypeError(root->token); return FAILURE;
                case TYPE_STRING:
                        branch = (predicate.strval->len > 0); break;
                default:
                        TypeError(root->token); return FAILURE;

        }
        if (branch) return _interpretStatement(prsinfo, root->operands[1], ns);
        else if (root->operands[2] != NULL) return _interpretStatement(prsinfo, root->operands[2], ns);
        return SUCCESS;
}

Object interpretExpression(const Node* root, Namespace **const ns) {
        static const ExprInterpretFn interpreters[LEN_OPERATORS] = {
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

                [OP_CALL] = interpretCall,
        };
        LOG("");
        return interpreters[root->operator](root, ns);
}
int _interpretStatement(parser_info *const prsinfo, const Node* root, Namespace **const ns) {
        static const StmtInterpretFn interpreters[LEN_OPERATORS] = {
                [OP_BLOCK] = interpretBlock,
                [OP_IFELSE] = interpretIf,
        };

        if (root == NULL) return FAILURE;

        StmtInterpretFn interpreter = interpreters[root->operator];

        if (interpreter == NULL) interpretExpression(root, ns);
        else interpreters[root->operator](prsinfo, root, ns);

        return SUCCESS;
}

int interpretStatement(parser_info *const prsinfo, Namespace **const ns) {
        LOG("Interpreting a new statement");
        Node* root = parse_statement(prsinfo);
        return _interpretStatement(prsinfo, root, ns);
}

#undef SUCCESS
#undef FAILURE
