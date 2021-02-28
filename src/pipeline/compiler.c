#include <stdlib.h>
#include <stddef.h>

#include "headers/pipeline/node.h"
#include "headers/pipeline/compiler.h"

static CompiledProgram* growProgram(CompiledProgram* ptr, const size_t newsize) {
        ptr = realloc(ptr, offsetof(CompiledProgram, bytecode) + sizeof(CompressedBFOperator)*newsize);
        ptr->maxlen = newsize;
        return ptr;
}

compiler_info mk_compiler_info(FILE* file) {
        compiler_info cmpinfo = (compiler_info) {.prsinfo=mk_parser_info(file), .program=growProgram(NULL, 0)};
        cmpinfo.program->len = 0;
        return cmpinfo;
}

void del_compiler_info(compiler_info cmpinfo) {
        free(cmpinfo.program);
}

int compile_statement(compiler_info *const state) {
        Node* node = parse_statement(&(state->prsinfo));
        if (node == NULL) return 0;
        else {
                // TODO
        }
        return 1;
}
