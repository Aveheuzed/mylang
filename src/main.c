#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "headers/pipeline/lexer.h"
#include "headers/pipeline/parser.h"
#include "headers/pipeline/interpreter.h"
#include "headers/utils/builtins.h"


static inline void declare_variable(parser_info *const prsinfo, Namespace **const ns, const char* key, Object value) {
        ns_set_value(
                ns,
                internalize(&(prsinfo->lxinfo.record), strdup(key), strlen(key)),
                value
        );
}

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
        Namespace* ns = allocateNamespace(NULL);

        declare_variable(&psinfo, &ns, "print", (Object){.type=TYPE_NATIVEF, .natfunval=&print_value});
        declare_variable(&psinfo, &ns, "clock", (Object){.type=TYPE_NATIVEF, .natfunval=&native_clock});

        while (interpretStatement(&psinfo, &ns));

        del_parser_info(psinfo);

        return EXIT_SUCCESS;
}
