#ifndef interpreter_h
#define interpreter_h

#include "parser.h"
#include "object.h"

Object interpret(const Node* root);

#endif
