#ifndef identifiers_record_h
#define identifiers_record_h

#include <stddef.h>

#include "hash.h"

typedef struct Identifier {
        char* source;
        hash_t hash;
} Identifier;

typedef struct IdentifiersRecord {
        size_t len; // guaranteed to be a power of 2
        size_t nb_entries;
        Identifier pool[];
} IdentifiersRecord;

IdentifiersRecord* allocateRecord(void);
void freeRecord(IdentifiersRecord* pool);

char* internalize(IdentifiersRecord **const record, char *const string, unsigned short length);

#endif
