#ifndef mm_h
#define mm_h

#include <stddef.h>

#include "headers/pipeline/compiler.h"
#include "headers/utils/runtime_types.h"

typedef struct Value {
        size_t pos;
        RuntimeType type;
} Value;


typedef enum BFState {
        STATE_UNTOUCHED, // guaranteed to be zero
        STATE_AVAILABLE, // not guaranteed
        STATE_TAKEN,
} BFState;

typedef struct BFMemoryView {
        size_t size;
        BFState band[];
} BFMemoryView;

BFMemoryView* createMemoryView(void);
void freeMemoryView(BFMemoryView* view);

Value BF_allocate(compiler_info *const state, const RuntimeType type) __attribute__ ((warn_unused_result));

void BF_free(compiler_info *const state, const Value v);

#endif
