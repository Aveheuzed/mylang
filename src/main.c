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
        unsigned int nb_stmt = 0;
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
                do {
                        roots[nb_stmt] = parse(&current_token, PREC_NONE);
                        if (roots[nb_stmt] != NULL) {
                                if (roots[nb_stmt]->operator != OP_PARSE_ERROR) {
                                        nb_stmt++;
                                        if (current_token->type != TOKEN_EOF) {
                                                continue;
                                        }
                                        else {
                                                break;
                                        }
                                }
                                else {
                                        break;
                                }
                        }
                        else {
                                continue;
                        }
                } while (1);
        }
        printf("%d statements.\n", nb_stmt);
        puts("Parsing done.");
        for (unsigned int istmt=0; istmt<nb_stmt; istmt++) {
                print_value(interpret(roots[istmt]));
        }
        puts("All done !");
        for (unsigned int istmt=0; istmt<nb_stmt; istmt++) {
                freeNode(roots[istmt]);
        }
}
