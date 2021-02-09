#ifdef DEBUG
#include <stdio.h>
#define LOG(text, ...) printf("[%-22s]" text "\n", __func__, __VA_ARGS__)
#else
#define LOG(text, ...)
#endif
