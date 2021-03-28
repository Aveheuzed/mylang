#include <stdio.h>
#include <stdlib.h>

#include "utils/builtins.h"
#include "utils/mm.h"
#include "utils/compiler_helpers.h"
#include "utils/runtime_types.h"


int builtin_print(compiler_info *const state, const Value argv[], const Target target) {
        switch (argv[0].type) {
                case TYPE_INT:
                        seekpos(state, argv[0].pos);
                        emitOutput(state);
                        return 1;
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
