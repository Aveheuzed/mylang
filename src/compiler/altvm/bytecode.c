#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "compiler/altvm/bytecode.h"
#include "compiler/altvm/vm.h"

CompiledProgram* createProgram(void) {
        CompiledProgram* ret = malloc(offsetof(CompiledProgram, bytecode) + sizeof(Bytecode)*16);
        ret->up = NULL;
        ret->comput_arr = NULL;
        ret->maxlen = 16;
        ret->len = 0;
        return ret;
}
void freeProgram(CompiledProgram* pgm) {
        free(pgm->comput_arr);
        free(pgm);
}
static CompiledProgram* growProgram(CompiledProgram* ptr, const size_t newsize) {
        ptr = realloc(ptr, offsetof(CompiledProgram, bytecode) + sizeof(Bytecode)*newsize);
        ptr->maxlen = newsize;
        return ptr;
}

static CompiledProgram* ensure_no_computarr(CompiledProgram* ptr) {
        if (ptr->comput_arr != NULL) {
                if (ptr->comput_arr->len++ > BF_MAX_RUN) LOG("Warning: computation array length > BF_MAX_RUN.");

                // ptr->comput_arr->len++;

                const RawComputationArray* arr = ptr->comput_arr;

                if (ptr->len + sizeof(arr->bytecode) + 1 >= ptr->maxlen)
                        ptr = growProgram(ptr, ptr->maxlen + sizeof(arr->bytecode) + 16);

                ptr->bytecode[ptr->len++].control = (ControlByte) {.mode=MODE_COMPUTE, .length=arr->len};

                memcpy(&(ptr->bytecode[ptr->len]), arr->bytecode, sizeof(arr->bytecode[0])*arr->len);
                // for (unsigned char i=0; i<arr->len; i++) {
                //         ptr->bytecode[ptr->len++].byte = arr->bytecode[i][0];
                //         ptr->bytecode[ptr->len++].byte = arr->bytecode[i][1];
                // }

                ptr->len += sizeof(arr->bytecode[0])*arr->len;

                free(ptr->comput_arr);
                ptr->comput_arr = NULL;
        }

        return ptr;
}
static CompiledProgram* ensure_computarr(CompiledProgram* ptr) {
        if (ptr->comput_arr == NULL)
                ptr->comput_arr = calloc(sizeof(RawComputationArray), 1);
        else if (ptr->comput_arr->len >= BF_MAX_RUN) {
                ptr = ensure_no_computarr(ptr);
                ptr->comput_arr = calloc(sizeof(RawComputationArray), 1);

                // (ptr=ensure_no_computarr(ptr))->comput_arr = calloc(sizeof(RawComputationArray))
        }
        return ptr;
}

CompiledProgram* emitLeftRight(CompiledProgram* program, ssize_t amount) {
        program = ensure_computarr(program);

        RawComputationArray *const arr = program->comput_arr;

        if (arr->bytecode[arr->len][1]) arr->len++;

        int8_t already_there = arr->bytecode[arr->len][0];
        if (already_there + amount >= INT8_MIN
        && already_there + amount <= INT8_MAX) {
                arr->bytecode[arr->len][0] += amount;
                return program;
        }

        if (amount > 0) {
                amount -= INT8_MAX - already_there;
                arr->bytecode[arr->len++][0] = INT8_MAX;
        }
        else { // if (amount <= 0)
                amount += INT8_MIN + already_there;
                arr->bytecode[arr->len++][0] = INT8_MIN;
        }
        return emitLeftRight(program, amount);
}
CompiledProgram* emitPlusMinus(CompiledProgram* program, ssize_t amount) {
        program = ensure_computarr(program);

        RawComputationArray *const arr = program->comput_arr;

        int8_t already_there = arr->bytecode[arr->len][1];
        if (already_there + amount >= INT8_MIN
        && already_there + amount <= INT8_MAX) {
                arr->bytecode[arr->len][1] += amount;
                return program;
        }

        if (amount > 0) {
                amount -= INT8_MAX - already_there;
                arr->bytecode[arr->len++][1] = INT8_MAX;
        }
        else { // if (amount <= 0)
                amount += INT8_MIN + already_there;
                arr->bytecode[arr->len++][1] = INT8_MIN;
        }
        return emitPlusMinus(program, amount);
}
CompiledProgram* emitIn(CompiledProgram* program) {
        program = ensure_no_computarr(program);

        if (program->len >= program->maxlen)
                program = growProgram(program, program->maxlen*2);
        program->bytecode[program->len++].control.mode = MODE_IN;
        return program;
}
CompiledProgram* emitOut(CompiledProgram* program) {
        program = ensure_no_computarr(program);

        if (program->len >= program->maxlen)
                program = growProgram(program, program->maxlen*2);
        program->bytecode[program->len++].control.mode = MODE_OUT;
        return program;
}
CompiledProgram* emitEnd(CompiledProgram* program) {
        program = ensure_no_computarr(program);

        if (program->up != NULL) {
                LOG("Error: terminating compilation with brackets still open.");
                freeProgram(program);
                return NULL;
        }

        if (program->len >= program->maxlen)
                program = growProgram(program, program->maxlen+1);
        program->bytecode[program->len++].control.mode = MODE_END;
        return program;
}

