#ifndef container_h
#define container_h

#include <stddef.h>
#include <stdint.h>

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

ObjContainter* wrapString(char* string, size_t len);
ObjContainter* makeString(char* string, size_t len);

ObjContainter* concatenateStrings(const ObjContainter* strA, const ObjContainter* strB);
ObjContainter* multiplyString(const ObjContainter* str, intmax_t amount);

void free_container(ObjContainter* containter);
#endif
