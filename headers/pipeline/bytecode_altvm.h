#ifndef bytecode_h
#define bytecode_h

typedef enum BFMode {
        MODE_COMPUTE=0,
        MODE_END,
        MODE_IN,
        MODE_OUT,
        MODE_CJUMPFWD,
        MODE_NCJUMPFWD,
        MODE_CJUMPBWD,
        MODE_NCJUMPBWD,
} BFMode;

/*
Modes :
* MODE_COMPUTE:
        bytecode is an array of <length> pairs of (shift, increment) representing a band shift (`<`, `>`) and a data increment (`+`, `-`).
* MODE_END: end of computation
* MODE_IN: `,`
* MODE_OUT: `.`
* MODE_CJUMPFWD : `[` with an argument in <length> (how much to jump)
* MODE_CJUMPBWD : `]` with an argument in <length> (how much to jump)
* MODE_NCJUMPFWD : `[` with an argument in the next <sizeof(size_t)> bytes
* MODE_NCJUMPBWD : `]` with an argument in the next <sizeof(size_t)> bytes
*/

typedef struct ControlByte {
        BFMode mode :3;
        unsigned char length :5;
} ControlByte;

typedef union Bytecode {
        int8_t byte;
        ControlByte control;
} Bytecode;

#define BF_MAX_RUN ((1<<5)-1)

typedef struct CompiledProgram {
        struct CompiledProgram* up;
        struct RawComputationArray* comput_arr;
        size_t len;
        size_t maxlen;
        Bytecode bytecode[];
} CompiledProgram;


#endif
