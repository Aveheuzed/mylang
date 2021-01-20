#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "builtins.h"

void print_token(const Token* token) {
        printf("\"%.*s\" : ln %u, col %u, len %u. Type : %d\n", token->length, token->source, token->line, token->column, token->length, token->type);
}

void main() {
        char* line = NULL;
        {
                size_t len = 0;
                getline(&line, &len, stdin);
        }
        Token tokens[128];
        Node* roots[128];
        puts("Starting…");
        {
                char* current_char = line;
                Token* current_token = tokens;
                do {
                        current_char = lex(current_char, current_token++);
                } while (current_token[-1].type != TOKEN_EOF && current_token[-1].type != TOKEN_ERROR);
                if (current_token[-1].type == TOKEN_ERROR) {
                        printf("Lexing error.\n");
                        exit(-1);
                }
                else {
                        for (Token* tk = tokens; tk<current_token; tk++) {
                                print_token(tk);
                        }
                }
        }
        puts("Lexing done.");
        {
                Token* current_token = tokens;
                Node** current_root = roots - 1;
                do {
                        *++current_root = parse(&current_token, PREC_NONE);
                } while (*current_root != NULL);
        }
        puts("Parsing done.");
        for (Node** current_root=roots; *current_root != NULL; current_root++) {
                print_value(interpret(*current_root));
        }
        puts("All done !");
        for (Node** current_root=roots; *current_root != NULL; current_root++) {
                freeNode(*current_root);
        }
}
