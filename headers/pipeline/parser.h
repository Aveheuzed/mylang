#ifndef parser_h
#define parser_h

#include <stdint.h> // for the uintptr_t, may be needed in the use of Nodes

#include "headers/pipeline/token.h"
#include "headers/pipeline/node.h"

// tokens[] is supposed to end with a TOKEN_EOF
Node* parse_statement(Token **const tokens);

#endif
