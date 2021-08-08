#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "compiler/compile/bytecode.h"

CompiledProgram* createProgram(void) {
        CompiledProgram* ret = malloc(offsetof(CompiledProgram, bytecode) + sizeof(CompressedBFOperator)*16);
        ret->up = NULL;
        ret->maxlen = 16;
        ret->len = 0;
        return ret;
}
void freeProgram(CompiledProgram* pgm) {
        free(pgm);
}
static CompiledProgram* growProgram(CompiledProgram* ptr, const size_t newsize) {
        ptr = realloc(ptr, offsetof(CompiledProgram, bytecode) + sizeof(CompressedBFOperator)*newsize);
        ptr->maxlen = newsize;
        return ptr;
}

static CompiledProgram* emitCompressible(CompiledProgram* program, BFOperator op, size_t amount) {
        if (op > BF_CANCOMPRESS) {
                LOG("Error : trying to compress operator %hu", op);
                return program;
        }
        if (program->len) {
                CompressedBFOperator *const last_op = &(program->bytecode[program->len-1]);
                if (last_op->operator == op && last_op->run) {
                // we check for run != 0 to prevent `+` from nesting into a `]`'s field.
                // a compressible operator with a run of 0 is silly anyway!
                        if (last_op->run + amount < BF_MAX_RUN) {
                                last_op->run += amount;
                                amount = 0;
                        }
                        else {
                                amount -= BF_MAX_RUN - last_op->run;
                                last_op->run = BF_MAX_RUN;
                        }
                }
        }
        for (; amount >= BF_MAX_RUN; amount -= BF_MAX_RUN) {
                if (program->len >= program->maxlen)
                        program = growProgram(program, program->maxlen*2);
                program->bytecode[program->len++] = (CompressedBFOperator) {.operator=op, .run=BF_MAX_RUN};
        }
        if (amount) {
                if (program->len >= program->maxlen)
                        program = growProgram(program, program->maxlen*2);
                program->bytecode[program->len++] = (CompressedBFOperator) {.operator=op, .run=amount};
        }

        return program;
}
static CompiledProgram* emitNonCompressible(CompiledProgram* program, BFOperator op) {
        if (op <= BF_CANCOMPRESS) {
                LOG("Warning : trying to not compress operator %hu", op);
        }
        if (program->len >= program->maxlen)
                program = growProgram(program, program->maxlen*2);
        program->bytecode[program->len++] = (CompressedBFOperator) {.operator=op, .run=0};
        return program;
}

CompiledProgram* emitOpeningBracket(CompiledProgram* program) {
        CompiledProgram *const new = createProgram();
        new->up = program;
        return new;
}
CompiledProgram* emitClosingBracket(CompiledProgram* program) {
        CompiledProgram* up = program->up;
        if (up != NULL) {
                size_t uplen = up->len;

                // the `+1`s account for the not-yet-written `]`
                size_t bwdjump = (program->len + 1);

                // we're gonna add the whole program, plus `[`, plus `]`
                up->len += program->len + 1 + 1;
                if (up->len >= up->maxlen)
                        up = growProgram(up, up->len);

                if (bwdjump <= BF_MAX_RUN) {
                        // short jump, encoded in the bracket's `.run`
                        up->bytecode[uplen++] = (CompressedBFOperator) {.operator=BF_JUMP_FWD, .run=bwdjump};
                        memcpy(&(up->bytecode[uplen]), program->bytecode, program->len*sizeof(CompressedBFOperator));
                        uplen += program->len;
                        up->bytecode[uplen++] = (CompressedBFOperator) {.operator=BF_JUMP_BWD, .run=bwdjump};
                }
                else {
                        // long jump, encoded in separate fields (size_t)

                        // ajusting for the two fields
                        size_t fwdjump = bwdjump + 2 * sizeof(fwdjump) / sizeof(CompressedBFOperator);
                        up->len += (sizeof(fwdjump)+sizeof(bwdjump))/sizeof(CompressedBFOperator);
                        if (up->len >= up->maxlen)
                                up = growProgram(up, up->len);

                        up->bytecode[uplen++] = (CompressedBFOperator) {.operator=BF_JUMP_FWD, .run=0};

                        memcpy(&(up->bytecode[uplen]), &fwdjump, sizeof(fwdjump));
                        uplen += sizeof(fwdjump)/sizeof(CompressedBFOperator);

                        memcpy(&(up->bytecode[uplen]), program->bytecode, program->len*sizeof(CompressedBFOperator));
                        uplen += program->len;

                        up->bytecode[uplen++] = (CompressedBFOperator) {.operator=BF_JUMP_BWD, .run=0};

                        memcpy(&(up->bytecode[uplen]), &bwdjump, sizeof(bwdjump));
                        uplen += sizeof(bwdjump)/sizeof(CompressedBFOperator);
                }

                if (uplen != up->len) LOG("Assertion error while closing a bracket: lengths off by %lu", uplen - up->len);

        }
        else LOG("Error: trying to close a bracket on top-level");

        freeProgram(program);
        return up;
}
CompiledProgram* emitPlusMinus(CompiledProgram* program, ssize_t amount) {
        if (amount < 0) return emitCompressible(program, BF_MINUS, llabs(amount));
        if (amount > 0) return emitCompressible(program, BF_PLUS, llabs(amount));
        return program;
}
CompiledProgram* emitLeftRight(CompiledProgram* program, ssize_t amount) {
        if (amount < 0) return emitCompressible(program, BF_LEFT, llabs(amount));
        if (amount > 0) return emitCompressible(program, BF_RIGHT, llabs(amount));
        return program;
}
CompiledProgram* emitIn(CompiledProgram* program) {
        return emitNonCompressible(program, BF_INPUT);
}
CompiledProgram* emitOut(CompiledProgram* program) {
        return emitNonCompressible(program, BF_OUTPUT);
}
CompiledProgram* emitEnd(CompiledProgram* program) {
        return program;
}

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
