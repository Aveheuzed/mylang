#ifndef bytecode_h__sub
#define bytecode_h__sub
// this is the more refined version of bytecode.h in headers/compiler/
// contains the innner structures of the bytecode
#include "compiler/bytecode.h"

typedef enum BFOperator {
        BF_PLUS,
        BF_MINUS,
        BF_LEFT,
        BF_RIGHT,

        BF_CANCOMPRESS=BF_RIGHT, // not an actual operator

        // these __must not__ be compressed
        BF_INPUT,
        BF_OUTPUT,

        BF_NOOPERAND = BF_OUTPUT, // not an actual operator

        BF_JUMP_FWD,
        BF_JUMP_BWD,
} BFOperator;

/*
RLE compression.
A CBFO with run=0 signals EOF
*/

typedef struct CompressedBFOperator {
        BFOperator operator :3;
        unsigned char run :5;
} CompressedBFOperator;

typedef struct CompiledProgram {
        struct CompiledProgram* up;
        size_t len;
        size_t maxlen;
        CompressedBFOperator bytecode[];
} CompiledProgram;

#endif /* end of include guard: bytecode_h__sub */
