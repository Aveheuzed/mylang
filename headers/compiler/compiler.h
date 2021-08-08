#ifndef compiler_h
#define compiler_h

#include <stdio.h>
#include <stddef.h>

#include "compiler/state.h"

struct CompiledProgram;

void mk_compiler_info(compiler_info *const cmpinfo);
void del_compiler_info(compiler_info *const cmpinfo);

int compile_statement(compiler_info *const state);

struct CompiledProgram* get_bytecode(pipeline_state *const state);


#endif
