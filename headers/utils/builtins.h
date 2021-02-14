#ifndef builtins_h
#define builtins_h

#include "headers/utils/object.h"

Object print_value(const uintptr_t argc, const Object* obj);
Object native_clock(const uintptr_t argc, const Object* obj);

#endif
