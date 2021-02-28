#ifndef compiler_h
#define compiler_h

#include <stdio.h>

#include "headers/pipeline/parser.h"
#include "headers/pipeline/bytecode.h"

typedef struct compiler_info {
        parser_info prsinfo;
        CompiledProgram* program;
} compiler_info;

compiler_info mk_compiler_info(FILE* file);
void del_compiler_info(compiler_info cmpinfo);

int compile_statement(compiler_info *const state);


#endif
