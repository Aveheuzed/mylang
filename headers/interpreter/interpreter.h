#ifndef interpreter_h
#define interpreter_h

#include "interpreter/parser.h"
#include "interpreter/namespace.h"

typedef enum {
        OK_OK,
        OK_ABORT,
        ERROR_ABORT,
} errcode;

typedef struct interpreter_info interpreter_info;

struct interpreter_info {
        parser_info prsinfo;
        Namespace ns;
};

void mk_interpreter_info(interpreter_info *const interp);
void del_interpreter_info(interpreter_info *const interp);

errcode interpretStatement(interpreter_info *const interpinfo);

#endif
