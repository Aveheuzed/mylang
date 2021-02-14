#ifndef object_h
#define object_h

#include <stdint.h>

#include "headers/utils/container.h"

typedef struct Object Object;

typedef Object (native_function)(const uintptr_t argc, const Object* argv);

typedef enum ObjType {
        TYPE_INT,
        TYPE_BOOL,
        TYPE_NONE,
        TYPE_FLOAT,
        TYPE_STRING,
        TYPE_NATIVEF,

        LEN_OBJTYPES // do NOT add anything below this line!
} ObjType;

struct Object {
        ObjType type;
        union {
                intmax_t intval;
                double floatval;
                ObjString* strval;
                native_function* natfunval;
        };
};

#endif
