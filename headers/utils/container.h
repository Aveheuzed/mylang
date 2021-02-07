#ifndef container_h
#define container_h

#include <stdint.h>

typedef struct ObjString {
        intmax_t len;
        char value[];
} ObjString;

ObjString* makeString(const char* string, intmax_t len);

ObjString* concatenateStrings(const ObjString* strA, const ObjString* strB);
ObjString* multiplyString(const ObjString* str, intmax_t amount);

void free_string(ObjString* container);
#endif
