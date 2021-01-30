#ifndef lexer_h
#define lexer_h

#include "token.h"
#include "identifiers_record.h"

char* lex(char *source, Token *const token, IdentifiersRecord **const record);

#endif
