#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "interpreter/builtins.h"
#include "interpreter/error.h"

Object print_value(const uintptr_t argc, const Object* obj) {
        for (uintptr_t iarg=0; iarg<argc; iarg++) {
                if (obj[iarg].type == TYPE_STRING) {
                        printf("\"%s\"\n", obj[iarg].strval->value);
                }
                else {
                        Object o = tostring(1, &obj[iarg]);
                        if (o.type == TYPE_ERROR) {
                                return o;
                        } else {
                                puts(o.strval->value);
                                free_string(o.strval);
                        }
                }
        }
        return OBJ_NONE;
}

Object native_clock(const uintptr_t argc, const Object* obj) {
        if (argc) return ERROR;
        return (Object) {.type=TYPE_FLOAT, .floatval=((double)clock())/(CLOCKS_PER_SEC)};
}

Object input(const uintptr_t argc, const Object* obj) {
        char* string = NULL;
        size_t bufsize = 0;
        ssize_t length;
        Object result = ERROR;
        switch (argc) {
                case 1:
                        print_value(argc, obj);
                case 0:
                        length = getline(&string, &bufsize, stdin);
                        if (length == -1) length = 0;
                        else if (length > 0) length--; // remove final CRLF
                        result = (Object) {.type=TYPE_STRING, .strval=makeString(string, length)};
                        free(string);
                        break;
                default:
                        break;
        }
        return result;

}

Object tostring(const uintptr_t argc, const Object* obj) {
        if (argc != 1) return ERROR;
        static const size_t buflen = 256;
        char result[buflen];
        int len;
        switch (obj->type) {
                case TYPE_INT:
                        len = snprintf(result, buflen, "%ld", obj->intval);
                        break;
                case TYPE_BOOL:
                        len = snprintf(result, buflen, obj->intval?"true":"false");
                        break;
                case TYPE_FLOAT:
                        len = snprintf(result, buflen, "%f", obj->floatval);
                        break;
                case TYPE_NONE:
                        len = snprintf(result, buflen, "none");
                        break;
                case TYPE_STRING:
                        return *obj;
                case TYPE_NATIVEF:
                        len = snprintf(result, buflen, "<built-in function>");
                        break;
                case TYPE_USERF:
                        len = snprintf(result, buflen, "<user-defined function>");
                        break;
                default:
                        return ERROR;
        }
        return (Object){.type=TYPE_STRING, .strval=makeString(result, len)};
}
Object tobool(const uintptr_t argc, const Object* obj) {
        if (argc != 1) return ERROR;
        switch (obj->type) {
                case TYPE_INT:
                        return obj->intval ? OBJ_TRUE : OBJ_FALSE;
                case TYPE_BOOL:
                        return *obj;
                case TYPE_FLOAT:
                        return ERROR;
                case TYPE_NONE:
                        return OBJ_FALSE;
                case TYPE_STRING:
                        return obj->strval->len ? OBJ_TRUE : OBJ_FALSE;
                case TYPE_NATIVEF:
                        return OBJ_TRUE;
                default:
                        return ERROR;
        }
}
Object toint(const uintptr_t argc, const Object* obj) {
        if (argc != 1) return ERROR;
        switch (obj->type) {
                case TYPE_INT:
                        return *obj;
                case TYPE_BOOL:
                        return (Object) {.type=TYPE_INT, .intval=obj->intval};
                case TYPE_FLOAT:
                        return (Object) {.type=TYPE_INT, .intval=obj->floatval};
                case TYPE_NONE:
                        return ERROR;
                case TYPE_STRING:
                        if (obj->strval->len) {
                                char* end;
                                intmax_t result = strtoll(obj->strval->value, &end, 10);
                                if (end - obj->strval->value == obj->strval->len) {
                                        return (Object) {.type=TYPE_INT, .intval=result};
                                }
                        }
                        return ERROR;
                case TYPE_NATIVEF:
                        return ERROR;
                default:
                        return ERROR;
        }
}
Object tofloat(const uintptr_t argc, const Object* obj) {
        if (argc != 1) return ERROR;
        switch (obj->type) {
                case TYPE_INT:
                        return (Object) {.type=TYPE_FLOAT, .floatval=obj->intval};
                case TYPE_BOOL:
                        return (Object) {.type=TYPE_FLOAT, .floatval=obj->intval};
                case TYPE_FLOAT:
                        return *obj;
                case TYPE_NONE:
                        return ERROR;
                case TYPE_STRING:
                        if (obj->strval->len) {
                                char* end;
                                double result = strtod(obj->strval->value, &end);
                                if (end - obj->strval->value == obj->strval->len) {
                                        return (Object) {.type=TYPE_FLOAT, .floatval=result};
                                }
                        }
                        return ERROR;
                case TYPE_NATIVEF:
                        return ERROR;
                default:
                        return ERROR;
        }
}
