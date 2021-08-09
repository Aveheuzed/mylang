#ifndef bytecode_h
#define bytecode_h
// generic bytecode API, for when you don't need to know the internals

#include <stdio.h>

// opaque object used by bytecode.c and bf.c
struct CompiledProgram;
typedef struct CompiledProgram CompiledProgram;
#define BF_MAX_RUN ((1<<5)-1)

CompiledProgram* createProgram(void);
void freeProgram(CompiledProgram* pgm);

CompiledProgram* emitLeftRight(CompiledProgram* program, ssize_t amount);
CompiledProgram* emitPlusMinus(CompiledProgram* program, ssize_t amount);
CompiledProgram* emitIn(CompiledProgram* program);
CompiledProgram* emitOut(CompiledProgram* program);
CompiledProgram* emitOpeningBracket(CompiledProgram* program);
CompiledProgram* emitClosingBracket(CompiledProgram* program);
CompiledProgram* emitEnd(CompiledProgram* program);

void output_bf(FILE* file, const CompiledProgram* pgm);
void output_cbf(FILE* file, const CompiledProgram* pgm);

CompiledProgram* input_cbf(FILE* file);

void execute(const CompiledProgram* pgm);

#endif
