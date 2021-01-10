#ifndef object_h
#define object_h

#include <stdint.h>
#include "container.h"

typedef enum ObjType {
        TYPE_INT,
        TYPE_BOOL,
        TYPE_FLOAT,
        TYPE_CONTAINER,
} ObjType;

typedef struct Object {
        ObjType type;
        union {
                intmax_t intval;
                double floatval;
                ObjContainter* payload;
        };
} Object;

#endif
