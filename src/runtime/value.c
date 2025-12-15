#include "../../include/runtime/value.h"
#include <stdlib.h>
#include <string.h>

Value value_number(double num) {
    Value v;
    v.type = VALUE_NUMBER;
    v.as.number = num;
    return v;
}

Value value_string(const char *str) {
    Value v;
    v.type = VALUE_STRING;
    v.as.string = malloc(strlen(str) + 1);
    strcpy(v.as.string, str);
    return v;
}

Value value_bool(int b) {
    Value v;
    v.type = VALUE_BOOL;
    v.as.boolean = b;
    return v;
}

Value value_nil(void) {
    Value v;
    v.type = VALUE_NIL;
    return v;
}
