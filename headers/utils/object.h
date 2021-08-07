#ifndef object_h
#define object_h

#include <stdint.h>

#include "utils/string.h"
#include "utils/function.h"

typedef struct Object Object;

typedef Object (native_function)(const uintptr_t argc, const Object* argv);

typedef enum ObjType {
        TYPE_ERROR,
        TYPE_INT,
        TYPE_BOOL,
        TYPE_NONE,
        TYPE_FLOAT,
        TYPE_STRING,
        TYPE_NATIVEF,
        TYPE_USERF,

        LEN_OBJTYPES // do NOT add anything below this line!
} ObjType;

typedef union ObjectCore {
        intmax_t intval;
        double floatval;
        ObjString* strval;
        ObjFunction* funval;
        native_function* natfunval;
} ObjectCore;

struct Object {
        ObjType type;
        union {
                intmax_t intval;
                double floatval;
                ObjString* strval;
                ObjFunction* funval;
                native_function* natfunval;
        };
};

#define ERROR_GUARD(obj) if((obj).type == TYPE_ERROR) return ERROR

#define ERROR ((Object){.type=TYPE_ERROR})
#define OBJ_NONE ((Object){.type=TYPE_NONE})
#define OBJ_TRUE ((Object){.type=TYPE_BOOL, .intval=1})
#define OBJ_FALSE ((Object){.type=TYPE_BOOL, .intval=0})

#endif
