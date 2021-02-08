#ifndef lexer_h
#define lexer_h

#include "headers/utils/identifiers_record.h"
#include "headers/pipeline/token.h"


char* lex(char *source, Token *const token, IdentifiersRecord **const record);

#endif
