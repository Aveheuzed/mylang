#include <stdio.h>
#include <stddef.h>

#include "interpreter/pipeline.h"

#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include "interpreter/interpreter.h"

void mk_pipeline(pipeline_state *const state, FILE* file) {
        mk_lexer_info(&(state->interpinfo.prsinfo.lxinfo), file);
        mk_parser_info(&(state->interpinfo.prsinfo));
        mk_interpreter_info(&(state->interpinfo));
}

void del_pipeline(pipeline_state *const state) {
        del_lexer_info(&(state->interpinfo.prsinfo.lxinfo));
        del_parser_info(&(state->interpinfo.prsinfo));
        del_interpreter_info(&(state->interpinfo));
}
