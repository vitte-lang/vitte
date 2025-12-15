#ifndef STEEL_JSON_H
#define STEEL_JSON_H

#include "../runtime/value.h"

char *steel_json_stringify(Value v);
Value steel_json_parse(const char *json);

#endif
