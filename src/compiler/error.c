#include <stdio.h>

#include "utils/error.h"

void TypeError(const LocalizedToken t) {
        fprintf(stderr, "Incompatible types for %s.\n", t.tok.source);
}
void SyntaxError(const LocalizedToken t) {
        fprintf(stderr, "Invalid syntax on line %u at \"%*s\".\n", t.pos.line, t.tok.length, t.tok.source);
}
