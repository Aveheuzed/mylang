#ifndef interpreter_h
#define interpreter_h

#include "parser.h"
#include <stdint.h>
#include <stddef.h>

typedef enum ObjType {
        TYPE_INT,
        TYPE_BOOL,
        TYPE_FLOAT,
        TYPE_CONTAINER,
} ObjType;

typedef enum ObjContainterType {
        CONTENT_STRING,
} ObjContainterType;

typedef struct ObjContainter {
        ObjContainterType type;
        size_t len;
        union {
                char* strval;
        };
} ObjContainter;

typedef struct Object {
        ObjType type;
        union {
                intmax_t intval;
                double floatval;
                ObjContainter* payload;
        };
} Object;

typedef Object (*InterpretFn)(const Node* root);

Object interpret(const Node* root);

#endif
