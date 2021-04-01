#ifndef bytecode_h
#define bytecode_h

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

#define BF_MAX_RUN ((1<<5)-1)

typedef struct CompiledProgram {
        struct CompiledProgram* up;
        size_t len;
        size_t maxlen;
        CompressedBFOperator bytecode[];
} CompiledProgram;



#endif
