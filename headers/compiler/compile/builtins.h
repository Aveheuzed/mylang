#ifndef builtins_h
#define builtins_h

#include <stddef.h>

#include "compiler/state.h"
#include "compiler/compile/compiler_helpers.h"
#include "compiler/node.h"
#include "compiler/runtime_types.h"

typedef int (*BuiltinFunctionHandler)(compiler_info *const state, struct Node *const argv[], const Target target);

typedef struct BuiltinFunction {
        BuiltinFunctionHandler handler;
        char* name;
        size_t arity;
        RuntimeType returnType;
} BuiltinFunction;

extern BuiltinFunction builtins[];
extern const size_t nb_builtins;

#endif
