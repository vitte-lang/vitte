#include "builtin.h"
#include "../include/vitte_types.h"
#include "../include/vitte_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* String functions */
vitte_value_t builtin_strlen(vitte_value_t *args, int arg_count) {
    if (arg_count < 1 || !vitte_is_string(args[0])) {
        return vitte_int(0);
    }
    return vitte_int(args[0].as.string.length);
}

vitte_value_t builtin_substr(vitte_value_t *args, int arg_count) {
    if (arg_count < 3) return vitte_nil();
    if (!vitte_is_string(args[0])) return vitte_nil();
    if (!vitte_is_int(args[1]) || !vitte_is_int(args[2])) return vitte_nil();
    
    const char *str = args[0].as.string.data;
    int64_t start = args[1].as.integer;
    int64_t len = args[2].as.integer;
    
    if (start < 0 || len < 0 || start + len > args[0].as.string.length) {
        return vitte_nil();
    }
    
    char *result = (char *)malloc(len + 1);
    strncpy(result, str + start, len);
    result[len] = '\0';
    
    vitte_value_t v = vitte_string(result);
    free(result);
    return v;
}

vitte_value_t builtin_strcat(vitte_value_t *args, int arg_count) {
    if (arg_count < 2) return vitte_nil();
    
    size_t total_len = 0;
    for (int i = 0; i < arg_count; i++) {
        if (vitte_is_string(args[i])) {
            total_len += args[i].as.string.length;
        }
    }
    
    char *result = (char *)malloc(total_len + 1);
    char *ptr = result;
    
    for (int i = 0; i < arg_count; i++) {
        if (vitte_is_string(args[i])) {
            strcpy(ptr, args[i].as.string.data);
            ptr += args[i].as.string.length;
        }
    }
    *ptr = '\0';
    
    vitte_value_t v = vitte_string(result);
    free(result);
    return v;
}

vitte_value_t builtin_strtoupper(vitte_value_t *args, int arg_count) {
    if (arg_count < 1 || !vitte_is_string(args[0])) {
        return vitte_nil();
    }
    
    const char *str = args[0].as.string.data;
    char *result = (char *)malloc(strlen(str) + 1);
    
    for (int i = 0; str[i]; i++) {
        result[i] = toupper(str[i]);
    }
    result[strlen(str)] = '\0';
    
    vitte_value_t v = vitte_string(result);
    free(result);
    return v;
}

/* Math functions */
vitte_value_t builtin_sqrt(vitte_value_t *args, int arg_count) {
    if (arg_count < 1) return vitte_float(0.0);
    
    double val = vitte_is_float(args[0]) ? args[0].as.floating : (double)args[0].as.integer;
    return vitte_float(sqrt(val));
}

vitte_value_t builtin_abs(vitte_value_t *args, int arg_count) {
    if (arg_count < 1) return vitte_nil();
    
    if (vitte_is_int(args[0])) {
        return vitte_int(llabs(args[0].as.integer));
    }
    if (vitte_is_float(args[0])) {
        return vitte_float(fabs(args[0].as.floating));
    }
    
    return vitte_nil();
}

vitte_value_t builtin_floor(vitte_value_t *args, int arg_count) {
    if (arg_count < 1) return vitte_nil();
    
    double val = vitte_is_float(args[0]) ? args[0].as.floating : (double)args[0].as.integer;
    return vitte_float(floor(val));
}

vitte_value_t builtin_ceil(vitte_value_t *args, int arg_count) {
    if (arg_count < 1) return vitte_nil();
    
    double val = vitte_is_float(args[0]) ? args[0].as.floating : (double)args[0].as.integer;
    return vitte_float(ceil(val));
}

vitte_value_t builtin_pow(vitte_value_t *args, int arg_count) {
    if (arg_count < 2) return vitte_nil();
    
    double base = vitte_is_float(args[0]) ? args[0].as.floating : (double)args[0].as.integer;
    double exp = vitte_is_float(args[1]) ? args[1].as.floating : (double)args[1].as.integer;
    
    return vitte_float(pow(base, exp));
}

/* Array functions */
vitte_value_t builtin_array_len(vitte_value_t *args, int arg_count) {
    if (arg_count < 1 || !vitte_is_array(args[0])) {
        return vitte_int(0);
    }
    
    vitte_array_t *arr = (vitte_array_t *)args[0].as.pointer;
    return vitte_int(arr->count);
}

