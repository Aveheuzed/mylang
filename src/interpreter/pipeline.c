#include <stdio.h>
#include <stddef.h>

#include "interpreter/pipeline.h"

#include "lexer.h"
#include "interpreter/parser.h"
#include "interpreter/interpreter.h"
#include "keywords.h"

void mk_pipeline(pipeline_state *const state, FILE* file, const Keyword* keywords) {
        mk_lexer_info(&(state->interpinfo.prsinfo.lxinfo), file, keywords);
        mk_parser_info(&(state->interpinfo.prsinfo));
        mk_interpreter_info(&(state->interpinfo));
}

void del_pipeline(pipeline_state *const state) {
        del_lexer_info(&(state->interpinfo.prsinfo.lxinfo));
        del_parser_info(&(state->interpinfo.prsinfo));
        del_interpreter_info(&(state->interpinfo));
}
