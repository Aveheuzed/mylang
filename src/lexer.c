#include <string.h>

#include "token.h"
#include "lexer.h"

void detect_keywords(Token *const token) {
        const static struct {char* source; TokenType type;} keywords[] = {
                {"true", TOKEN_TRUE},
                {"false", TOKEN_FALSE},

                {NULL, 0}
        };

        if (token->type != TOKEN_IDENTIFIER) return;

        for (unsigned int i=0; keywords[i].source != NULL; i++) {
                if (!strncmp(token->source, keywords[i].source, token->length)) {
                        token->type = keywords[i].type;
                        break;
                }
        }

}

char* _lex(char *source, Token *const token) {
        #define CURRENT_CHAR() (*source)
        #define ADVANCE() ((*++source=='\n')?(line++, column=0):(column++))
        #define ISDIGIT() (CURRENT_CHAR() >= '0' && CURRENT_CHAR() <= '9')
        #define ISLETTER() ((CURRENT_CHAR() >= 'a' && CURRENT_CHAR() <= 'z') || (CURRENT_CHAR() >= 'A' && CURRENT_CHAR() <= 'Z'))
        #define ISBLANK() (CURRENT_CHAR() == '\n' || CURRENT_CHAR() == '\t' || CURRENT_CHAR() == '\r' || CURRENT_CHAR() == ' ')

        static unsigned int line = 1;
        static unsigned short column = 0;
        unsigned short length = 0;

        while (ISBLANK()) ADVANCE();

        token->source = source;
        token->line = line;
        token->column = column;
        token->length = 0;

        if (CURRENT_CHAR() == '\0') {
                token->type = TOKEN_EOF;
                return source;
        }

         switch(CURRENT_CHAR()) {
                case '+': ADVANCE(), token->type = TOKEN_PLUS;  break;
                case '-': ADVANCE(), token->type = TOKEN_MINUS; break;
                case '*': ADVANCE(), token->type = TOKEN_STAR; break;
                case '/': ADVANCE(), token->type = TOKEN_SLASH; break;
                case '(': ADVANCE(), token->type = TOKEN_POPEN; break;
                case ')': ADVANCE(), token->type = TOKEN_PCLOSE; break;
                case '=': ADVANCE(), token->type = TOKEN_EQUAL; break;
                case ';': ADVANCE(), token->type = TOKEN_SEMICOLON; break;

                default:
                        if (ISDIGIT()) {
                                token->type = TOKEN_INT;
                                do ADVANCE(); while (ISDIGIT());
                                if (CURRENT_CHAR() == '.') {
                                        token->type = TOKEN_FLOAT;
                                        do ADVANCE(); while (ISDIGIT());
                                }
                                break;
                        }
                        else if (CURRENT_CHAR() == '_' || ISLETTER() || ISDIGIT()) {
                                token->type = TOKEN_IDENTIFIER;
                                do ADVANCE(); while (CURRENT_CHAR() == '_' || ISLETTER() || ISDIGIT());
                                break;
                        }
                        else if (CURRENT_CHAR() == '"') {
                                token->type = TOKEN_STR;
                                do ADVANCE(); while (CURRENT_CHAR() != '"' && CURRENT_CHAR() != '\0');
                                if (CURRENT_CHAR() == '"') ADVANCE();
                                else token->type = TOKEN_ERROR;
                                break;
                        }
                        else token->type = TOKEN_ERROR;
                        break;
        }

        token->length = source - token->source;
        return source;

        #undef ISBLANK
        #undef ISLETTER
        #undef ISDIGIT
        #undef GETCHAR
        #undef ADVANCE
}

char* lex(char* source, Token *const token) {
        source = _lex(source, token);
        // TODO: intern strings + identifiers
        detect_keywords(token);
        return source;
}
