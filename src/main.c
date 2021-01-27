#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

void print_token(const Token* token) {
        printf("\"%.*s\" : ln %u, col %u, len %u. Type : %d\n", token->length, token->source, token->line, token->column, token->length, token->type);
}

void main() {
        char* line = NULL;
        { size_t len = 0;
                getline(&line, &len, stdin);
        }
        Token *tokens = calloc(128, sizeof(Token));
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
        Node* root;
        {
                Token* current_token = tokens;
                root = parse(&current_token, PREC_NONE);
        }
        puts("Parsing done.");
        printf("%ld\n", interpret(root));
        puts("All done !");
        free(tokens);
        free(line);

}
