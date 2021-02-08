#ifndef lexer_h
#define lexer_h

#include "headers/pipeline/token.h"

void init_lexing(void);
void end_lexing(void); // not to be called until tokens are not needed any more
char* lex(char *source, Token *const token);

#endif
