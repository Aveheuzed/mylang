#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "identifiers_record.h"

#define GROW_THRESHHOLD (0.8) // TODO: benchmark this value

static void put_identifier(IdentifiersRecord *const pool, Identifier id) {
        const size_t mask = pool->len-1;
        hash_t index = id.hash;
        for (;
                pool->pool[mask&index].source != NULL;
                index++);
        pool->pool[mask&index] = id;
        pool->nb_entries++;
}

static IdentifiersRecord* growRecord(IdentifiersRecord* pool) {
        const size_t new_size = pool->len * 2;
        IdentifiersRecord *new_pool = calloc(offsetof(IdentifiersRecord, pool) + sizeof(Identifier)*new_size, 1);
        new_pool->len = new_size;
        for (size_t i=0; i<pool->len; i++) {
                Identifier id = pool->pool[i];
                if (id.source != NULL) put_identifier(new_pool, id);
        }
        freeRecord(pool);
        return new_pool;
}

IdentifiersRecord* allocateRecord(void) {
        IdentifiersRecord *const pool = calloc(offsetof(IdentifiersRecord, pool) + sizeof(Identifier)*1, 1);
        pool->len = 1;
        return pool;
}
void freeRecord(IdentifiersRecord* pool) {
        free(pool);
}

char* internalize(IdentifiersRecord **const record, char *const string, unsigned short length) {
        #define BUCKET_FULL(bucket) ((bucket).source != NULL)
        #define SAME_HASH(bucket, hsh) ((bucket).hash == (hsh))
        #define SAME_STRING(bucket, s2, l) (strncmp((bucket).source, (s2), (l)) == 0)
        #define SAME_CONTENT(bucket, hsh, s2, l) (SAME_HASH((bucket), (hsh)) && SAME_STRING((bucket), (s2), (l)))

        #define ADVANCE() (index = (index+1) & mask, (*record)->pool[index]) // careful, modifies local var `index`

        // I'd rather the pool doesn't change size in the middle of the function
        if ((*record)->len * GROW_THRESHHOLD <= (*record)->nb_entries)
                *record = growRecord(*record);

        const size_t mask = (*record)->len-1;
        const hash_t hash = hash_stringn(string, length);

        // Step 1: find wether the string is already internalized.

        hash_t index = hash;
        Identifier id;

        index = index & mask;
        id = (*record)->pool[index];
        while (BUCKET_FULL(id) && !SAME_CONTENT(id, hash, string, length) ) {
                id = ADVANCE();
        }

        if (BUCKET_FULL(id)) return id.source;

        // Step 2: it's not there, put it in.
        do {
                id = ADVANCE();
        } while (BUCKET_FULL(id));

        (*record)->pool[mask&index] = (Identifier) {.source=string, .hash=hash};
        (*record)->nb_entries++;

        return string;

        #undef BUCKET_FULL
        #undef SAME_HASH
        #undef SAME_STRING
        #undef SAME_CONTENT
}

#undef GROW_THRESHHOLD
