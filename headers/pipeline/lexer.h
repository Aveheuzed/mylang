#ifndef lexer_h
#define lexer_h

#include <stdio.h>

#include "headers/pipeline/token.h"
#include "headers/utils/identifiers_record.h"

typedef struct lexer_info {
        FILE* file;
        unsigned int line;
        unsigned int column;
} lexer_info;

lexer_info mk_lexer_info(FILE* file);

IdentifiersRecord** init_lexing(void);
void end_lexing(void); // not to be called until tokens are not needed any more
Token lex(lexer_info *const state);

#endif
