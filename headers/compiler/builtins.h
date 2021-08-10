#ifndef builtins_h
#define builtins_h

#include <stddef.h>

#include "compiler/runtime_types.h"
#include "compiler/compiler.h"
#include "compiler/node.h"
#include "compiler/compiler_helpers.h"

typedef int (*BuiltinFunctionHandler)(struct compiler_info *const state, struct Node *const argv[], const struct Target target);

typedef struct BuiltinFunction {
        BuiltinFunctionHandler handler;
        char* name;
        size_t arity;
        RuntimeType returnType;
} BuiltinFunction;

extern BuiltinFunction builtins[];
extern const size_t nb_builtins;

#endif
