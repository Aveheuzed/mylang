#ifndef builtins_h
#define builtins_h

#include <stddef.h>

#include "pipeline/state.h"
#include "utils/compiler_helpers.h"
#include "pipeline/node.h"
#include "utils/runtime_types.h"

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
