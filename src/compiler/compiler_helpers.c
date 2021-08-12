#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#include "compiler/compiler_helpers.h"
#include "compiler/compiler.h"
#include "compiler/bytecode.h"
#include "compiler/mm.h"
#include "hash.h"


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
