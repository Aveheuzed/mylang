#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#include "compiler/compiler_helpers.h"
#include "compiler/state.h"
#include "compiler/bytecode.h"
#include "compiler/mm.h"
#include "hash.h"

#define GROW_THRESHHOLD (0.8) // TODO: benchmark this value

static inline hash_t hash(const char* key) {
        /* we have to remove the least significant bits, because that pointer
        was returned by `malloc`, and is therefore aligned for the largest types
        available. We take 16 bytes for "largest type available", to be safe.
        */
        return ((hash_t) key) >> 4;
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

// ---------------------- core helpers -----------------------------------------

void seekpos(compiler_info *const state, const size_t i) {
        if (state->current_pos < i) EMIT_RIGHT(state, i-state->current_pos);
        else if (state->current_pos > i) EMIT_LEFT(state, state->current_pos-i);
        state->current_pos = i;
}
void transfer(compiler_info *const state, const size_t pos, const int nb_targets, const Target targets[]) {
        seekpos(state, pos);
        OPEN_JUMP(state);
        for (int i=0; i<nb_targets; i++) {
                const Target t = targets[i];
                if (pos == t.pos) {
                        LOG("Warning: linear transformation onto oneself might result in an infinite loop");
                }
                seekpos(state, t.pos);
                if (t.weight > 0) {
                        EMIT_PLUS(state, t.weight);
                } else if (t.weight < 0) {
                        EMIT_MINUS(state, -t.weight);
                } else {
                        LOG("Warning: Target with weight 0");
                }
        }
        seekpos(state, pos);
        EMIT_MINUS(state, 1);
        CLOSE_JUMP(state);
}
void reset(compiler_info *const state, const size_t i) {
        // [-]
        transfer(state, i, 0, NULL);
}

void runtime_mul_int(compiler_info *const state, const Target target, const size_t xpos, const size_t ypos) {
        Value yclone = BF_allocate(state, TYPE_INT);

        seekpos(state, xpos);
        OPEN_JUMP(state);
        EMIT_MINUS(state, 1); // loop: do <X> times:

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
        CLOSE_JUMP(state);

        BF_free(state, yclone);
}
// ---------------------- end core helpers -------------------------------------


#undef GROW_THRESHHOLD
