#ifndef interpreter_h
#define interpreter_h

#include "parser.h"
#include <stdint.h>

typedef intmax_t Object;

typedef Object (*InterpretFn)(const Node* root);

Object interpret(const Node* root);

#endif
