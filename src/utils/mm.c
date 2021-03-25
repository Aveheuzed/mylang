#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "pipeline/state.h"

#include "utils/compiler_helpers.h"
#include "utils/runtime_types.h"
#include "utils/mm.h"

BFMemoryView* createMemoryView(void) {
        BFMemoryView* ret = malloc(offsetof(BFMemoryView, band) + sizeof(BFState)*16);
        ret->size = 16;
        for (int i=0; i<16; i++) {
                ret->band[i] = STATE_UNTOUCHED;
        }
        return ret;
}
void freeMemoryView(BFMemoryView* view) {
        free(view);
}
static BFMemoryView* growMemoryView(BFMemoryView* oldptr, const size_t newsize) {
        oldptr = realloc(oldptr, offsetof(BFMemoryView, band)+sizeof(BFState)*newsize);
        for (size_t i=oldptr->size; i<newsize; i++) oldptr->band[i] = STATE_UNTOUCHED;
        oldptr->size = newsize;
        return oldptr;
}

// --------------------------- memory view managers ----------------------------

static size_t BF_allocate_number(compiler_info *const state) {
        size_t i;
        for (i=0; i<state->memstate->size; i++) {
                switch (state->memstate->band[i]) {
                        case STATE_AVAILABLE:
                                reset(state, i);
                        case STATE_UNTOUCHED:
                                state->memstate->band[i] = STATE_TAKEN;
                                return i;
                                break;
                        default:
                                break;
                }
        }
        state->memstate = growMemoryView(state->memstate, state->memstate->size*2);

        state->memstate->band[i] = STATE_TAKEN;
        return i;
}
Value BF_allocate(compiler_info *const state, const RuntimeType type) {
        switch (type) {
                case TYPE_INT:
                        return (Value) {.pos=BF_allocate_number(state), .type=type};
                default:
                        return (Value){.pos=SIZE_MAX, .type=type};
        }
}

static void BF_free_number(compiler_info *const state, const size_t index) {
        if (state->memstate->band[index] != STATE_TAKEN) LOG("Warning: the slot to be freed is not in the expected state");
        state->memstate->band[index] = STATE_AVAILABLE;
}
void BF_free(compiler_info *const state, const Value v) {
        if (v.pos != SIZE_MAX) switch (v.type) {
                case TYPE_INT:
                        BF_free_number(state, v.pos);
                        break;
                default:
                        LOG("Error: unhandled value type %d", v.type);
                        break;
        }
}

// --------------------------- end memory view managers ------------------------
