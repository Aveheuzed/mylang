#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "headers/utils/container.h"

//TODO: implement a gc
void free_string(ObjString* container) {
        free(container);
}

static ObjString* allocate_string(const intmax_t len) {
        // allocates an ObjString big enough to contain a string of length <len> (not counting the final '\0').
        return malloc(offsetof(ObjString, value) + sizeof(char)*(len+1));
}

ObjString* makeString(const char* string, intmax_t len) {
        // WARNING : hidden malloc()
        ObjString *const container = allocate_string(len);
        container->len = len;
        strncpy(container->value, string, len); // we need strncpy here cuz the string buffer may not be null-terminated (source code buffer)
        return container;
}

ObjString* concatenateStrings(const ObjString* strA, const ObjString* strB) {
        // WARNING : hidden malloc()
        const intmax_t len_total = strA->len + strB->len;
        ObjString *const container = allocate_string(len_total);
        container->len = len_total;
        strcpy(container->value, strA->value);
        strcpy(container->value+(strA->len), strB->value);
        return container;
}
ObjString* multiplyString(const ObjString* str, intmax_t amount) {
        // WARNING : hidden malloc()
        const intmax_t len_total = str->len*amount;
        ObjString *const dest = allocate_string(len_total);
        for (char* i=dest->value; amount>0; (amount--, i+=str->len)) {
                strcpy(i, str->value);
        }
        return dest;
}
