#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include "headers/pipeline/lexer.h"
#include "headers/pipeline/parser.h"
#include "headers/pipeline/interpreter.h"

int main(int argc, char* argv[]) {
        FILE* source_code;
        switch (argc) {
                case 1:
                        source_code = stdin; break;
                case 2:
                        source_code = fopen(argv[1], "r"); break;
                default:
                        puts("Invalid number of arguments.\nUsage : mylang [file]");
                        return EXIT_FAILURE;
        }
        parser_info psinfo = mk_parser_info(source_code);
        init_lexing();
        Namespace* ns = allocateNamespace();
        while (interpretStatement(&psinfo, &ns));
        fclose(source_code);
        return EXIT_SUCCESS;
}
