#ifndef error_h
#define error_h

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "headers/pipeline/token.h"


void TypeError(const Token t);
void SyntaxError(const Token t);
void RuntimeError(const Token t);
void ArityError(const size_t expected, const size_t actual);

#endif
