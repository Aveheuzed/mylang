#include <stdio.h>
#include <string.h>

#include "compiler/compiler.h"
#include "compiler/pipeline.h"
#include "compiler/shellio.h"
#include "compiler/bytecode.h"
#include "compiler/builtins.h"
#include "identifiers_record.h"
#include "compiler/namespace.h"
#include "keywords.h"

static const Keyword keywords[] = {
        {"int", TOKEN_KW_INT},
        {"str", TOKEN_KW_STR},
        {"if", TOKEN_IF},
        {"else", TOKEN_ELSE},
        {"do", TOKEN_DO},
        {"while", TOKEN_WHILE},

        {NULL, 0}
};

static inline void declare_variable(pipeline_state *const pipeline,  BuiltinFunction *const function) {
        char *const mallocd = internalize(&(pipeline->cmpinfo.prsinfo.lxinfo.record), strdup(function->name), strlen(function->name));

        Variable v = (Variable) {.name=mallocd, .func=function};

        addVariable(&(pipeline->cmpinfo), v);
}

CompiledProgram* input_highlevel(FILE* file) {
        pipeline_state pipeline;
        mk_pipeline(&pipeline, file, keywords);

        for (size_t i=0; i<nb_builtins; i++) {
                declare_variable(&pipeline, &(builtins[i]));
        }

        while (compile_statement(&pipeline.cmpinfo));

        CompiledProgram *const pgm = get_bytecode(&pipeline);
        del_pipeline(&pipeline);

        return pgm;
}

CompiledProgram* input_bf(FILE* file) {
        CompiledProgram* pgm = createProgram();
        while (1) switch (getc(file)) {
                case EOF:
                        return emitEnd(pgm);
                case '<':
                        pgm = emitLeftRight(pgm, -1);
                        break;
                case '>':
                        pgm = emitLeftRight(pgm, +1);
                        break;
                case '+':
                        pgm = emitPlusMinus(pgm, 1);
                        break;
                case '-':
                        pgm = emitPlusMinus(pgm, -1);
                        break;
                case '.':
                        pgm = emitOut(pgm);
                        break;
                case ',':
                        pgm = emitIn(pgm);
                        break;
                case '[':
                        pgm = emitOpeningBracket(pgm);
                        break;
                case ']':
                        pgm = emitClosingBracket(pgm);
                        if (pgm == NULL) {
                                fputs("Malformation detected in input file!\n", stderr);
                                return NULL;
                        }
                        break;
                default:
                        break;
        }
}
