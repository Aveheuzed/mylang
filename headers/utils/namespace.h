#ifndef identifiers_pool_h
#define identifiers_pool_h

#include <stddef.h>

#include "headers/utils/hash.h"
#include "headers/utils/object.h"

typedef struct Variable {
        /* Since identifiers are internalized, we use the pointer to the string
        as hash digest for the identifier.
        We can therefore guarantee no collisions.
        Furthermore, the hashing is free! */
        union {
                char* key;
                hash_t hash;
        };
        Object value;
} Variable;

typedef struct Namespace {
        size_t len; // guaranteed to be a power of 2
        size_t nb_entries;
        struct Namespace** enclosing;
        Variable pool[];
} Namespace;

Namespace* allocateNamespace(Namespace **const enclosing);
void freeNamespace(Namespace* pool);

void ns_set_value(Namespace **const pool, char *const key, Object value);
Object* ns_get_value(Namespace *const pool, const char* key);

#endif
