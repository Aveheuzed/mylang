#include <stdio.h>

#include "headers/utils/builtins.h"

void print_value(const Object obj) {
        switch (obj.type) {
                case TYPE_INT:
                        printf("%ld\n", obj.intval); break;
                case TYPE_BOOL:
                        printf("%s\n", obj.intval?"true":"false"); break;
                case TYPE_FLOAT:
                        printf("%f\n", obj.floatval); break;
                case TYPE_STRING:
                        printf("\"%s\"\n", obj.strval->value); break;
                case TYPE_NONE:
                        printf("none\n"); break;
        }
}