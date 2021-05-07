#include <stdlib.h>
#include <stddef.h>

#include "headers/utils/namespace.h"

#define GROW_THRESHHOLD (0.8) // TODO: benchmark this value

#define BUCKET_FULL(bucket) ((bucket).key != NULL)

static inline hash_t hash(const char* key) {
        /* we have to remove the least significant bits, because that pointer
        was returned by `malloc`, and is therefore aligned for the largest types
        available. We take 16 bytes for "largest type available", to be safe.
        */
        return ((hash_t) key) >> 4;
}

static void raw_ns_set_value(Namespace *const pool, Variable v) {
        const size_t mask = pool->len-1;
        hash_t index = hash(v.key) & mask;
        LOG("%s expects to be put at %lu", v.key, index);
        for (;
                BUCKET_FULL(pool->pool[mask&index]) &&
                (pool->pool[mask&index].key != v.key);
                index++);
        LOG("Putting %s at index %lu", v.key, index&mask);
        if (pool->pool[mask&index].key != v.key) pool->nb_entries++;
        pool->pool[mask&index] = v;
}

static Namespace* growNamespace(Namespace* pool) {
        const size_t new_size = pool->len * 2;

        LOG("Namespace grows from %lu to %lu entries, because it contains %lu variables", pool->len, new_size, pool->nb_entries);

        Namespace *new_pool = calloc(offsetof(Namespace, pool) + sizeof(Variable)*new_size, 1);
        new_pool->len = new_size;
        new_pool->enclosing = pool->enclosing;
        for (size_t i=0; i<pool->len; i++) {
                Variable v = pool->pool[i];
                if (v.key != NULL) raw_ns_set_value(new_pool, v);
        }
        free(pool);
        return new_pool;
}

Namespace* allocateNamespace(Namespace **const enclosing) {
        Namespace *const pool = calloc(offsetof(Namespace, pool) + sizeof(Variable)*1, 1);
        pool->len = 1;
        pool->enclosing = enclosing;
        pool->returned = OBJ_NONE;
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
        LOG("Trying to get value %s", key);
        const hash_t mask = pool->len - 1;
        hash_t index = mask & hash(key);
        Variable* v;
        do {
                v = &(pool->pool[(index++)&mask]);
                if (v->key == key) {
                        LOG("Found");
                        return &(v->value);
                }
                if (!BUCKET_FULL(*v)) {
                        if (pool->enclosing != NULL) {
                                LOG("Recursing");
                                return ns_get_value(*(pool->enclosing), key);
                        }
                        else {
                                LOG("Not found");
                                return NULL;
                        }
                }
        } while (1);
}

Object* ns_get_rw_value(Namespace *const pool, const char* key) {
        LOG("Trying to get value %s", key);
        const hash_t mask = pool->len - 1;
        hash_t index = mask & hash(key);
        Variable* v;
        do {
                v = &(pool->pool[(index++)&mask]);
                if (v->key == key) {
                        LOG("Found");
                        return &(v->value);
                }
                if (!BUCKET_FULL(*v)) {
                        return NULL;
                }
        } while (1);
}

#undef BUCKET_FULL
#undef GROW_THRESHHOLD
