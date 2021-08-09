#ifndef parser_h
#define parser_h

#include "compiler/state.h"
#include "compiler/node.h"

void mk_parser_info(parser_info *const prsinfo);
void del_parser_info(parser_info *const prsinfo);

Node* parse_statement(parser_info *const state);

#endif
