#include <string.h>
#include <stdlib.h>

#include "interpreter/lexer.h"
#include "interpreter/identifiers_record.h"
#include "interpreter/error.h"

inline lexer_info mk_lexer_info(FILE* file) {
        lexer_info lxinfo = (lexer_info) {.file=file, .pos.line=1, .pos.column=1};
        lxinfo.record = allocateRecord();
        return lxinfo;
}
inline void del_lexer_info(lexer_info lxinfo) {
        fclose(lxinfo.file);
        freeRecord(lxinfo.record);
}

/*
Note on the content of the token (source) :
Its value is allocated through malloc() in _lex, then the "ownership" is
transferred to the static record in intern_token => internalize.
Therefore, internalize needn't `strndup` it, but it should `free` it if it
deduplicates it.
*/

static inline void intern_token(lexer_info *const lxinfo, Token *const token) {
        if (token->length)
        token->source = internalize(&(lxinfo->record), token->source, token->length);
}

static inline void detect_keywords(Token *const token) {
        static const struct {char* source; TokenType type;} keywords[] = {
                {"true", TOKEN_TRUE},
                {"false", TOKEN_FALSE},
                {"none", TOKEN_NONE},
                {"if", TOKEN_IF},
                {"else", TOKEN_ELSE},
                {"while", TOKEN_WHILE},
                {"function", TOKEN_FUNC},
                {"return", TOKEN_RETURN},

                {NULL, 0}
        };

        for (unsigned int i=0; keywords[i].source != NULL; i++) {
                if (
                        strlen(keywords[i].source) == token->length &&
                        !strncmp(token->source, keywords[i].source, token->length)
                ) {
                        token->type = keywords[i].type;
                        break;
                }
        }

}

struct token_buffer {
        char* source;
        size_t bufsize;
        size_t taken;
};
static void commit(struct token_buffer *const buffer, char c) {
        if (buffer->taken >= buffer->bufsize) {
                buffer->bufsize = buffer->bufsize ? (buffer->bufsize*2) : 1;
                buffer->source = reallocarray(buffer->source, buffer->bufsize, sizeof(char));
        }
        buffer->source[buffer->taken++] = c;
}
static char pull_char(lexer_info *const state) {
        const char c = getc(state->file);
        if (c == '\n') {
                state->pos.line++;
                state->pos.column = 1;
        } else {
                state->pos.column++;
        }
        return c;
}
static char peek(lexer_info *const state) {
        char c = getc(state->file);
        ungetc(c, state->file);
        return c;
}

