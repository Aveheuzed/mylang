#ifndef lexer_h
#define lexer_h

#include <stdio.h>

#include "pipeline/token.h"
#include "utils/identifiers_record.h"

typedef struct lexer_info {
        FILE* file;
        IdentifiersRecord* record;
        Localization pos;
} lexer_info;

lexer_info mk_lexer_info(FILE* file);

void del_lexer_info(lexer_info lxinfo);

LocalizedToken lex(lexer_info *const state);

#endif
