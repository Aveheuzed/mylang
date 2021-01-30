#ifndef interpreter_h
#define interpreter_h

#include "parser.h"
#include "object.h"
#include "namespace.h"

Object interpret(const Node* root, Namespace **const ns);

#endif
