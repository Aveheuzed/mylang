#ifndef parser_h
#define parser_h

#include "lexer.h"
#include "compiler/node.h"
#include "compiler/runtime_types.h"

typedef struct parser_info parser_info;

struct parser_info {
        struct lexer_info lxinfo;
        struct ResolverRecord* resolv;
        LocalizedToken last_produced;
        char stale; // state of the token, 1 if it needs to be refreshed
};

void mk_parser_info(parser_info *const prsinfo);
void del_parser_info(parser_info *const prsinfo);

struct ResolverRecord* record_variable(struct ResolverRecord* record, char const* key, size_t arity, RuntimeType type);
Node* parse_statement(parser_info *const state);

#endif
