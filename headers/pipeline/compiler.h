#ifndef compiler_h
#define compiler_h

#include <stdio.h>
#include <stddef.h>

#include "headers/pipeline/state.h"
#include "headers/pipeline/bytecode.h"

void mk_compiler_info(compiler_info *const cmpinfo);
void del_compiler_info(compiler_info *const cmpinfo);

int compile_statement(compiler_info *const state);

CompiledProgram* get_bytecode(pipeline_state *const state);


#endif
