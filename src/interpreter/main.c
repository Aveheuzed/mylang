#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "interpreter/pipeline.h"
#include "lexer.h"
#include "interpreter/parser.h"
#include "interpreter/interpreter.h"
#include "interpreter/builtins.h"
#include "keywords.h"

static const Keyword keywords[] = {
        {"true", TOKEN_TRUE},
        {"false", TOKEN_FALSE},
        {"none", TOKEN_NONE},
        {"if", TOKEN_IF},
        {"else", TOKEN_ELSE},
        {"while", TOKEN_WHILE},
        {"function", TOKEN_FUNC},
        {"return", TOKEN_RETURN},

        {NULL, 0}
};


static inline void declare_variable(pipeline_state *const pipeline, const char* key, Object value) {
        ns_set_value(
                &(pipeline->interpinfo.ns),
                internalize(&(pipeline->interpinfo.prsinfo.lxinfo.record), strdup(key), strlen(key)),
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
                        printf("Invalid number of arguments.\nUsage : %s [file]", argv[0]);
                        return EXIT_FAILURE;
        }
        pipeline_state state;
        mk_pipeline(&state, source_code, keywords);

        declare_variable(&state, "print", (Object){.type=TYPE_NATIVEF, .natfunval=&print_value});
        declare_variable(&state, "clock", (Object){.type=TYPE_NATIVEF, .natfunval=&native_clock});

        declare_variable(&state, "str", (Object){.type=TYPE_NATIVEF, .natfunval=&tostring});
        declare_variable(&state, "bool", (Object){.type=TYPE_NATIVEF, .natfunval=&tobool});
        declare_variable(&state, "int", (Object){.type=TYPE_NATIVEF, .natfunval=&toint});
        declare_variable(&state, "float", (Object){.type=TYPE_NATIVEF, .natfunval=&tofloat});

        // no input in REPL, because reading tokens and input from the same source cases havroc
        if (argc == 2) declare_variable(&state, "input", (Object){.type=TYPE_NATIVEF, .natfunval=&input});

        while (interpretStatement(&(state.interpinfo)) == OK_OK);

        del_pipeline(&state);

        return EXIT_SUCCESS;
}
