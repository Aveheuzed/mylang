#ifndef builtins_h
#define builtins_h

#include <stddef.h>

#include "headers/pipeline/compiler.h"
#include "headers/utils/mm.h"
#include "headers/utils/compiler_helpers.h"
#include "headers/utils/runtime_types.h"

typedef int (*BuiltinFunctionHandler)(compiler_info *const state, const Value argv[], const Target target);

typedef struct BuiltinFunction {
        BuiltinFunctionHandler handler;
        char* name;
        size_t arity;
        RuntimeType returnType;
} BuiltinFunction;

extern BuiltinFunction builtins[];
extern const size_t nb_builtins;

#endif
