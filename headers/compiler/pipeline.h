#ifndef pipeline_h
#define pipeline_h

#include <stdio.h>

#include "compiler/compiler.h"
#include "compiler/bytecode.h"
#include "keywords.h"

typedef struct pipeline_state pipeline_state;

struct pipeline_state { // redirects to the last stage of the pipeline
        compiler_info cmpinfo;
};

// keywords: NULL-terminated
void mk_pipeline(pipeline_state *const state, FILE* file, const Keyword* keywords);
void del_pipeline(pipeline_state *const state);
CompiledProgram* get_bytecode(pipeline_state *const state);

#endif
