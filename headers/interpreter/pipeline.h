#ifndef pipeline_h
#define pipeline_h

#include <stdio.h>

#include "interpreter/interpreter.h"

typedef struct pipeline_state pipeline_state;

struct pipeline_state { // redirects to the last stage of the pipeline
        interpreter_info interpinfo;
};

void mk_pipeline(pipeline_state *const state, FILE* file);
void del_pipeline(pipeline_state *const state);

#endif
