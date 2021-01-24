#ifndef object_h
#define object_h

#include <stdint.h>
#include "container.h"

typedef enum ObjType {
        TYPE_INT,
        TYPE_BOOL,
        TYPE_FLOAT,
        TYPE_STRING,

        LEN_OBJTYPES // do NOT add anything below this line!
} ObjType;

typedef struct Object {
        ObjType type;
        union {
                intmax_t intval;
                double floatval;
                ObjString* strval;
        };
} Object;

#endif
