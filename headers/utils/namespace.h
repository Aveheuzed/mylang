#ifndef identifiers_pool_h
#define identifiers_pool_h

#include <stddef.h>

#include "headers/utils/object.h"

typedef struct Namespace {
        struct {
                char const* key;
                size_t level;
        } *keys;
        Object* values;
        size_t nb_entries;
        size_t len;
        size_t current_level;
        Object staging;
} Namespace;

Namespace allocateNamespace(void);
void freeNamespace(Namespace* ns);

void pushNamespace(Namespace *const ns);
void popNamespace(Namespace *const ns);

void ns_set_value(Namespace *const ns, char *const key, Object value);
Object* ns_get_value(Namespace *const ns, const char* key);
Object* ns_get_rw_value(Namespace *const pool, const char* key);

#endif
