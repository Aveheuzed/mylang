#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "headers/pipeline/node.h"
#include "headers/pipeline/compiler.h"
#include "headers/utils/compiler_helpers.h"
#include "headers/utils/mm.h"
#include "headers/utils/error.h"
#include "headers/utils/builtins.h"


// return SUCCESS (1) or FAILURE (0)
typedef int (*StmtCompilationHandler)(compiler_info *const state, const Node* node);
typedef int (*ExprCompilationHandler)(compiler_info *const state, const Node* node, const Target target);

static int _compile_statement(compiler_info *const state, const Node* node);
static int compile_expression(compiler_info *const state, const Node* node, const Target target);


compiler_info mk_compiler_info(FILE* file) {
        compiler_info c = (compiler_info) {
                .prsinfo=mk_parser_info(file),
                .program=createProgram(),
                .memstate=createMemoryView(),
                .currentNS=NULL,
                .current_pos=0,
        };
        pushNamespace(&c);
        for (size_t i = 0; i < nb_builtins; i++) {
                Variable v = (Variable) {.name=builtins[i].name, .func=&(builtins[i])};
                addVariable(&c, v);
        }
        return c;
}
CompiledProgram* del_compiler_info(compiler_info cmpinfo) {
        while (cmpinfo.currentNS != NULL) {
                /* we could also have repeatedly popped, but this is faster
                since we don't have to free the variables in the memory view. */
                BFNamespace* next = cmpinfo.currentNS->enclosing;
                free(cmpinfo.currentNS);
                cmpinfo.currentNS = next;
        }
        freeMemoryView(cmpinfo.memstate);
        return cmpinfo.program;
}

// ------------------------ compilation handlers -------------------------------

