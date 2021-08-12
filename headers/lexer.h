#ifndef lexer_h
#define lexer_h

#include <stdio.h>

#include "token.h"
#include "identifiers_record.h"
#include "keywords.h"

typedef struct lexer_info {
        const Keyword* keywords;
        FILE* file;
        IdentifiersRecord* record;
        Localization pos;
} lexer_info;

void mk_lexer_info(lexer_info *const lxinfo, FILE* file, const Keyword* keywords);
void del_lexer_info(lexer_info *const lxinfo);

LocalizedToken lex(lexer_info *const state);

#endif
