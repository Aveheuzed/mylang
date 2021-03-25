#include <stdio.h>
#include <stddef.h>

#include "headers/pipeline/state.h"

#include "headers/pipeline/lexer.h"
#include "headers/pipeline/parser.h"
#include "headers/pipeline/compiler.h"

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
