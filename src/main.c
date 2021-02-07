#include <stdlib.h>
#include <stdio.h>

#include "headers/pipeline/lexer.h"
#include "headers/pipeline/parser.h"
#include "headers/pipeline/interpreter.h"
#include "headers/utils/builtins.h"

void print_token(const Token* token) {
        printf("\"%.*s\" : ln %u, col %u, len %u. Type : %d\n", token->length, token->source, token->line, token->column, token->length, token->type);
}

void _main() {
        char* line = NULL;
        {
                size_t len = 0;
                getline(&line, &len, stdin);
        }
        Token tokens[128];
        Node* roots[128];
        unsigned int nb_stmt = 0;
        {
                IdentifiersRecord* record = allocateRecord();
                char* current_char = line;
                Token* current_token = tokens;
                do {
                        current_char = lex(current_char, current_token++, &record);
                } while (current_token[-1].type != TOKEN_EOF && current_token[-1].type != TOKEN_ERROR);
                if (current_token[-1].type == TOKEN_ERROR) {
                        printf("Lexing error.\n");
                        exit(-1);
                }
                freeRecord(record);
        }
        {
                Token* current_token = tokens;

                do {
                        roots[nb_stmt] = parse_statement(&current_token);
                } while (roots[nb_stmt++] != NULL);
                nb_stmt--;
        }

        Namespace* ns = allocateNamespace();
        for (unsigned int istmt=0; istmt<nb_stmt; istmt++) {
                interpretStatement(roots[istmt], &ns);
        }
        freeNamespace(ns);


        for (unsigned int istmt=0; istmt<nb_stmt; istmt++) {
                freeNode(roots[istmt]);
        }
}


void main() {
        while (1) _main();
}