CompiledProgram* emitOpeningBracket(CompiledProgram* program) {
        program = ensure_no_computarr(program);
        CompiledProgram *const new = createProgram();
        new->up = program;
        return new;
}
CompiledProgram* emitClosingBracket(CompiledProgram* program) {
        CompiledProgram* up = program->up;
        if (up == NULL) {
                LOG("Error: trying to close a bracket on top-level.");
                freeProgram(program);
                return NULL;
        }

        program = ensure_no_computarr(program);

        size_t forwardjump = program->len + 1;
        size_t backwardjump = program->len + 1; // +1 → `]`

        /*
        In the following case, we can skip a few brackets when jumping backward
        [ [ foo ] bar ] baz
           ^----------+
        Instead of
        [ [ foo ] bar ] baz
         ^------------+

        On the other hand, the following case would require rewriting bytecode, so we leave that optimization to runtime.
        [ foo [ bar ] ] baz
              +--------^
        Instead of
        [ foo [ bar ] ] baz
              +------^
        */
        for (size_t i=0; i<program->len; i++) {
                if (program->bytecode[i].control.mode == MODE_CJUMPFWD) {
                        backwardjump -= 1;
                        continue;
                }
                if (program->bytecode[i].control.mode == MODE_NCJUMPFWD) {
                        backwardjump -= sizeof(size_t) + 1;
                        i += sizeof(size_t);
                        continue;
                }
                break;
        }
        if (backwardjump > BF_MAX_RUN) forwardjump += sizeof(size_t);
        if (forwardjump > BF_MAX_RUN) forwardjump += sizeof(size_t);

        const size_t new_upsize = up->len
                        + program->len
                        + (forwardjump > BF_MAX_RUN)*sizeof(size_t)
                        + (backwardjump > BF_MAX_RUN)*sizeof(size_t)
                        + 2;
        if (new_upsize >= up->maxlen)
                up = growProgram(up, new_upsize + 16);

        if (forwardjump <= BF_MAX_RUN) {
                up->bytecode[up->len++].control = (ControlByte) {.mode=MODE_CJUMPFWD, .length=forwardjump};
        }
        else {
                up->bytecode[up->len++].control = (ControlByte) {.mode=MODE_NCJUMPFWD};

                memcpy(&(up->bytecode[up->len]), &forwardjump, sizeof(forwardjump));
                up->len += sizeof(forwardjump);
        }

        memcpy(&(up->bytecode[up->len]), program->bytecode, program->len);
        up->len += program->len;

        if (backwardjump <= BF_MAX_RUN) {
                up->bytecode[up->len++].control = (ControlByte) {.mode=MODE_CJUMPBWD, .length=backwardjump};
        }
        else {
                up->bytecode[up->len++].control = (ControlByte) {.mode=MODE_NCJUMPBWD};

                memcpy(&(up->bytecode[up->len]), &backwardjump, sizeof(backwardjump));
                up->len += sizeof(backwardjump);
        }

        freeProgram(program);
        return up;
}


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

void execute(const CompiledProgram* pgm) {
        return interpretBF(pgm->bytecode);
}
