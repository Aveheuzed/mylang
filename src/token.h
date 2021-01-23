#ifndef token_h
#define token_h

typedef enum {
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_STAR,
        TOKEN_SLASH,
        TOKEN_POPEN,
        TOKEN_PCLOSE,

        TOKEN_EQUAL,
        TOKEN_SEMICOLON,

        TOKEN_IDENTIFIER,
        TOKEN_INT,
        TOKEN_FLOAT,
        TOKEN_STR,
        TOKEN_TRUE,
        TOKEN_FALSE,

        TOKEN_ERROR,
        TOKEN_EOF,
} TokenType;

typedef struct Token {
        char* source;
        unsigned int line;
        unsigned short column;
        unsigned short length;
        TokenType type;
} Token;

#endif
