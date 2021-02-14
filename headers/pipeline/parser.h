#ifndef parser_h
#define parser_h

#include <stdint.h> // for the uintptr_t, may be needed in the use of Nodes

#include "headers/pipeline/lexer.h"
#include "headers/pipeline/node.h"

typedef struct parser_info {
        lexer_info lxinfo;
        Token last_produced;
} parser_info;

parser_info mk_parser_info(FILE* file);
void del_parser_info(parser_info prsinfo);

Node* parse_statement(parser_info *const state);

#endif
