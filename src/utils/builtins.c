#include <stdio.h>
#include <time.h>

#include "headers/utils/builtins.h"
#include "headers/utils/error.h"

Object print_value(const uintptr_t argc, const Object* obj) {
        if (argc != 1) {
                ArityError(1, argc);
                return ERROR;
        }
        switch (obj[0].type) {
                case TYPE_INT:
                        printf("%ld\n", obj[0].intval); break;
                case TYPE_BOOL:
                        printf("%s\n", obj[0].intval?"true":"false"); break;
                case TYPE_FLOAT:
                        printf("%f\n", obj[0].floatval); break;
                case TYPE_STRING:
                        printf("\"%s\"\n", obj[0].strval->value); break;
                case TYPE_NONE:
                        printf("none\n"); break;
                case TYPE_NATIVEF:
                        printf("<function>\n"); break;
                default:
                        printf("--unrecognized object!\n"); break;
        }
        return OBJ_NONE;
}

Object native_clock(const uintptr_t argc, const Object* obj) {
        if (argc) {
                ArityError(0, argc);
                return ERROR;
        }
        return (Object) {.type=TYPE_FLOAT, .floatval=((double)clock())/(CLOCKS_PER_SEC)};
}
