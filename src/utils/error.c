#include <stdio.h>
#include <stdarg.h>

#include "headers/utils/error.h"

void Error(const LocalizedToken* where, const char* message, ...) {
        va_list params;
        va_start(params, message);
        fprintf(stderr, "Error at line %u, column %u, at `%s`: ", where->pos.line, where->pos.column, where->tok.source);
        vfprintf(stderr, message, params);
        va_end(params);
}

void Warning(const LocalizedToken* where, const char* message, ...) {
        va_list params;
        va_start(params, message);
        fprintf(stderr, "Warning at line %u, column %u, at `%s`: ", where->pos.line, where->pos.column, where->tok.source);
        vfprintf(stderr, message, params);
        va_end(params);
}
