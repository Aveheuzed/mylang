#include <stdio.h>
#include <stddef.h>

#include "compiler/pipeline.h"

#include "lexer.h"
#include "compiler/parser.h"
#include "compiler/compiler.h"
#include "compiler/bytecode.h"

void mk_pipeline(pipeline_state *const state, FILE* file) {
        mk_lexer_info(&(state->cmpinfo.prsinfo.lxinfo), file);
        mk_parser_info(&(state->cmpinfo.prsinfo));
        mk_compiler_info(&(state->cmpinfo));
}

void del_pipeline(pipeline_state *const state) {
        del_lexer_info(&(state->cmpinfo.prsinfo.lxinfo));
        del_parser_info(&(state->cmpinfo.prsinfo));
        del_compiler_info(&(state->cmpinfo));
}

CompiledProgram* get_bytecode(pipeline_state *const state) {
        CompiledProgram *const pgm = state->cmpinfo.program;
        state->cmpinfo.program = NULL;
        return emitEnd(pgm);
}
