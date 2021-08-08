#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "compiler/compiler.h"
#include "compiler/state.h"

#include "compiler/altvm/shellio.h"
#include "compiler/altvm/compiler_helpers.h"


void output_bf(FILE* file, const CompiledProgram* pgm) {
        for (size_t i=0; i<pgm->len; i++) {
                const ControlByte op = pgm->bytecode[i].control;
                switch (op.mode) {
                case MODE_COMPUTE:
                        for (uint8_t l=0; l<op.length; l++) {
                                int8_t run = pgm->bytecode[++i].byte;
                                for (; run>0; run--) fputc('>', file);
                                for (; run<0; run++) fputc('<', file);
                                run = pgm->bytecode[++i].byte;
                                for (; run>0; run--) fputc('+', file);
                                for (; run<0; run++) fputc('-', file);
                        }
                        break;
                case MODE_END:
                        return;
                case MODE_IN:
                        fputc(',', file);
                        break;
                case MODE_OUT:
                        fputc('.', file);
                        break;
                case MODE_CJUMPFWD:
                        fputc('[', file);
                        break;
                case MODE_CJUMPBWD:
                        fputc(']', file);
                        break;
                case MODE_NCJUMPFWD:
                        fputc('[', file);
                        i += sizeof(size_t)/sizeof(int8_t);
                        break;
                case MODE_NCJUMPBWD:
                        fputc(']', file);
                        i += sizeof(size_t)/sizeof(int8_t);
                        break;
                }
        }
}

void output_cbf(FILE* file, const CompiledProgram* pgm) {
        fwrite(pgm->bytecode, sizeof(pgm->bytecode[0]), pgm->len, file);
}


CompiledProgram* input_bf(FILE* file) {
        CompiledProgram* pgm = createProgram();
        while (1) switch (getc(file)) {
                case EOF:
                        return _emitEnd(pgm);
                case '<':
                        pgm = _emitLeftRight(pgm, -1);
                        break;
                case '>':
                        pgm = _emitLeftRight(pgm, +1);
                        break;
                case '+':
                        pgm = _emitPlusMinus(pgm, 1);
                        break;
                case '-':
                        pgm = _emitPlusMinus(pgm, -1);
                        break;
                case '.':
                        pgm = _emitOut(pgm);
                        break;
                case ',':
                        pgm = _emitIn(pgm);
                        break;
                case '[':
                        pgm = _emitOpeningBracket(pgm);
                        break;
                case ']':
                        pgm = _emitClosingBracket(pgm);
                        if (pgm == NULL) {
                                fputs("Malformation detected in input file!\n", stderr);
                                return NULL;
                        }
                        break;
                default:
                        break;
        }
}

CompiledProgram* input_cbf(FILE* file) {
        const long startpos = ftell(file);
        fseek(file, 0, SEEK_END);
        const long len = ftell(file) - startpos;
        fseek(file, startpos, SEEK_SET);

        CompiledProgram *const pgm = malloc(offsetof(CompiledProgram, bytecode) + len);
        pgm->len = pgm->maxlen = len;
        const size_t actual_len = fread(pgm->bytecode, 1, len, file);
        if (actual_len != len) {
                LOG("Warning: expected to read %lu bytes, got %lu", len, actual_len);
        }

        return pgm;
}

CompiledProgram* input_highlevel(FILE* file) {
        pipeline_state pipeline;
        mk_pipeline(&pipeline, file);

        while (compile_statement(&pipeline.cmpinfo));

        CompiledProgram *const pgm = get_bytecode(&pipeline);
        del_pipeline(&pipeline);

        return pgm;
}
