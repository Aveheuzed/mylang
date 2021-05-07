#ifndef identifiers_pool_h
#define identifiers_pool_h

#include <stddef.h>

#include "headers/utils/hash.h"
#include "headers/utils/object.h"

typedef struct Namespace {
        struct NS_pool* rw;
        struct NS_pool* ro;
        Object returned;
} Namespace;

Namespace allocateNamespace(Namespace *const enclosing);
void freeNamespace(Namespace* ns);

void ns_set_value(Namespace *const ns, char *const key, Object value);
Object* ns_get_value(Namespace *const ns, const char* key);
Object* ns_get_rw_value(Namespace *const pool, const char* key);

#endif
