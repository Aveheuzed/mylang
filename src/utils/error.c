#include <stdio.h>

#include "headers/utils/error.h"

void TypeError(const Token t) {
        fprintf(stderr, "Incompatible types for %s.\n", t.source);
}
void SyntaxError(const Token t) {
        fprintf(stderr, "Invalid syntax on line %u at \"%*s\".\n", t.line, t.length, t.source);
}
void RuntimeError(const Token t) {
        fprintf(stderr, "Runtime error on line %u at \"%*s\".\n", t.line, t.length, t.source);
}
void ArityError(const uintptr_t expected, const uintptr_t actual) {
        fprintf(stderr, "Arity error : expected %lu argument(s), got %lu.\n", expected, actual);
}
