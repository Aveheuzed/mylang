#ifndef compiler_helpers
#define compiler_helpers

#include <stddef.h>

#include "headers/pipeline/compiler.h"
#include "headers/utils/runtime_types.h"
#include "headers/utils/mm.h"


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

CompiledProgram* createProgram(void);
void freeProgram(CompiledProgram* ptr);

void pushNamespace(compiler_info *const state);
void popNamespace(compiler_info *const state);
Variable getVariable(compiler_info *const state, char const* name);

// returns zero on failure
int addVariable(compiler_info *const state, Variable v);


void emitPlus(compiler_info *const state, const size_t amount);
void emitMinus(compiler_info *const state, const size_t amount);
void emitLeft(compiler_info *const state, const size_t amount);
void emitRight(compiler_info *const state, const size_t amount);
void emitInput(compiler_info *const state);
void emitOutput(compiler_info *const state);

size_t openJump(compiler_info *const state) __attribute__ ((warn_unused_result));
void closeJump(compiler_info *const state, const size_t jump_pos);


void seekpos(compiler_info *const state, const size_t i);
void transfer(compiler_info *const state, const size_t pos, const int nb_targets, const Target targets[]);
void reset(compiler_info *const state, const size_t i);

#endif
