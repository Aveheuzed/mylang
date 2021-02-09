#ifndef token_h
#define token_h

typedef enum {
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_STAR,
        TOKEN_SLASH,
        TOKEN_AND,
        TOKEN_OR,
        TOKEN_EQ, // "==": comparison
        TOKEN_NE,
        TOKEN_GE,
        TOKEN_LE,
        TOKEN_GT,
        TOKEN_LT,
        TOKEN_POPEN,
        TOKEN_PCLOSE,
        TOKEN_BOPEN,
        TOKEN_BCLOSE,

        TOKEN_EQUAL, // "=" : affectation
        TOKEN_SEMICOLON,
        TOKEN_NOT,

        TOKEN_IDENTIFIER,
        TOKEN_INT,
        TOKEN_FLOAT,
        TOKEN_STR,
        TOKEN_TRUE,
        TOKEN_FALSE,
        TOKEN_NONE,

        TOKEN_IF,
        TOKEN_ELSE,

        TOKEN_ERROR,
        TOKEN_EOF, // please don't put new token types below here
} TokenType;

typedef struct Token {
        char* source;
        unsigned int line;
        unsigned int column;
        unsigned int length;
        TokenType type;
} Token;

#endif