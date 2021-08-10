#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "compiler/node.h"
#include "compiler/parser.h"
#include "compiler/compiler.h"
#include "compiler/bytecode.h"
#include "compiler/compiler_helpers.h"
#include "compiler/mm.h"
#include "error.h"
#include "compiler/builtins.h"


// return SUCCESS (1) or FAILURE (0)
typedef int (*StmtCompilationHandler)(compiler_info *const state, const Node* node);
typedef int (*ExprCompilationHandler)(compiler_info *const state, const Node* node, const Target target);

static int _compile_statement(compiler_info *const state, const Node* node);
static int compile_expression(compiler_info *const state, const Node* node, const Target target);


void mk_compiler_info(compiler_info *const cmpinfo) {
        cmpinfo->program = createProgram();
        cmpinfo->memstate = createMemoryView();
        cmpinfo->currentNS = NULL;
        cmpinfo->current_pos = 0;
        cmpinfo->code_isnonlinear = 0;

        pushNamespace(cmpinfo);
        for (size_t i = 0; i < nb_builtins; i++) {
                Variable v = (Variable) {.name=builtins[i].name, .func=&(builtins[i])};
                addVariable(cmpinfo, v);
        }
}
void del_compiler_info(compiler_info *const cmpinfo) {
        while (cmpinfo->currentNS != NULL) {
                /* we could also have repeatedly popped, but this is faster
                since we don't have to free the variables in the memory view. */
                BFNamespace* next = cmpinfo->currentNS->enclosing;
                free(cmpinfo->currentNS);
                cmpinfo->currentNS = next;
        }
        freeMemoryView(cmpinfo->memstate);
        freeProgram(cmpinfo->program);
}

// ------------------------ compilation handlers -------------------------------

static int compileNop(compiler_info *const state, const Node* node) {
        return 1;
}
static int compileExpressionStmt(compiler_info *const state, const Node* node) {
        const Value val = BF_allocate(state, node->type);
        const Target target = (Target) {.pos=val.pos, .weight=0};
        // weight=0 cuz we don't actually care about the value ;)

        const int status = compile_expression(state, node, target);
        BF_free(state, val);
        return status;

}
static int compileBlock(compiler_info *const state, const Node* node) {
        const uintptr_t len = node->operands[0].len;
        pushNamespace(state);
        for (uintptr_t i=0; i<len; i++) if (!_compile_statement(state, node->operands[i+1].nd)) {
                popNamespace(state);
                return 0;
        }
        popNamespace(state);
        return 1;
}
static int compileDeclaration(compiler_info *const state, const Node* node) {
        Variable v;
        v.name = node->operands[0].nd->token.tok.source;
        v.val = BF_allocate(state, node->type);

        if (node->operands[1].nd != NULL) {
                const Target t = (Target) {.pos = v.val.pos, .weight=1};
                compile_expression(state, node->operands[1].nd, t);
        }
        return addVariable(state, v);
}
static int compileIfElse(compiler_info *const state, const Node* node) {
        if (node->operands[0].nd->type != TYPE_INT) LOG("Warning : non-int type used for condition");
        Value condition = BF_allocate(state, node->operands[0].nd->type);
        Value notCondition = BF_allocate(state, TYPE_INT);
        seekpos(state, notCondition.pos);
        EMIT_PLUS(state, 1);

        if (!compile_expression(state, node->operands[0].nd, (Target){.pos=condition.pos, .weight=1})) {
                BF_free(state, condition);
                BF_free(state, notCondition);
                return 0;
        }

        seekpos(state, condition.pos);
        OPEN_JUMP(state);
        reset(state, condition.pos);
        seekpos(state, notCondition.pos);
        EMIT_MINUS(state, 1);
        int status = _compile_statement(state, node->operands[1].nd);
        seekpos(state, condition.pos);
        CLOSE_JUMP(state);

        BF_free(state, condition);

        if (status && node->operands[2].nd != NULL) {
                seekpos(state, notCondition.pos);
                OPEN_JUMP(state);
                reset(state, notCondition.pos);
                status = _compile_statement(state, node->operands[2].nd);
                seekpos(state, notCondition.pos);
                CLOSE_JUMP(state);
        }

        BF_free(state, notCondition);

        return status;
}
static int compileDoWhile(compiler_info *const state, const Node* node) {
        Value condition = BF_allocate(state, node->operands[1].nd->type);
        seekpos(state, condition.pos);
        EMIT_PLUS(state, 1);
        OPEN_JUMP(state);
        reset(state, condition.pos);
        if (!_compile_statement(state, node->operands[0].nd)) {
                CLOSE_JUMP(state);
                BF_free(state, condition);
                return 0;
        }
        const int status = compile_expression(state, node->operands[1].nd, (Target){.weight=1, .pos=condition.pos});
        seekpos(state, condition.pos);
        CLOSE_JUMP(state);
        BF_free(state, condition);
        return status;
}
static int compileWhile(compiler_info *const state, const Node* node) {
        // condition[action reset condition]
        Value condition = BF_allocate(state, node->operands[0].nd->type);
        if (!compile_expression(state, node->operands[0].nd, (Target){.pos=condition.pos, .weight=1})) {
                BF_free(state, condition);
                return 0;
        }
        seekpos(state, condition.pos);
        OPEN_JUMP(state);
        if (!_compile_statement(state, node->operands[1].nd)) {
                CLOSE_JUMP(state);
                BF_free(state, condition);
                return 0;
        }
        reset(state, condition.pos);
        compile_expression(state, node->operands[0].nd, (Target){.pos=condition.pos, .weight=1});
        seekpos(state, condition.pos);
        CLOSE_JUMP(state);

        BF_free(state, condition);
        return 1;
}

