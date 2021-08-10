#include <stdio.h>

#include "compiler/compiler.h"
#include "compiler/pipeline.h"
#include "compiler/shellio.h"
#include "compiler/bytecode.h"

CompiledProgram* input_highlevel(FILE* file) {
        pipeline_state pipeline;
        mk_pipeline(&pipeline, file);

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