static int compileNop(compiler_info *const state, const Node* node) {
        return 1;
}
static int compileExpressionStmt(compiler_info *const state, const Node* node) {
        const Value val = BF_allocate(state, node->type);
#ifdef DEBUG
        const Target target = (Target) {.pos=val.pos, .weight=1};
#else
        const Target target = (Target) {.pos=val.pos, .weight=0};
         // weight=0 cuz we don't actually care about the value ;)
#endif

        const int status = compile_expression(state, node, target);
        BF_free(state, val);
        return status;

}
static int compileBlock(compiler_info *const state, const Node* node) {
        const uintptr_t len = node->operands[0].len;
        pushNamespace(state);
        for (uintptr_t i=0; i<len; i++) if (!_compile_statement(state, node->operands[i+1].nd)) return 0;
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


// ------------------------ statement handlers -------------------------------
// ------------------------ expression handlers -------------------------------

static int compile_literal_int(compiler_info *const state, const Node* node, const Target target) {
        seekpos(state, target.pos);
        emitPlus(state, atoi(node->token.tok.source)*target.weight);
        return 1;
}
static int compile_variable(compiler_info *const state, const Node* node, const Target target) {
        const Variable v = getVariable(state, node->token.tok.source);
        Value temp = BF_allocate(state, v.val.type);
        const Target targets[] = {
                target,
                {.pos=temp.pos, .weight=1}
        };
        transfer(state, v.val.pos, sizeof(targets)/sizeof(*targets), targets);
        transfer(state, temp.pos, 1, &((Target){.pos=v.val.pos, .weight=1}));
        BF_free(state, temp);
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
static int compile_affect(compiler_info *const state, const Node* node, const Target target) {
        const Variable v = getVariable(state, node->operands[0].nd->token.tok.source);
        if (target.weight == 0) {
                // shortcut, if we don't need the result we can write it directly in place.
                return compile_expression(state, node->operands[1].nd, (Target) {.pos=v.val.pos, .weight=1});
        }

        const Value temp = BF_allocate(state, v.val.type);
        if (!compile_expression(state, node->operands[1].nd, (Target) {.pos=temp.pos, .weight=1})) {
                BF_free(state, temp);
                return 0;
        }
        const Target tgts[] = {
                target,
                {.pos=v.val.pos, .weight=1}
        };
        reset(state, v.val.pos);
        transfer(state, temp.pos, sizeof(tgts)/sizeof(*tgts), tgts);
        BF_free(state, temp);
        return 1;

        // can't do that because the variable might be used to compute the lhs.
        /*
        reset(state, v->val.pos);
        return compile_expression(state, node->operands[1].nd, (Target){.pos=v->val.pos, .weight=1});*/
}
static int compile_call(compiler_info *const state, const Node* node, const Target target) {
        BuiltinFunctionHandler called = getVariable(state, node->operands[1].nd->token.tok.source).func->handler;

        Value arguments[node->operands[0].len-1];

        // node structure :
        // [len, funcname, args…]

        for (size_t i = 0; i < sizeof(arguments)/sizeof(*arguments); i++) {
                arguments[i] = BF_allocate(state, node->operands[i+2].nd->type);
                if (!compile_expression(state, node->operands[i+2].nd, (Target) {.pos=arguments[i].pos, .weight=1})) {
                        return 0;
                }
        }

        const int status = called(state, arguments, target);

        for (size_t i = 0; i < sizeof(arguments)/sizeof(*arguments); i++) {
                BF_free(state, arguments[i]);
        }

        return status;

}
static int compile_multiply(compiler_info *const state, const Node* node, const Target target) {
        const Node* opA = node->operands[0].nd;
        const Node* opB = node->operands[1].nd;
        if ((opA->type != TYPE_INT) || (opB->type != TYPE_INT)) {
                TypeError(node->token);
                return 0;
        }


        if (opA->operator == OP_INT) {
                if (opB->operator == OP_INT) {
                        // easy case: multiply an interger literal with another
                        // we can compute that at compile-time
                        seekpos(state, target.pos);
                        emitPlus(state,
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
                        Value xval = BF_allocate(state, opA->type);
                        Value yval = BF_allocate(state, opB->type);
                        if ( // computing operands
                        !compile_expression(state, opB, (Target) {.pos=yval.pos, .weight=1}) ||
                        !compile_expression(state, opA, (Target) {.pos=xval.pos, .weight=1})
                        ) {
                                BF_free(state, xval);
                                BF_free(state, yval);
                                return 0;
                        }
                        Value yclone = BF_allocate(state, yval.type);

                        seekpos(state, xval.pos);
                        const size_t mainloop = openJump(state);
                        emitMinus(state, 1); // loop: do <X> times:

                        const Target forth[] = {
                                target,
                                {.pos=yclone.pos, .weight=1},
                        }; // transfer Y onto these
                        const Target back[] = {
                                {.pos=yval.pos, .weight=1},
                        }; // then transfer Y' back on Y
                        transfer(state, yval.pos, sizeof(forth)/sizeof(*forth), forth);
                        transfer(state, yclone.pos, sizeof(back)/sizeof(*back), back);

                        seekpos(state, xval.pos); // the loop is on X ; don't forget to jump back there before closing the jump!
                        closeJump(state, mainloop);

                        BF_free(state, xval);
                        BF_free(state, yval);
                        BF_free(state, yclone);
                        return 1;
                }
        }
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
                [OP_AFFECT] = compile_affect,
                [OP_CALL] = compile_call,
                [OP_PRODUCT] = compile_multiply,
        };

        const ExprCompilationHandler handler = handlers[node->operator];
        if (handler == NULL) {
                LOG("Operand not (yet) implemented");
                SyntaxError(node->token);
                return 0;
        }

        return handler(state, node, target);
}

static int _compile_statement(compiler_info *const state, const Node* node) {
        static const StmtCompilationHandler handlers[LEN_OPERATORS] = {
                [OP_NOP] = compileNop,
                [OP_BLOCK] = compileBlock,
                [OP_DECLARE] = compileDeclaration,
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
