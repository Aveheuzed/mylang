#ifndef interpreter_h
#define interpreter_h

#include "headers/pipeline/node.h"
#include "headers/utils/object.h"
#include "headers/utils/namespace.h"

Object interpretExpression(const Node* root, Namespace **const ns);
void interpretStatement(const Node* root, Namespace **const ns);

#endif
