#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include "headers/pipeline/lexer.h"
#include "headers/pipeline/parser.h"
#include "headers/pipeline/interpreter.h"

#define TOKENS_START_NUMBER 16

static int repl() {
        char* source = NULL;

        size_t nb_tokens = 0;
        size_t len_tokens = TOKENS_START_NUMBER;
        Token* tokens = calloc(sizeof(Token), len_tokens);

        IdentifiersRecord* record = allocateRecord();

        Namespace* ns = allocateNamespace();

        while (1) {
                size_t linelen = 0;
                source = NULL;
                printf("> ");
                if (getline(&source, &linelen, stdin) == -1) {
                        puts("");
                        return EXIT_SUCCESS;
                }
                // the memory for the line is never freed; this is intentionnal.
                // (regarding IdentifiersRecord, witch doesn't own its strings)

                nb_tokens = 0;
                do {
                        if (nb_tokens >= len_tokens) {
                                len_tokens *= 2;
                                tokens = reallocarray(tokens, sizeof(Token), len_tokens);
                        }
                        source = lex(source, &(tokens[nb_tokens++]), &record);
                } while (tokens[nb_tokens-1].type != TOKEN_EOF && tokens[nb_tokens-1].type != TOKEN_ERROR);
                if (tokens[nb_tokens-1].type == TOKEN_ERROR) {
                        puts("Lexing error.");
                        continue;
                }


                {
                        Token* tkns = tokens;
                        Node* node = parse_statement(&tkns);
                        while (node != NULL) {
                                interpretStatement(node, &ns);
                                node = parse_statement(&tkns);
                        }
                }
        }

        // unreachable
        free(tokens);
        freeRecord(record);
        freeNamespace(ns);

        return EXIT_SUCCESS;
}

static int run_file(const char* filename) {
        FILE* file = fopen(filename, "r");
        if (file == NULL) {
                printf("Can't open file `%s`.\n", filename);
                return EXIT_FAILURE;
        }
        fseek(file, 0, SEEK_END);
        long size = ftell(file) + 1;
        fseek(file, 0, SEEK_SET);

        char* contents = malloc(size*sizeof(char));
        contents[fread(contents, size, sizeof(char), file)] = '\0';
        fclose(file);

        IdentifiersRecord* record = allocateRecord();
        Namespace* ns = allocateNamespace();

        size_t nb_tokens;
        size_t len_tokens = TOKENS_START_NUMBER;
        char* source = contents;
        Token* tokens = calloc(sizeof(Token), len_tokens);

        nb_tokens = 0;
        do {
                if (nb_tokens >= len_tokens) {
                        len_tokens *= 2;
                        tokens = reallocarray(tokens, sizeof(Token), len_tokens);
                }
                source = lex(source, &(tokens[nb_tokens++]), &record);
        } while (tokens[nb_tokens-1].type != TOKEN_EOF && tokens[nb_tokens-1].type != TOKEN_ERROR);
        if (tokens[nb_tokens-1].type == TOKEN_ERROR) {
                puts("Lexing error.");
                return EXIT_FAILURE;
        }

        {
                Token* tkns = tokens;
                Node* node = parse_statement(&tkns);
                while (node != NULL) {
                        interpretStatement(node, &ns);
                        node = parse_statement(&tkns);
                }
        }

        free(contents);
        free(tokens);
        freeRecord(record);
        freeNamespace(ns);

        return EXIT_SUCCESS;

}

int main(int argc, char* argv[]) {
        switch (argc) {
                case 1:
                        return repl();
                case 2:
                        return run_file(argv[1]);
                default:
                        puts("Invalid number of arguments.\nUsage : mylang [file]");
                        return EXIT_FAILURE;
        }
}

#undef TOKENS_START_NUMBER
