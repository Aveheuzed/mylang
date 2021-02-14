#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "headers/pipeline/lexer.h"
#include "headers/pipeline/parser.h"
#include "headers/pipeline/interpreter.h"
#include "headers/utils/builtins.h"

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
        IdentifiersRecord** tstring = init_lexing();
        Namespace* ns = allocateNamespace();

        LOG("Starting to put built-in functions in the global namespace");

        ns_set_value(&ns, internalize(tstring, strdup("print"), strlen("print")), (Object){.type=TYPE_NATIVEF, .natfunval=&print_value});
        ns_set_value(&ns, internalize(tstring, strdup("clock"), strlen("clock")), (Object){.type=TYPE_NATIVEF, .natfunval=&native_clock});

        LOG("Done putting built-in functions in the global namespace");

        while (interpretStatement(&psinfo, &ns));
        fclose(source_code);
        return EXIT_SUCCESS;
}
