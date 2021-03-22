#include <stdlib.h>
#include <stdio.h>

#include "headers/pipeline/bytecode.h"
#include "headers/pipeline/bf.h"
#include "headers/utils/shellio.h"

int main(int argc, char* argv[]) {
        FILE* source_code;
        switch (argc) {
                case 1:
                        source_code = stdin;
                        break;
                case 2:
                        source_code = fopen(argv[1], "r");
                        break;
                default:
                        puts("Invalid number of arguments.\nUsage : mylang [file]");
                        return EXIT_FAILURE;
        }

        CompiledProgram* program = input_highlevel(source_code);
        fclose(source_code);
        if (program == NULL) return EXIT_FAILURE;

        interpretBF(program);
        free(program);

        return EXIT_SUCCESS;
}
