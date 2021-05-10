#include <stdlib.h>

#include "headers/pipeline/node.h"
#include "headers/utils/function.h"

ObjFunction* createFunction(const size_t arity) {
        return malloc(offsetof(ObjFunction, arguments) + sizeof(char*)*arity);
}
ObjFunction* reallocFunction(ObjFunction* fun, const size_t arity) {
        return realloc(fun, offsetof(ObjFunction, arguments) + sizeof(char*)*arity);
}
void free_function(ObjFunction* function) {
        free(function);
}
