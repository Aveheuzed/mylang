#include <stdio.h>

#include "headers/utils/error.h"

void TypeError(const LocalizedToken t) {
        fprintf(stderr, "Incompatible types for %s.\n", t.tok.source);
}
void SyntaxError(const LocalizedToken t) {
        fprintf(stderr, "Invalid syntax on line %u at \"%*s\".\n", t.pos.line, t.tok.length, t.tok.source);
}
void RuntimeError(const LocalizedToken t) {
        fprintf(stderr, "Runtime error on line %u at \"%*s\".\n", t.pos.line, t.tok.length, t.tok.source);
}
void ArityError(const uintptr_t expected, const uintptr_t actual) {
        fprintf(stderr, "Arity error : expected %lu argument(s), got %lu.\n", expected, actual);
}
