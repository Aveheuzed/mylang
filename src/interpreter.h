#ifndef interpreter_h
#define interpreter_h

#include "parser.h"
#include <stdint.h>

typedef enum ObjType {
        TYPE_INT,
        TYPE_BOOL,
        TYPE_FLOAT,
        TYPE_STR,
} ObjType;

typedef struct Object {
        ObjType type;
        union {
                intmax_t intval;
                double floatval;
                char* strval;
        };
} Object;

typedef Object (*InterpretFn)(const Node* root);

Object interpret(const Node* root);

#endif