static int compile_iadd(compiler_info *const state, const Node* node) {
        const Variable* v = getVariable(state, node->operands[0].nd->token.tok.source);
        return compile_expression(state, node->operands[1].nd, (Target) {.pos=v->val.pos, .weight=1});
}
static int compile_isub(compiler_info *const state, const Node* node) {
        const Variable* v = getVariable(state, node->operands[0].nd->token.tok.source);
        return compile_expression(state, node->operands[1].nd, (Target) {.pos=v->val.pos, .weight=-1});
}
static int compile_imul(compiler_info *const state, const Node* node) {
        Value temp;
        Variable* v;
        if (node->operands[1].nd->operator == OP_INT) {
                v = getVariable(state, node->operands[0].nd->token.tok.source);
                temp = BF_allocate(state, v->val.type);
                transfer(state, v->val.pos, 1, &((Target){.pos=temp.pos, .weight=atoi(node->operands[1].nd->token.tok.source)}));
        }
        else {
                const Value multiplier = BF_allocate(state, node->operands[1].nd->type);
                if (!compile_expression(state, node->operands[1].nd, (Target){.pos=multiplier.pos, .weight=1})) {
                        BF_free(state, multiplier);
                        return 0;
                }

                // important: get the variable _after_ evaluating the right operand
                // if the code is linear and the variable is referenced in the right operand,
                // it might get moved as a side-effect.
                v = getVariable(state, node->operands[0].nd->token.tok.source);
                temp = BF_allocate(state, v->val.type);

                runtime_mul_int(state, (Target){.pos=temp.pos, .weight=1}, v->val.pos, multiplier.pos);
                BF_free(state, multiplier);
        }

        if (state->code_isnonlinear) {
                transfer(state, temp.pos, 1, &((Target){.pos=v->val.pos, .weight=1}));
                BF_free(state, temp);
        }
        else {
                BF_free(state, v->val);
                v->val = temp;
        }
        return 1;
}
static int compile_affect(compiler_info *const state, const Node* node) {
        const Variable* v = getVariable(state, node->operands[0].nd->token.tok.source);

        const Value temp = BF_allocate(state, v->val.type);
        if (!compile_expression(state, node->operands[1].nd, (Target) {.pos=temp.pos, .weight=1})) {
                BF_free(state, temp);
                return 0;
        }
        const Target tgts[] = {
                {.pos=v->val.pos, .weight=1}
        };
        reset(state, v->val.pos);
        transfer(state, temp.pos, sizeof(tgts)/sizeof(*tgts), tgts);
        BF_free(state, temp);
        return 1;

        // can't do that because the variable might be used to compute the lhs.
        /*
        reset(state, v->val.pos);
        return compile_expression(state, node->operands[1].nd, (Target){.pos=v->val.pos, .weight=1});*/
}

