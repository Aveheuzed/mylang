#ifndef runtime_types_h
#define runtime_types_h

typedef enum RuntimeType {
        TYPEERROR=0,
        TYPE_INT,
        TYPE_STR,
        TYPE_VOID,

        LEN_TYPES=TYPE_VOID
} RuntimeType;

#endif
