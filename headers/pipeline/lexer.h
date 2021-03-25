#ifndef lexer_h
#define lexer_h

#include <stdio.h>

#include "headers/pipeline/token.h"
#include "headers/pipeline/state.h"

void mk_lexer_info(lexer_info *const lxinfo, FILE* file);
void del_lexer_info(lexer_info *const lxinfo);

Token lex(lexer_info *const state);

#endif