// ------------------------ statement handlers -------------------------------
// ------------------------ expression handlers -------------------------------

static int compile_literal_int(compiler_info *const state, const Node* node, const Target target) {
        seekpos(state, target.pos);
        const ssize_t value = atoi(node->token.tok.source)*target.weight;
        if (value > 0) EMIT_PLUS(state, value);
        else EMIT_MINUS(state, -value);
        return 1;
}
static int compile_variable(compiler_info *const state, const Node* node, const Target target) {
        if (target.weight == 0) return 1;

        Variable *const v = getVariable(state, node->token.tok.source);
        Value copy = BF_allocate(state, v->val.type);
        const Target targets[] = {
                target,
                {.pos=copy.pos, .weight=1}
        };
        transfer(state, v->val.pos, sizeof(targets)/sizeof(*targets), targets);
        if (state->code_isnonlinear) {
                transfer(state, copy.pos, 1, &((Target){.pos=v->val.pos, .weight=1}));
                BF_free(state, copy);
        }
        else {
                Value oldvariable = v->val;
                v->val.pos = copy.pos;
                BF_free(state, oldvariable);
        }
        return 1;
}

static int compile_unary_plus(compiler_info *const state, const Node* node, const Target target) {
        return compile_expression(state, node->operands[0].nd, target);
}
static int compile_unary_minus(compiler_info *const state, const Node* node, const Target target) {
        return compile_expression(state, node->operands[0].nd, (Target) {.pos=target.pos, .weight=-target.weight});
}
static int compile_binary_plus(compiler_info *const state, const Node* node, const Target target) {
        return
           compile_expression(state, node->operands[0].nd, target)
        && compile_expression(state, node->operands[1].nd, target);
}
static int compile_binary_minus(compiler_info *const state, const Node* node, const Target target) {
        return
           compile_expression(state, node->operands[0].nd, target)
        && compile_expression(state, node->operands[1].nd, (Target) {.pos=target.pos, .weight=-target.weight});
}
static int compile_call(compiler_info *const state, const Node* node, const Target target) {
        BuiltinFunctionHandler called = getVariable(state, node->operands[1].nd->token.tok.source)->func->handler;

        return called(state, &(node->operands[2].nd), target);
}
static int compile_multiply(compiler_info *const state, const Node* node, const Target target) {
        const Node* opA = node->operands[0].nd;
        const Node* opB = node->operands[1].nd;
        if ((opA->type != TYPE_INT) || (opB->type != TYPE_INT)) {
                Error(&(node->token), "TypeError: Can't multiply non-integers.\n");
                return 0;
        }


        if (opA->operator == OP_INT) {
                if (opB->operator == OP_INT) {
                        // easy case: multiply an interger literal with another
                        // we can compute that at compile-time
                        seekpos(state, target.pos);
                        EMIT_PLUS(state,
                                 atoi(opA->token.tok.source)
                                *atoi(opB->token.tok.source)
                        );
                        return 1;
                }
                else
                        // multiply a literal with whatever
                        // we just have to adjut the target's weight
                        return compile_expression(state, opB,
                        (Target) {.pos=target.pos, .weight=target.weight*atoi(opA->token.tok.source)}
                );
        }
        else {
                if (opB->operator == OP_INT)
                        // multiply whatever with a literal
                        // we just have to adjut the target's weight
                        return compile_expression(state, opA, (Target) {.pos=target.pos, .weight=target.weight*atoi(opB->token.tok.source)}
                        );
                else {
                        // general case, basically two nested `transfer`s

                        // principle: X*Y = Y+Y+Y+Y+Y+Y+… (X times)
                        // so we "just" transfer Y on the target, without losing its value, X times

                        // shortcut if we don't actually need the result
                        // we still recurse, there might be side-effects down in the AST.
                        if (target.weight == 0) {
                                return compile_expression(state, opA, target) && compile_expression(state, opB, target);
                        }

                        Value xval = BF_allocate(state, opA->type);
                        Value yval = BF_allocate(state, opB->type);
                        if ( // computing operands
                        !compile_expression(state, opA, (Target) {.pos=yval.pos, .weight=1}) ||
                        !compile_expression(state, opB, (Target) {.pos=xval.pos, .weight=1})
                        ) {
                                BF_free(state, xval);
                                BF_free(state, yval);
                                return 0;
                        }
                        runtime_mul_int(state, target, xval.pos, yval.pos);
                        return 1;
                }
        }
}
static int compile_not(compiler_info *const state, const Node* node, const Target target) {
        Value temp = BF_allocate(state, node->type);
        int status = compile_expression(state, node->operands[0].nd, (Target){.weight=1, .pos=temp.pos});

        if (status) {
                seekpos(state, target.pos);
                EMIT_PLUS(state, 1);

                seekpos(state, temp.pos);
                OPEN_JUMP(state);
                reset(state, temp.pos);
                seekpos(state, target.pos);
                EMIT_MINUS(state, 1);
                seekpos(state, temp.pos);
                CLOSE_JUMP(state);
        }
        BF_free(state, temp);
        return status;

}

