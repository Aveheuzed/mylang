#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "headers/pipeline/lexer.h"
#include "headers/pipeline/parser.h"
#include "headers/pipeline/compiler.h"
#include "headers/pipeline/bf.h"

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
        compiler_info cmpinfo = mk_compiler_info(source_code);

        while (compile_statement(&cmpinfo));

        interpretBF(cmpinfo.program);

        del_compiler_info(cmpinfo);

        return EXIT_SUCCESS;
}
