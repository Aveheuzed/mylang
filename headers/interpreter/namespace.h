#ifndef identifiers_pool_h
#define identifiers_pool_h

#include <stddef.h>

#include "interpreter/object.h"

typedef struct Namespace {
        char const** keys; // may be NULL, indicating NS push
        Object* values;
        size_t nb_entries;
        size_t len;
        Object staging;
} Namespace;

Namespace allocateNamespace(void);
void freeNamespace(Namespace* ns);

size_t pushNamespace(Namespace *const ns);
void popNamespace(Namespace *const ns, size_t restore);

void ns_set_value(Namespace *const ns, char *const key, Object value);
Object* ns_get_value(Namespace *const ns, const char* key);
Object* ns_get_rw_value(Namespace *const pool, const char* key);

#endif
