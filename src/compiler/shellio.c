#include <stdio.h>

#include "compiler/compiler.h"
#include "compiler/state.h"

#include "compiler/shellio.h"
#include "compiler/bytecode.h"

CompiledProgram* input_highlevel(FILE* file) {
        pipeline_state pipeline;
        mk_pipeline(&pipeline, file);

        while (compile_statement(&pipeline.cmpinfo));

        CompiledProgram *const pgm = get_bytecode(&pipeline);
        del_pipeline(&pipeline);

        return pgm;
}
