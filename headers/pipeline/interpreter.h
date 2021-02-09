#ifndef interpreter_h
#define interpreter_h

#include "headers/pipeline/parser.h"
#include "headers/utils/namespace.h"

int interpretStatement(parser_info *const prsinfo, Namespace **const ns);

#endif
