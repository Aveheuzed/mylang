#ifndef function_h
#define function_h

#include <stddef.h>

typedef struct ObjFunction {
        struct Node* body;
        size_t arity;
        char* arguments[];
} ObjFunction;

ObjFunction* createFunction(const size_t arity);
ObjFunction* reallocFunction(ObjFunction* fun, const size_t arity);
void free_function(ObjFunction* function);

#endif
