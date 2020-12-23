#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "token.h"

void print_token(const Token* token) {
        printf("\"%.*s\" : ln %u, col %u, len %u. Type : %d\n", token->length, token->source, token->line, token->column, token->length, token->type);
}

void main() {
        Token token;
        char* const buffer_start = malloc(1024);
        char* line;
        do {
                line = fgets(buffer_start, 1024, stdin);
                if (line == NULL) break;
                do {
                        line = lex(line, &token);
                        print_token(&token);
                } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
        } while (strlen(buffer_start));
        free(buffer_start);
}
