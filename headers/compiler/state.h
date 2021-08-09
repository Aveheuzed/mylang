#ifndef state_h
#define state_h

#include <stdio.h>
#include <stddef.h>

#include "token.h"

typedef struct pipeline_state   pipeline_state;

typedef struct lexer_info       lexer_info;
typedef struct parser_info    parser_info;
typedef struct compiler_info    compiler_info;

// IdentifiersRecord defined in identifiers_record.h
// ResolverRecord defined in parser.c
// CompiledProgram defined in bytecode.h
// BFMemoryView defined in mm.h
// BFNamespace defined in compiler_helpers.h

struct pipeline_state {
        struct compiler_info {
                struct parser_info {
                        struct lexer_info {
                                FILE* file;
                                struct IdentifiersRecord* record;
                                Localization pos;
                        } lxinfo;
                        struct ResolverRecord* resolv;
                        LocalizedToken last_produced;
                        char stale; // state of the token, 1 if it needs to be refreshed
                } prsinfo;
                struct CompiledProgram* program;
                struct BFMemoryView* memstate;
                struct BFNamespace* currentNS;
                size_t current_pos; // not necessarily enough (var len str)
                unsigned int code_isnonlinear;
        } cmpinfo;
};

void mk_pipeline(pipeline_state *const state, FILE* file);
void del_pipeline(pipeline_state *const state);

#endif