Token _lex(lexer_info *const state) {
        #define PEEK() (peek(state))
        #define COMMIT(c) (commit(&buffer, c))
        #define EQUAL_FOLLOWS(then_, else_) (\
                COMMIT(staging),\
                (PEEK() == '=')?\
                (\
                        COMMIT(pull_char(state)),\
                        then_\
                ):\
                else_\
        )
        #define REQUIRES(chr, ttype) (\
                COMMIT(staging),\
                (PEEK() == chr)?\
                (\
                        COMMIT(staging),\
                        COMMIT(pull_char(state)), \
                        ttype\
                ):\
                TOKEN_ERROR\
        )

        #define ISDIGIT(c) (c >= '0' && c <= '9')
        #define ISLETTER(c) (\
                (c >= 'a' && c <= 'z')\
                || (c >= 'A' && c <= 'Z')\
        )
        #define ISBLANK(c) (\
                c == '\n'\
                || c == '\t'\
                || c == '\r'\
                || c == ' '\
        )

        Token target;
        struct token_buffer buffer = (struct token_buffer) {.source = malloc(0), .bufsize=0, .taken=0};
        char staging;

        while (ISBLANK(PEEK())) pull_char(state);


        switch(staging = pull_char(state)) {
                case EOF: target.type = TOKEN_EOF; break;

                case '(': COMMIT(staging); target.type = TOKEN_POPEN; break;
                case ')': COMMIT(staging); target.type = TOKEN_PCLOSE; break;
                case ';': COMMIT(staging); target.type = TOKEN_SEMICOLON; break;
                case ',': COMMIT(staging); target.type = TOKEN_COMMA; break;
                case '{': COMMIT(staging); target.type = TOKEN_BOPEN; break;
                case '}': COMMIT(staging); target.type = TOKEN_BCLOSE; break;

                case '+':
                        target.type = EQUAL_FOLLOWS(TOKEN_IADD, TOKEN_PLUS);
                        break;
                case '-':
                        target.type = EQUAL_FOLLOWS(TOKEN_ISUB, TOKEN_MINUS);
                        break;
                case '*':
                        target.type = EQUAL_FOLLOWS(TOKEN_IMUL, TOKEN_STAR);
                        break;
                case '/':
                        target.type = EQUAL_FOLLOWS(TOKEN_IDIV, TOKEN_SLASH);
                        break;

                case '=':
                        target.type = EQUAL_FOLLOWS(TOKEN_EQ, TOKEN_EQUAL);
                        break;
                case '!':
                        target.type = EQUAL_FOLLOWS(TOKEN_NE, TOKEN_NOT);
                        break;
                case '>':
                        target.type = EQUAL_FOLLOWS(TOKEN_GE, TOKEN_GT);
                        break;
                case '<':
                        target.type = EQUAL_FOLLOWS(TOKEN_LE, TOKEN_LT);
                        break;

                case '&':
                        target.type = REQUIRES('&', TOKEN_AND);
                        break;
                case '|':
                        target.type = REQUIRES('|', TOKEN_OR);
                        break;

                default:
                        if (ISDIGIT(staging)) {
                                COMMIT(staging);
                                target.type = TOKEN_INT;
                                while (ISDIGIT(PEEK())) {
                                        COMMIT(pull_char(state));
                                }
                                if (PEEK() == '.') {
                                        COMMIT(pull_char(state));
                                        target.type = TOKEN_FLOAT;
                                        while (ISDIGIT(PEEK())) {
                                                COMMIT(pull_char(state));
                                        }
                                }
                                break;
                        }
                        else if (staging == '_' || ISLETTER(staging)) {
                                COMMIT(staging);
                                target.type = TOKEN_IDENTIFIER;
                                while (staging = PEEK(),
                                        staging == '_' || ISLETTER(staging) || ISDIGIT(staging)
                                ) {
                                        COMMIT(pull_char(state));
                                }
                                break;
                        }
                        else if (staging == '"') {
                                COMMIT(staging);
                                target.type = TOKEN_STR;
                                while (staging = PEEK(),
                                        staging != '"' && staging != EOF
                                ) {
                                        COMMIT(pull_char(state));
                                }
                                if (staging == '"') COMMIT(pull_char(state));
                                else target.type = TOKEN_ERROR;
                                break;
                        }
                        else {
                                COMMIT(staging);
                                target.type = TOKEN_ERROR;
                        }
                        break;
        }

        target.length = buffer.taken;
        target.source = buffer.source;

        return target;

        #undef PEEK
        #undef COMMIT
        #undef EQUAL_FOLLOWS
        #undef REQUIRES
        #undef ISDIGIT
        #undef ISLETTER
        #undef ISBLANK
}

LocalizedToken lex(lexer_info *const state) {
        LocalizedToken tok;
        tok.pos = state->pos;
        tok.tok = _lex(state);
        while (tok.tok.type == TOKEN_ERROR) {
                Error(&tok, "Syntax error: unrecognized token.\n");
                tok.tok = _lex(state);
        }
        intern_token(state, &(tok.tok));
        if (tok.tok.type == TOKEN_IDENTIFIER) {
                detect_keywords(&(tok.tok));
        }

        LOG("Producing type-%.2d token: `%.*s`. (line %u[%u:%u])", tok.tok.type, tok.tok.length, tok.tok.source, tok.pos.line, tok.pos.column, tok.pos.column+tok.tok.length-1);

        return tok;
}