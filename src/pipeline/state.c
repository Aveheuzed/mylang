#include <stdio.h>
#include <stddef.h>

#include "pipeline/state.h"

#include "pipeline/lexer.h"
#include "pipeline/parser.h"
#include "pipeline/compiler.h"

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
