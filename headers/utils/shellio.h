#ifndef shellio_h
#define shellio_h

#include <stdio.h>
#include "headers/pipeline/bytecode.h"

void output_bf(FILE* file, const CompiledProgram* pgm);
void output_cbf(FILE* file, const CompiledProgram* pgm);
CompiledProgram* input_bf(FILE* file);
CompiledProgram* input_cbf(FILE* file);
CompiledProgram* input_highlevel(FILE* file);

#endif
