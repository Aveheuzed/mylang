#ifndef parser_h
#define parser_h

#include <stdint.h> // for the uintptr_t, may be needed in the use of Nodes

#include "lexer.h"
#include "interpreter/node.h"

typedef struct parser_info {
        lexer_info lxinfo;
        LocalizedToken last_produced;
        unsigned int func_def_depth;
        char stale; // state of the token, 1 if it needs to be refreshed
} parser_info;

void mk_parser_info(parser_info *const prsinfo);
void del_parser_info(parser_info *const prsinfo);

Node* parse_statement(parser_info *const state);

#endif
