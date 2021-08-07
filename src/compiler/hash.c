#include "compiler/hash.h"

// constants for FNV-1A
#ifdef __GNUC__ // GCC defines convenient macros, let's use them!

#ifdef __LP64__ // values for 64-bits arch
#define HASH_BASE 0xcbf29ce484222325
#define HASH_MUL 0x00000100000001B3
#else // values for 32-bits arch
#define HASH_BASE 0x811c9dc5
#define HASH_MUL 0x01000193
#endif

#else // if we aren't compiled by GCC, we assume we're on a 32-bits machine
#define HASH_BASE 0x811c9dc5
#define HASH_MUL 0x01000193
#endif


// FNV-1A; from https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

hash_t resume_hashing(char const* string, hash_t hash) {
        // uint64_t hash = HASH_BASE;
        for (;*string; string++) {
                hash ^= *string;
                hash *= HASH_MUL;
        }
        return hash;
}
hash_t hash_stringn(char const* string, const unsigned short len) {
        // designed to hash token strings, therefore `unsigned short` instead of `intmax_t`
        hash_t hash = HASH_BASE;
        for (unsigned short i=0; i<len; i++) {
                hash ^= string[i];
                hash *= HASH_MUL;
        }
        return hash;
}
hash_t hash_string(char const* string) {
        return resume_hashing(string, 0xcbf29ce484222325);
}

#undef HASH_BASE
#undef HASH_MUL
