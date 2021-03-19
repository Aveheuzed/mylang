#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "headers/pipeline/node.h"
#include "headers/pipeline/compiler.h"
#include "headers/utils/compiler_helpers.h"
#include "headers/utils/mm.h"
#include "headers/utils/error.h"


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
        return c;
}
void del_compiler_info(compiler_info cmpinfo) {
        while (cmpinfo.currentNS != NULL) {
                /* we could also have repeatedly popped, but this is faster
                since we don't have to free the variables in the memory view. */
                BFNamespace* next = cmpinfo.currentNS->enclosing;
                free(cmpinfo.currentNS);
                cmpinfo.currentNS = next;
        }
        freeProgram(cmpinfo.program);
        freeMemoryView(cmpinfo.memstate);
}

// ------------------------ compilation handlers -------------------------------

static int compileNop(compiler_info *const state, const Node* node) {
        return 1;
}
static int compileExpressionStmt(compiler_info *const state, const Node* node) {
        const Value val = BF_allocate(state, node->type);
        const Target target = (Target) {.pos = val.pos, .weight=0};
         // weight=0 cuz we don't actually care about the value ;)

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

// ------------------------ end compilation handlers ---------------------------

static int compile_expression(compiler_info *const state, const Node* node, const Target target) {
        static const ExprCompilationHandler handlers[LEN_OPERATORS] = {
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
