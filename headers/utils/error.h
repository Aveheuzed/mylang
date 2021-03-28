#ifndef error_h
#define error_h

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "pipeline/token.h"


void TypeError(const LocalizedToken t);
void SyntaxError(const LocalizedToken t);

#endif