// ------------------------ end compilation handlers ---------------------------

static int compile_expression(compiler_info *const state, const Node* node, const Target target) {
        static const ExprCompilationHandler handlers[LEN_OPERATORS] = {
                [OP_INT] = compile_literal_int,
                [OP_VARIABLE] = compile_variable,
                [OP_UNARY_PLUS] = compile_unary_plus,
                [OP_UNARY_MINUS] = compile_unary_minus,
                [OP_SUM] = compile_binary_plus,
                [OP_DIFFERENCE] = compile_binary_minus,
                [OP_NE] = compile_binary_minus,
                [OP_CALL] = compile_call,
                [OP_PRODUCT] = compile_multiply,
                [OP_INVERT] = compile_not,
        };

        const ExprCompilationHandler handler = handlers[node->operator];
        if (handler == NULL) {
                LOG("Operand not (yet) implemented");
                Error(&(node->token), "SyntaxError: invalid syntax.\n");
                return 0;
        }

        return handler(state, node, target);
}

static int _compile_statement(compiler_info *const state, const Node* node) {
        static const StmtCompilationHandler handlers[LEN_OPERATORS] = {
                [OP_NOP] = compileNop,
                [OP_BLOCK] = compileBlock,
                [OP_DECLARE] = compileDeclaration,
                [OP_IFELSE] = compileIfElse,
                [OP_DOWHILE] = compileDoWhile,
                [OP_WHILE] = compileWhile,
                [OP_AFFECT] = compile_affect,
                [OP_IADD] = compile_iadd,
                [OP_ISUB] = compile_isub,
                [OP_IMUL] = compile_imul,
        };

        const StmtCompilationHandler handler = handlers[node->operator];
        if (handler == NULL) return compileExpressionStmt(state, node);
        else return handler(state, node);
}
int compile_statement(compiler_info *const state) {
        Node* node = parse_statement(&(state->prsinfo));
        if (node == NULL) return 0;
        else return _compile_statement(state, node);
}


// ---------------------- built-in functions -----------------------------------

static int builtin_print_int(compiler_info *const state, const struct Node* arg) {
        if (arg->operator == OP_VARIABLE) {
                Value val = getVariable(state, arg->token.tok.source)->val;
                seekpos(state, val.pos);
                EMIT_OUTPUT(state);
                return 1;
        }
        else {
                Value val = BF_allocate(state, arg->type);
                Target t = {.pos=val.pos, .weight=1};
                int status = compile_expression(state, arg, t);
                if (status) {
                        seekpos(state, val.pos);
                        EMIT_OUTPUT(state);
                }
                BF_free(state, val);
                return status;
        }
}

static int builtin_print(compiler_info *const state, struct Node *const argv[], const Target target) {
        switch (argv[0]->type) {
                case TYPE_INT:
                        return builtin_print_int(state, argv[0]);
                default:
                        return 0;
        }
}

BuiltinFunction builtins[] = {
        {
                .handler=builtin_print,
                .name="print", // don't forget to internalize this (and therefore strdup' it)
                .arity=1,
                .returnType=TYPE_VOID
        }
};
const size_t nb_builtins = sizeof(builtins)/sizeof(*builtins);

// ---------------------- built-in functions -----------------------------------
