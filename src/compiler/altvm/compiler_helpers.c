#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#include "compiler/compiler_helpers.h"
#include "compiler/state.h"
#include "compiler/altvm/bytecode.h"
#include "compiler/mm.h"
#include "compiler/hash.h"

#define GROW_THRESHHOLD (0.8) // TODO: benchmark this value

typedef struct RawComputationArray {
        unsigned char len; // number of completed [(</>) (+/-)] pairs
        int8_t bytecode[BF_MAX_RUN][2];
} RawComputationArray;

static inline hash_t hash(const char* key) {
        /* we have to remove the least significant bits, because that pointer
        was returned by `malloc`, and is therefore aligned for the largest types
        available. We take 16 bytes for "largest type available", to be safe.
        */
        return ((hash_t) key) >> 4;
}

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
Variable* getVariable(compiler_info *const state, char const* name) {
        // this function is guaranteed to return; any nonexistent variables
        // raise errors on parsing, so we're safe here at compiler level.
        for (BFNamespace* ns=state->currentNS; ns!=NULL; ns=ns->enclosing) {
                const hash_t mask = ns->allocated - 1;

                hash_t index;
                for (
                        index = hash(name) & mask;
                        ns->dict[index].name != NULL;
                        index = (index+1)&mask
                ) if (ns->dict[index].name == name) return &(ns->dict[index]);
        }

        // just there so as not to trigger GCC
        return NULL;
}

// --------------------------- emitXXX helpers ---------------------------------

CompiledProgram* _emitLeftRight(CompiledProgram* program, ssize_t amount) {
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
        return _emitLeftRight(program, amount);
}
CompiledProgram* _emitPlusMinus(CompiledProgram* program, ssize_t amount) {
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
        return _emitPlusMinus(program, amount);
}
CompiledProgram* _emitIn(CompiledProgram* program) {
        program = ensure_no_computarr(program);

        if (program->len >= program->maxlen)
                program = growProgram(program, program->maxlen*2);
        program->bytecode[program->len++].control.mode = MODE_IN;
        return program;
}
CompiledProgram* _emitOut(CompiledProgram* program) {
        program = ensure_no_computarr(program);

        if (program->len >= program->maxlen)
                program = growProgram(program, program->maxlen*2);
        program->bytecode[program->len++].control.mode = MODE_OUT;
        return program;
}
CompiledProgram* _emitEnd(CompiledProgram* program) {
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

CompiledProgram* _emitOpeningBracket(CompiledProgram* program) {
        program = ensure_no_computarr(program);
        CompiledProgram *const new = createProgram();
        new->up = program;
        return new;
}
CompiledProgram* _emitClosingBracket(CompiledProgram* program) {
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

void emitPlus(compiler_info *const state, const size_t amount) {
        state->program = _emitPlusMinus(state->program, amount);
}
void emitMinus(compiler_info *const state, const size_t amount) {
        state->program = _emitPlusMinus(state->program, -amount);
}
void emitLeft(compiler_info *const state, const size_t amount) {
        state->program = _emitLeftRight(state->program, -amount);
}
void emitRight(compiler_info *const state, const size_t amount) {
        state->program = _emitLeftRight(state->program, amount);
}
void emitInput(compiler_info *const state) {
        state->program = _emitIn(state->program);
}
void emitOutput(compiler_info *const state) {
        state->program = _emitOut(state->program);
}
void openJump(compiler_info *const state) {
        state->code_isnonlinear++;
        state->program = _emitOpeningBracket(state->program);
}
void closeJump(compiler_info *const state) {
        state->program = _emitClosingBracket(state->program);
        state->code_isnonlinear--;
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
        openJump(state);
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
        closeJump(state);
}
void reset(compiler_info *const state, const size_t i) {
        // [-]
        transfer(state, i, 0, NULL);
}

void runtime_mul_int(compiler_info *const state, const Target target, const size_t xpos, const size_t ypos) {
        Value yclone = BF_allocate(state, TYPE_INT);

        seekpos(state, xpos);
        openJump(state);
        emitMinus(state, 1); // loop: do <X> times:

        const Target forth[] = {
                target,
                {.pos=yclone.pos, .weight=1},
        }; // transfer Y onto these
        const Target back[] = {
                {.pos=ypos, .weight=1},
        }; // then transfer Y' back on Y
        transfer(state, ypos, sizeof(forth)/sizeof(*forth), forth);
        transfer(state, yclone.pos, sizeof(back)/sizeof(*back), back);

        seekpos(state, xpos); // the loop is on X ; don't forget to jump back there before closing the jump!
        closeJump(state);

        BF_free(state, yclone);
}
// ---------------------- end core helpers -------------------------------------


#undef GROW_THRESHHOLD
