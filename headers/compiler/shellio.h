#ifndef shellio_h
#define shellio_h

#include <stdio.h>

#include "compiler/bytecode.h"

CompiledProgram* input_bf(FILE* file);
CompiledProgram* input_highlevel(FILE* file);

#endif
