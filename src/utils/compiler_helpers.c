#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "utils/compiler_helpers.h"
#include "pipeline/state.h"
#include "pipeline/bytecode.h"
#include "utils/mm.h"
#include "utils/hash.h"

#define GROW_THRESHHOLD (0.8) // TODO: benchmark this value

static inline hash_t hash(const char* key) {
        /* we have to remove the least significant bits, because that pointer
        was returned by `malloc`, and is therefore aligned for the largest types
        available. We take 16 bytes for "largest type available", to be safe.
        */
        return ((hash_t) key) >> 4;
}

CompiledProgram* createProgram(void) {
        CompiledProgram* ret = malloc(offsetof(CompiledProgram, bytecode) + sizeof(CompressedBFOperator)*16);
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

void pushNamespace(compiler_info *const state) {
        BFNamespace* ns = malloc(offsetof(BFNamespace, dict) + sizeof(Variable)*16);
        ns->allocated = 16;
        ns->len = 0;
        ns->enclosing = state->currentNS;
        state->currentNS = ns;
        for (int i=0; i < ns->allocated; i++) {
                ns->dict[i].name = NULL;
        }
}
void popNamespace(compiler_info *const state) {
        BFNamespace* ns = state->currentNS;
        state->currentNS = ns->enclosing;

        for (size_t i=0 ; i<ns->allocated; i++) {
                Variable v = ns->dict[i];
                if (v.name != NULL) BF_free(state, v.val);
        }
        free(ns);
}

// do not use on a non-toplevel namespace, that would break the ns chain
static BFNamespace* growNamespace(BFNamespace* ns) {
        const size_t oldsize = ns->allocated;
        ns->allocated *= 2;
        ns = realloc(ns, offsetof(BFNamespace, dict) + sizeof(Variable)*(ns->allocated));
        for (int i=oldsize; i < ns->allocated; i++) {
                ns->dict[i].name = NULL;
        }
        return ns;

}

// returns nonzero on failure
// len must be 1 when allocating a !TYPE_VSTRING
int addVariable(compiler_info *const state, Variable v) {
        if (state->currentNS->allocated*GROW_THRESHHOLD < state->currentNS->len) {
                state->currentNS = growNamespace(state->currentNS);
        }

        BFNamespace *const ns = state->currentNS;

        const hash_t mask = ns->allocated - 1;

        hash_t index;
        for (
                index = hash(v.name) & mask;
                ns->dict[index].name != NULL;
                index = (index+1)&mask
        ) if (ns->dict[index].name == v.name) return 0;

        ns->dict[index] = v;

        return 1;
}
Variable getVariable(compiler_info *const state, char const* name) {
        // this function is guaranteed to return; any nonexistent variables
        // raise errors on parsing, so we're safe here at compiler level.
        for (BFNamespace* ns=state->currentNS; ns!=NULL; ns=ns->enclosing) {
                const hash_t mask = ns->allocated - 1;

                hash_t index;
                for (
                        index = hash(name) & mask;
                        ns->dict[index].name != NULL;
                        index = (index+1)&mask
                ) if (ns->dict[index].name == name) return ns->dict[index];
        }

        // just there so as not to trigger GCC
        return (Variable){.name=NULL, .val.pos=SIZE_MAX, .val.type=TYPEERROR};
}

// --------------------------- emitXXX helpers ---------------------------------

CompiledProgram* _emitCompressible(CompiledProgram* program, BFOperator op, size_t amount) {
        if (op > BF_CANCOMPRESS) {
                LOG("Error : trying to compress operator %hu", op);
                return program;
        }
        if (program->len) {
                CompressedBFOperator *const last_op = &(program->bytecode[program->len-1]);
                if (last_op->operator == op) {
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
CompiledProgram* _emitNonCompressible(CompiledProgram* program, BFOperator op) {
        if (op <= BF_CANCOMPRESS) {
                LOG("Warning : trying to not compress operator %hu", op);
        }
        if (program->len >= program->maxlen)
                program = growProgram(program, program->maxlen*2);
        program->bytecode[program->len++] = (CompressedBFOperator) {.operator=op, .run=1};
        return program;
}

CompiledProgram* _emitOpeningBracket(CompiledProgram* program, size_t *const bracketpos) {
        *bracketpos = program->len;
        program->len++;
        program->len += sizeof(ptrdiff_t) / sizeof(CompressedBFOperator);
        if (program->len > program->maxlen)
                program = growProgram(program, program->maxlen*2);
        program->bytecode[*bracketpos] = (CompressedBFOperator) {.operator=BF_JUMP_FWD, .run=1};
        program->bytecode[program->len-1].run = BF_MAX_RUN; // security, this will be overwritten later
        return program;
}
CompiledProgram* _emitClosingBracket(CompiledProgram* program, const size_t bracket) {
        size_t pos = program->len;
        program->bytecode[program->len++] = (CompressedBFOperator) {.operator=BF_JUMP_BWD, .run=1};
        program->len += sizeof(ptrdiff_t) / sizeof(CompressedBFOperator);
        if (program->len > program->maxlen)
                program = growProgram(program, program->maxlen*2);

        ptrdiff_t fwdjump = pos - bracket + sizeof(ptrdiff_t)/sizeof(CompressedBFOperator);
        ptrdiff_t bwdjump = bracket - pos + sizeof(ptrdiff_t)/sizeof(CompressedBFOperator);
        memcpy(&(program->bytecode[bracket+1]), &fwdjump, sizeof(fwdjump));
        memcpy(&(program->bytecode[pos+1]), &bwdjump, sizeof(bwdjump));
        return program;
}

void emitPlus(compiler_info *const state, const size_t amount) {
        state->program = _emitCompressible(state->program, BF_PLUS, amount);
}
void emitMinus(compiler_info *const state, const size_t amount) {
        state->program = _emitCompressible(state->program, BF_MINUS, amount);
}
void emitLeft(compiler_info *const state, const size_t amount) {
        state->program = _emitCompressible(state->program, BF_LEFT, amount);
}
void emitRight(compiler_info *const state, const size_t amount) {
        state->program = _emitCompressible(state->program, BF_RIGHT, amount);
}
void emitInput(compiler_info *const state) {
        state->program = _emitNonCompressible(state->program, BF_INPUT);
}
void emitOutput(compiler_info *const state) {
        state->program = _emitNonCompressible(state->program, BF_OUTPUT);
}
size_t openJump(compiler_info *const state) {
        size_t jump;
        state->program = _emitOpeningBracket(state->program, &jump);
        return jump;
}
void closeJump(compiler_info *const state, const size_t jump_pos) {
        state->program = _emitClosingBracket(state->program, jump_pos);
}

// ----------------------- end emitXXX helpers ---------------------------------

// ---------------------- core helpers -----------------------------------------

void seekpos(compiler_info *const state, const size_t i) {
        if (state->current_pos < i) emitRight(state, i-state->current_pos);
        else if (state->current_pos > i) emitLeft(state, state->current_pos-i);
        state->current_pos = i;
}
void transfer(compiler_info *const state, const size_t pos, const int nb_targets, const Target targets[]) {
        seekpos(state, pos);
        const size_t jump = openJump(state);
        for (int i=0; i<nb_targets; i++) {
                const Target t = targets[i];
                if (pos == t.pos) {
                        LOG("Warning: linear transformation onto oneself might result in an infinite loop");
                }
                seekpos(state, t.pos);
                if (t.weight > 0) {
                        emitPlus(state, t.weight);
                } else if (t.weight < 0) {
                        emitMinus(state, -t.weight);
                } else {
                        LOG("Warning: Target with weight 0");
                }
        }
        seekpos(state, pos);
        emitMinus(state, 1);
        closeJump(state, jump);
}
void reset(compiler_info *const state, const size_t i) {
        // [-]
        transfer(state, i, 0, NULL);
}
// ---------------------- end core helpers -------------------------------------


#undef GROW_THRESHHOLD
