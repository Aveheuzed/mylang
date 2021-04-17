#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "pipeline/bf.h"

typedef uint8_t Word;

static inline Word* growBand(Word* ptr, const size_t oldlen, const size_t newlen) {
        ptr = reallocarray(ptr, newlen, sizeof(Word));
        memset(ptr+oldlen, 0, (newlen-oldlen)*sizeof(Word));
        return ptr;
}

void interpretBF(const Bytecode bytecode[]) {
        
}
