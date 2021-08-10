#ifndef compiler_h
#define compiler_h

#include <stdio.h>
#include <stddef.h>

#include "compiler/parser.h"

// CompiledProgram defined in bytecode.h
// BFMemoryView defined in mm.h
// BFNamespace defined in compiler_helpers.h

typedef struct compiler_info compiler_info;

struct compiler_info {
        struct parser_info prsinfo;
        struct CompiledProgram* program;
        struct BFMemoryView* memstate;
        struct BFNamespace* currentNS;
        size_t current_pos; // not necessarily enough (var len str)
        unsigned int code_isnonlinear;
};

void mk_compiler_info(compiler_info *const cmpinfo);
void del_compiler_info(compiler_info *const cmpinfo);

int compile_statement(compiler_info *const state);

#endif
