#ifndef compiler_helpers
#define compiler_helpers

#include <stddef.h>

#include "compiler/compiler.h"
#include "compiler/runtime_types.h"
#include "compiler/mm.h"


typedef struct Target {
        size_t pos;
        char weight;
} Target;

typedef struct Variable {
        char* name;
        union {
                Value val;
                struct BuiltinFunction* func; // defined in builtins.h, but *circular includes*…
        };
} Variable;

typedef struct BFNamespace {
        struct BFNamespace* enclosing;
        size_t allocated, len; // `allocated` guaranteed to be a power of 2
        Variable dict[];
} BFNamespace;

#define EMIT_PLUS(state, amount) (state->program = emitPlusMinus(state->program, amount))
#define EMIT_MINUS(state, amount) (state->program = emitPlusMinus(state->program, -amount))
#define EMIT_LEFT(state, amount) (state->program = emitLeftRight(state->program, -amount))
#define EMIT_RIGHT(state, amount) (state->program = emitLeftRight(state->program, amount))
#define OPEN_JUMP(state) (state->program = emitOpeningBracket(state->program))
#define CLOSE_JUMP(state) (state->program = emitClosingBracket(state->program))
#define EMIT_INPUT(state) (state->program = emitIn(state->program))
#define EMIT_OUTPUT(state) (state->program = emitOut(state->program))

void pushNamespace(compiler_info *const state);
void popNamespace(compiler_info *const state);
Variable* getVariable(compiler_info *const state, char const* name);

// returns zero on failure
int addVariable(compiler_info *const state, Variable v);

void seekpos(compiler_info *const state, const size_t i);
void transfer(compiler_info *const state, const size_t pos, const int nb_targets, const Target targets[]);
void reset(compiler_info *const state, const size_t i);
void runtime_mul_int(compiler_info *const state, const Target target, const size_t xpos, const size_t ypos);

#endif
