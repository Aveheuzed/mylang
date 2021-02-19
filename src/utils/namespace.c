#include <stdlib.h>
#include <stddef.h>

#include "headers/utils/namespace.h"

#define GROW_THRESHHOLD (0.8) // TODO: benchmark this value

#define BUCKET_FULL(bucket) ((bucket).key != NULL)

static void raw_ns_set_value(Namespace *const pool, Variable v) {
        const size_t mask = pool->len-1;
        hash_t index = v.hash & mask;
        for (;
                BUCKET_FULL(pool->pool[mask&index]) &&
                (pool->pool[mask&index].key != v.key);
                index++);
        pool->pool[mask&index] = v;
        pool->nb_entries++;
}

static Namespace* growNamespace(Namespace* pool) {
        const size_t new_size = pool->len * 2;

        LOG("Namespace grows from %lu to %lu entries, because it contains %lu variables", pool->len, new_size, pool->nb_entries);

        Namespace *new_pool = calloc(offsetof(Namespace, pool) + sizeof(Variable)*new_size, 1);
        new_pool->len = new_size;
        for (size_t i=0; i<pool->len; i++) {
                Variable v = pool->pool[i];
                if (v.key != NULL) raw_ns_set_value(new_pool, v);
        }
        free(pool);
        return new_pool;
}

Namespace* allocateNamespace(void) {
        Namespace *const pool = calloc(offsetof(Namespace, pool) + sizeof(Variable)*1, 1);
        pool->len = 1;
        return pool;
}
void freeNamespace(Namespace* pool) {
        free(pool);
}

void ns_set_value(Namespace **const pool, char *const key, Object value) {
        if ((*pool)->len * GROW_THRESHHOLD <= ((*pool)->nb_entries+1))
                *pool = growNamespace(*pool);
        raw_ns_set_value(*pool, (Variable){.key=key, .value=value});
}

Object* ns_get_value(Namespace *const pool, const char* key) {
        const hash_t mask = pool->len - 1;
        hash_t index = mask & (hash_t) key;
        Variable* v;
        do {
                v = &(pool->pool[(index++)&mask]);
                if (v->key == key) return &(v->value);
                if (!BUCKET_FULL(*v)) return NULL;
        } while (1);
}

#undef BUCKET_FULL
#undef GROW_THRESHHOLD
