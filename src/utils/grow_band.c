#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// used internally by bf.s
uint8_t* growBand(uint8_t* ptr, const size_t oldlen, const size_t newlen) {
        ptr = reallocarray(ptr, newlen, sizeof(uint8_t));
        memset(ptr+oldlen, 0, (newlen-oldlen)*sizeof(uint8_t));
        return ptr;
}
