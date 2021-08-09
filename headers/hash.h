#ifndef hash_h
#define hash_h

#include <stdint.h>

typedef uintptr_t hash_t;

hash_t hash_string(char const* string);
hash_t hash_stringn(char const* string, const unsigned short len);
hash_t resume_hashing(char const* string, hash_t hash);

#endif
