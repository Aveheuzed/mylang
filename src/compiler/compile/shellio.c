#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "compiler/compile/compiler.h"
#include "compiler/state.h"

#include "compiler/compile/shellio.h"
#include "compiler/compile/compiler_helpers.h"


void output_bf(FILE* file, const CompiledProgram* pgm) {
        static const char operators[] = {
                [BF_PLUS] = '+',
                [BF_MINUS] = '-',
                [BF_LEFT] = '<',
                [BF_RIGHT] = '>',
                [BF_INPUT] = ',',
                [BF_OUTPUT] = '.',
                [BF_JUMP_FWD] = '[',
                [BF_JUMP_BWD] = ']',
        };
        for (size_t i=0; i<pgm->len; i++) {
                const CompressedBFOperator op = pgm->bytecode[i];
                if (op.operator <= BF_CANCOMPRESS) for (size_t j=0; j<op.run; j++) fputc(operators[op.operator], file);
                else fputc(operators[op.operator], file);

                if (op.operator > BF_NOOPERAND && !op.run)
                        i += sizeof(size_t)/sizeof(op);
        }
}

void output_cbf(FILE* file, const CompiledProgram* pgm) {
        fwrite(pgm->bytecode, sizeof(pgm->bytecode[0]), pgm->len, file);
}


CompiledProgram* input_bf(FILE* file) {
        CompiledProgram* pgm = createProgram();
        while (1) switch (getc(file)) {
                case EOF:
                        return pgm;
                case '<':
                        pgm = _emitCompressible(pgm, BF_LEFT, 1);
                        break;
                case '>':
                        pgm = _emitCompressible(pgm, BF_RIGHT, 1);
                        break;
                case '+':
                        pgm = _emitCompressible(pgm, BF_PLUS, 1);
                        break;
                case '-':
                        pgm = _emitCompressible(pgm, BF_MINUS, 1);
                        break;
                case '.':
                        pgm = _emitNonCompressible(pgm, BF_OUTPUT);
                        break;
                case ',':
                        pgm = _emitCompressible(pgm, BF_INPUT, 1);
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
        return pgm;
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
