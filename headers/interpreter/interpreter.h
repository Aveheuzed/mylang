#ifndef interpreter_h
#define interpreter_h

#include "interpreter/parser.h"
#include "interpreter/namespace.h"

typedef enum {
        OK_OK,
        OK_ABORT,
        ERROR_ABORT,
} errcode;

errcode interpretStatement(parser_info *const prsinfo, Namespace *const ns);

#endif