#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "headers/utils/namespace.h"

typedef struct Variable {
        /* Since identifiers are internalized, we use the pointer to the string
        to compute the hash digest for the identifier.
        This way, we don't need to iterate over the string.*/
        char* key;
        Object value;
} Variable;

typedef struct NS_pool {
        size_t len; // guaranteed to be a power of 2
        size_t nb_entries;
        Variable pool[];
} NS_pool;

static inline hash_t hash(const char* key) {
        /* we have to remove the least significant bits, because that pointer
        was returned by `malloc`, and is therefore aligned for the largest types
        available. We take 16 bytes for "largest type available", to be safe.
        */
        return ((hash_t) key) >> 4;
}

static void raw_ns_set_value(NS_pool *const pool, Variable v) {
        const size_t mask = pool->len-1;
        hash_t index = hash(v.key) & mask;
        LOG("%s expects to be put at %lu", v.key, index);
        for (;
                (pool->pool[mask&index].key !=  NULL) &&
                (pool->pool[mask&index].key != v.key);
                index++);
        LOG("Putting %s at index %lu", v.key, index&mask);
        if (pool->pool[mask&index].key == NULL) pool->nb_entries++;
        pool->pool[mask&index] = v;
}

static Object* raw_ns_get_value(NS_pool *const pool, const char* key) {
        const hash_t mask = pool->len - 1;
        hash_t index = mask & hash(key);
        Variable* v;
        do {
                v = &(pool->pool[(index++)&mask]);
                if (v->key == key) return &(v->value);
                if (v->key == NULL) return NULL;
        } while (1);
}

static NS_pool* growPool(NS_pool* pool) {
        const size_t new_size = pool->len * 2;

        LOG("Namespace grows from %lu to %lu entries, because it contains %lu variables", pool->len, new_size, pool->nb_entries);

        NS_pool *new_pool = calloc(offsetof(NS_pool, pool) + sizeof(Variable)*new_size, 1);
        new_pool->len = new_size;
        for (size_t i=0; i<pool->len; i++) {
                Variable v = pool->pool[i];
                if (v.key != NULL) raw_ns_set_value(new_pool, v);
        }
        free(pool);
        return new_pool;
}

static inline NS_pool* fuse_pools(Namespace *const ns) {
        if (ns->ro != NULL) {
                const size_t size = offsetof(NS_pool, pool) + sizeof(Variable) * ns->ro->len;
                NS_pool* target = malloc(size);
                memcpy(target, ns->ro, size);

                while (target->nb_entries + ns->rw->nb_entries >= target->len)
                        target = growPool(target);

                for (size_t i=0; i<ns->rw->len; i++) {
                        Variable v = ns->rw->pool[i];
                        if (v.key != NULL) raw_ns_set_value(target, v);
                }
                return target;
        }
        else {
                const size_t size = offsetof(NS_pool, pool) + sizeof(Variable) * ns->rw->len;
                NS_pool *const target = malloc(size);
                memcpy(target, ns->rw, size);
                return target;
        }
}

Namespace allocateNamespace(Namespace *const enclosing) {
        static const size_t ns_start_len = 1;

        Namespace new;

        new.rw = calloc(offsetof(NS_pool, pool) + sizeof(Variable) * ns_start_len, 1);
        new.rw->len = ns_start_len;

        if (enclosing == NULL) new.ro = NULL;
        else new.ro = fuse_pools(enclosing);
        new.returned = OBJ_NONE;

        return new;
}
void freeNamespace(Namespace* ns) {
        free(ns->ro);
        free(ns->rw);
}

void ns_set_value(Namespace *const ns, char *const key, Object value) {
        static const float grow_threshhold = 0.8;
        if (ns->rw->len * grow_threshhold <= (ns->rw->nb_entries+1))
                ns->rw = growPool(ns->rw);

        raw_ns_set_value(ns->rw, (Variable){.key=key, .value=value});
}

Object* ns_get_value(Namespace *const ns, const char* key) {
        if (ns->ro == NULL) return raw_ns_get_value(ns->rw, key);

        Object* value = NULL;
        if (value == NULL) value = raw_ns_get_value(ns->rw, key);
        if (value == NULL) value = raw_ns_get_value(ns->ro, key);
        return value;
}

Object* ns_get_rw_value(Namespace *const ns, const char* key) {
        return raw_ns_get_value(ns->rw, key);
}
