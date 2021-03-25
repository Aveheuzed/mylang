#ifndef parser_h
#define parser_h

#include "headers/pipeline/state.h"
#include "headers/pipeline/node.h"

void mk_parser_info(parser_info *const prsinfo);
void del_parser_info(parser_info *const prsinfo);

Node* parse_statement(parser_info *const state);

#endif
