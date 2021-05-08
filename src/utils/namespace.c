#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "headers/utils/namespace.h"

static void growNS(Namespace* ns) {
        const size_t new_size = ns->len * 2;

        LOG("Namespace grows from %lu to %lu entries, because it contains %lu variables", ns->len, new_size, ns->nb_entries);

        ns->keys = reallocarray(ns->keys, new_size, sizeof(ns->keys[0]));
        ns->values = reallocarray(ns->values, new_size, sizeof(ns->values[0]));

        ns->len = new_size;
}

Namespace allocateNamespace(void) {
        static const size_t ns_start_len = 16;

        Namespace new;

        new.keys = calloc(sizeof(new.keys[0]), ns_start_len);
        new.values = calloc(sizeof(new.values[0]), ns_start_len);
        new.nb_entries = 0;
        new.len = ns_start_len;

        pushNamespace(&new);

        return new;
}
void freeNamespace(Namespace* ns) {
        free(ns->keys);
        free(ns->values);
}

void ns_set_value(Namespace *const ns, char *const key, Object value) {
        // tunable according to your cache size
        static const unsigned lookupbuffer = 64/(2*sizeof(ns->keys[0]));

        for (unsigned i=0; i<lookupbuffer; i++) {
                if (ns->keys[ns->nb_entries-1-i] == NULL) break;
                if (ns->keys[ns->nb_entries-1-i] == key) {
                        ns->values[ns->nb_entries-1-i] = value;
                        return;
                }
        }

        if (ns->len <= ns->nb_entries) growNS(ns);

        ns->keys[ns->nb_entries] = key;
        ns->values[ns->nb_entries] = value;

        ns->nb_entries++;
}

Object* ns_get_value(Namespace *const ns, const char* key) {
        size_t entry = ns->nb_entries;

        do {
                entry--;
                if (ns->keys[entry] == key) return &(ns->values[entry]);
        } while (entry != 0);

        return NULL;
}

Object* ns_get_rw_value(Namespace *const ns, const char* key) {
        size_t entry = ns->nb_entries;

        do {
                entry--;
                if (ns->keys[entry] == NULL) break;
                if (ns->keys[entry] == key) return &(ns->values[entry]);
        } while (entry != 0);

        return NULL;
}

size_t pushNamespace(Namespace *const ns) {
        if (ns->len <= ns->nb_entries) growNS(ns);
        ns->nb_entries++;
        ns->keys[ns->nb_entries] = NULL;
        return ns->nb_entries-1;
}
void popNamespace(Namespace *const ns, size_t restore) {
        ns->nb_entries = restore;
}
