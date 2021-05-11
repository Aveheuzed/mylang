#ifndef interpreter_h
#define interpreter_h

#include "headers/pipeline/parser.h"
#include "headers/utils/namespace.h"

typedef enum {
        OK_OK,
        OK_ABORT,
        ERROR_ABORT,
} errcode;

errcode interpretStatement(parser_info *const prsinfo, Namespace *const ns);

#endif
