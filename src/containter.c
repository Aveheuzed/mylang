#include <stdlib.h>
#include <string.h>

#include "container.h"

//TODO: implement a gc
static ObjContainter* allocate_container(void) {
        return malloc(sizeof(ObjContainter));
}
void free_container(ObjContainter* container) {
        switch (container->type) {
                case CONTENT_STRING:
                        free(container->strval);
                default:
                        exit(-1);
        }
        free(container);
}

ObjContainter* wrapString(char* string, size_t len) {
        // WARNING : hidden malloc()
        ObjContainter *const container = allocate_container();
        *container = (ObjContainter){.type=CONTENT_STRING, .len=len, .strval=string};
        return container;
}
ObjContainter* makeString(char* string, size_t len) {
        // WARNING : hidden malloc()
        return wrapString(strndup(string, len), len);
}

ObjContainter* concatenateStrings(const ObjContainter* strA, const ObjContainter* strB) {
        // WARNING : hidden malloc()
        const size_t len_total = strA->len + strB->len;
        char *const dest = malloc(len_total + 1);
        strcpy(dest, strA->strval);
        strcpy(dest+strA->len, strB->strval);
        return wrapString(dest, len_total);
}
ObjContainter* multiplyString(const ObjContainter* str, intmax_t amount) {
        // WARNING : hidden malloc()
        const size_t len_total = str->len*amount;
        char *const dest = malloc(len_total+1);
        for (char* i=dest; amount>0; (amount--, i+=str->len)) {
                strcpy(i, str->strval);
        }
        return wrapString(dest, len_total);
}
