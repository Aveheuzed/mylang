#ifndef error_h
#define error_h

#include <stdio.h>

#define NOTIMPLEMENTED() (puts("Operation not implemented."), exit(-1))
#define TYPEERROR() (puts("Attempt to perform an operation on incompatible types"), exit(-1))
#define RUNTIMEERROR() (puts("Runtime error !"), exit(-1))
#define ARITYERROR(expected, actual) (printf("Arity error : expected %u argument(s), got %lu.\n", expected, actual))

#endif
