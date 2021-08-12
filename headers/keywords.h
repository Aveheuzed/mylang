#ifndef keywords_h
#define keywords_h

#include "token.h"

typedef struct Keyword Keyword;
struct Keyword {
        char* source;
        TokenType type;
};

#endif /* end of include guard: keywords_h */
