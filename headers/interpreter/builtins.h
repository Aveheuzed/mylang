#ifndef builtins_h
#define builtins_h

#include "interpreter/object.h"

Object print_value(const uintptr_t argc, const Object* obj);
Object native_clock(const uintptr_t argc, const Object* obj);
Object input(const uintptr_t argc, const Object* obj);

Object tostring(const uintptr_t argc, const Object* obj);
Object tobool(const uintptr_t argc, const Object* obj);
Object toint(const uintptr_t argc, const Object* obj);
Object tofloat(const uintptr_t argc, const Object* obj);

#endif
