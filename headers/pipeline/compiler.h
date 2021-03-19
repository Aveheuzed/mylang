#ifndef compiler_h
#define compiler_h

#include <stdio.h>
#include <stddef.h>

#include "headers/pipeline/parser.h"
#include "headers/pipeline/bytecode.h"

typedef struct compiler_info {
        parser_info prsinfo;
        CompiledProgram* program;
        struct BFMemoryView* memstate; // defined in mm.h, but we can't include it because of circular dependencies.
        struct BFNamespace* currentNS; // idem, defined in compiler_helpers.h
        size_t current_pos; // not necessarily enough (var len str)
} compiler_info;

compiler_info mk_compiler_info(FILE* file);
void del_compiler_info(compiler_info cmpinfo);

int compile_statement(compiler_info *const state);


#endif
