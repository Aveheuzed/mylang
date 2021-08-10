#ifndef parser_h
#define parser_h

#include "lexer.h"
#include "compiler/node.h"

typedef struct parser_info parser_info;

struct parser_info {
        struct lexer_info lxinfo;
        struct ResolverRecord* resolv;
        LocalizedToken last_produced;
        char stale; // state of the token, 1 if it needs to be refreshed
};

void mk_parser_info(parser_info *const prsinfo);
void del_parser_info(parser_info *const prsinfo);

Node* parse_statement(parser_info *const state);

#endif