vitte_value_t builtin_array_push(vitte_value_t *args, int arg_count) {
    if (arg_count < 2 || !vitte_is_array(args[0])) {
        return vitte_nil();
    }
    
    vitte_array_t *arr = (vitte_array_t *)args[0].as.pointer;
    
    if (arr->count >= arr->capacity) {
        arr->capacity *= 2;
        arr->values = (vitte_value_t *)realloc(arr->values, 
                                              sizeof(vitte_value_t) * arr->capacity);
    }
    
    arr->values[arr->count++] = args[1];
    return vitte_int(arr->count);
}

vitte_value_t builtin_array_pop(vitte_value_t *args, int arg_count) {
    if (arg_count < 1 || !vitte_is_array(args[0])) {
        return vitte_nil();
    }
    
    vitte_array_t *arr = (vitte_array_t *)args[0].as.pointer;
    
    if (arr->count == 0) {
        return vitte_nil();
    }
    
    return arr->values[--arr->count];
}

/* Type conversion functions */
vitte_value_t builtin_to_int(vitte_value_t *args, int arg_count) {
    if (arg_count < 1) return vitte_int(0);
    
    if (vitte_is_int(args[0])) return args[0];
    if (vitte_is_float(args[0])) return vitte_int((int64_t)args[0].as.floating);
    if (vitte_is_bool(args[0])) return vitte_int(args[0].as.boolean ? 1 : 0);
    if (vitte_is_string(args[0])) return vitte_int(atoll(args[0].as.string.data));
    
    return vitte_int(0);
}

vitte_value_t builtin_to_float(vitte_value_t *args, int arg_count) {
    if (arg_count < 1) return vitte_float(0.0);
    
    if (vitte_is_float(args[0])) return args[0];
    if (vitte_is_int(args[0])) return vitte_float((double)args[0].as.integer);
    if (vitte_is_bool(args[0])) return vitte_float(args[0].as.boolean ? 1.0 : 0.0);
    if (vitte_is_string(args[0])) return vitte_float(atof(args[0].as.string.data));
    
    return vitte_float(0.0);
}

vitte_value_t builtin_to_string(vitte_value_t *args, int arg_count) {
    if (arg_count < 1) return vitte_string("nil");
    
    char buffer[256];
    
    switch (args[0].type) {
        case VITTE_NIL:
            return vitte_string("nil");
        case VITTE_BOOL:
            return vitte_string(args[0].as.boolean ? "true" : "false");
        case VITTE_INT:
            snprintf(buffer, sizeof(buffer), "%" PRId64, args[0].as.integer);
            return vitte_string(buffer);
        case VITTE_FLOAT:
            snprintf(buffer, sizeof(buffer), "%g", args[0].as.floating);
            return vitte_string(buffer);
        case VITTE_STRING:
            return args[0];
        default:
            return vitte_string("[object]");
    }
}

/* Type checking functions */
vitte_value_t builtin_type_of(vitte_value_t *args, int arg_count) {
    if (arg_count < 1) return vitte_string("unknown");
    return vitte_string(vitte_type_name(args[0].type));
}

/* Builtin registry */
builtin_func_t builtin_registry[] = {
    { "strlen", builtin_strlen },
    { "substr", builtin_substr },
    { "strcat", builtin_strcat },
    { "strtoupper", builtin_strtoupper },
    { "sqrt", builtin_sqrt },
    { "abs", builtin_abs },
    { "floor", builtin_floor },
    { "ceil", builtin_ceil },
    { "pow", builtin_pow },
    { "array_len", builtin_array_len },
    { "array_push", builtin_array_push },
    { "array_pop", builtin_array_pop },
    { "to_int", builtin_to_int },
    { "to_float", builtin_to_float },
    { "to_string", builtin_to_string },
    { "type_of", builtin_type_of },
    { NULL, NULL }
};

builtin_func_t* builtin_find(const char *name) {
    for (int i = 0; builtin_registry[i].name != NULL; i++) {
        if (strcmp(builtin_registry[i].name, name) == 0) {
            return &builtin_registry[i];
        }
    }
    return NULL;
}

int builtin_count(void) {
    int count = 0;
    for (int i = 0; builtin_registry[i].name != NULL; i++) {
        count++;
    }
    return count;
}
