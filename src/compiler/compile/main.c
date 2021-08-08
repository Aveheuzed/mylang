#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "compiler/compile/bytecode.h"
#include "compiler/compile/bf.h"
#include "compiler/shellio.h"

int main(int argc, char *const argv[]) {
        static const char helpstring[] = "\n\
Command-line options:\n\
\n\
-h help\n\
\n\
-o output file (cbf)\n\
-O output file (bf)\n\
-x execute\n\
\n\
These are mutually exclusive; the last to be found will be used.\n\
-i input file (cbf)\n\
-I input file (bf)\n\
-s input file (script)\n\
\n\
Use '-' to indicate stdin/stdout when appropriate.\n\
When specifying several times the same option, the last one takes precedence.\n\
";
        static const char optstring[] = "hi:I:s:o:O:x";
        extern char* optarg;
        extern int optind;

        char* input_path = NULL;
        char* oarg = NULL;
        char* Oarg = NULL;
        char input_method = 0;
        char output_method = 0;

        int option;
        while ((option=getopt(argc, argv, optstring)) != -1) switch (option) {
                case '?':
                        fputs(helpstring, stderr);
                        return EXIT_FAILURE;
                case 'h':
                        fputs(helpstring, stderr);
                        return EXIT_SUCCESS;
                // inputs
                case 'i':
                        input_path = optarg;
                        input_method = 1;
                        break;
                case 'I':
                        input_path = optarg;
                        input_method = 2;
                        break;
                case 's':
                        input_path = optarg;
                        input_method = 3;
                        break;
                // outputs
                case 'o':
                        output_method |= 1<<1;
                        oarg = optarg;
                        break;
                case 'O':
                        output_method |= 1<<2;
                        Oarg = optarg;
                        break;
                case 'x':
                        output_method |= 1<<3;
                        break;
        }

        if (optind != argc) {
                fputs("Too many arguments.", stderr);
                fputs(helpstring, stderr);
                return EXIT_FAILURE;
        }

        if (!input_method || !output_method) {
                fputs("Expected an input and at least one processing order.", stderr);
                fputs(helpstring, stderr);
                return EXIT_FAILURE;
        }

        FILE* input_file = strcmp(input_path, "-") ? fopen(input_path, "r") : stdin;

        if (input_file == NULL) {
                fprintf(stderr, "I/O error: couldn't open %s for reading.", input_path);
                return EXIT_FAILURE;
        }

        CompiledProgram* pgm = NULL;
        switch (input_method) {
                case 1:
                        pgm = input_cbf(input_file);
                        break;
                case 2:
                        pgm = input_bf(input_file);
                        break;
                case 3:
                        pgm = input_highlevel(input_file);
                        break;
        }

        fclose(input_file);

        if (pgm == NULL) return EXIT_FAILURE;

        if (output_method&(1<<1)) {
                FILE* file = strcmp(oarg, "-") ? fopen(oarg, "w") : stdout;
                if (file == NULL) {
                        fprintf(stderr, "I/O error: couldn't open %s for writing.", oarg);
                        return EXIT_FAILURE;
                }
                output_cbf(file, pgm);
                fclose(file);
        }
        if (output_method&(1<<2)) {
                FILE* file = strcmp(Oarg, "-") ? fopen(Oarg, "w") : stdout;
                if (file == NULL) {
                        fprintf(stderr, "I/O error: couldn't open %s for writing.", Oarg);
                        return EXIT_FAILURE;
                }
                output_bf(file, pgm);
                fclose(file);
        }
        if (output_method&(1<<3)) {
                interpretBF(pgm->bytecode, pgm->bytecode + pgm->len);
        }

        free(pgm);

}
