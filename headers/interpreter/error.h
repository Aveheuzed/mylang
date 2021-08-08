#ifndef error_h
#define error_h

#include "interpreter/token.h"

void Error(const LocalizedToken* where, const char* message, ...) __attribute__((format(printf, 2, 3)));
void Warning(const LocalizedToken* where, const char* message, ...) __attribute__((format(printf, 2, 3)));

#endif
