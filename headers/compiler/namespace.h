#ifndef namespace_h
#define namespace_h

#include <stddef.h>

#include "compiler/compiler.h"
#include "compiler/compiler_helpers.h"

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

void pushNamespace(compiler_info *const state);
void popNamespace(compiler_info *const state);
Variable* getVariable(compiler_info *const state, char const* name);

// returns zero on failure
int addVariable(compiler_info *const state, Variable v);

#endif /* end of include guard: namespace_h */
