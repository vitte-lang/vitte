#include "../include/vitte_types.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* Value creation */
vitte_value_t vitte_nil(void) {
    return (vitte_value_t){ .type = VITTE_NIL };
}

vitte_value_t vitte_bool(bool value) {
    return (vitte_value_t){ 
        .type = VITTE_BOOL,
        .as.boolean = value
    };
}

vitte_value_t vitte_int(int64_t value) {
    return (vitte_value_t){
        .type = VITTE_INT,
        .as.integer = value
    };
}

vitte_value_t vitte_float(double value) {
    return (vitte_value_t){
        .type = VITTE_FLOAT,
        .as.floating = value
    };
}

vitte_value_t vitte_string(const char *value) {
    vitte_value_t v;
    v.type = VITTE_STRING;
    v.as.string.data = (char *)malloc(strlen(value) + 1);
    strcpy(v.as.string.data, value);
    v.as.string.length = strlen(value);
    return v;
}

vitte_value_t vitte_array(void) {
    vitte_array_t *arr = (vitte_array_t *)malloc(sizeof(vitte_array_t));
    if (!arr) return vitte_nil();
    
    arr->values = (vitte_value_t *)malloc(sizeof(vitte_value_t) * 16);
    arr->capacity = 16;
    arr->count = 0;
    
    return (vitte_value_t){
        .type = VITTE_ARRAY,
        .as.pointer = arr
    };
}

/* Type checking */
bool vitte_is_nil(vitte_value_t v) {
    return v.type == VITTE_NIL;
}

bool vitte_is_bool(vitte_value_t v) {
    return v.type == VITTE_BOOL;
}

bool vitte_is_int(vitte_value_t v) {
    return v.type == VITTE_INT;
}

bool vitte_is_float(vitte_value_t v) {
    return v.type == VITTE_FLOAT;
}

bool vitte_is_string(vitte_value_t v) {
    return v.type == VITTE_STRING;
}

bool vitte_is_array(vitte_value_t v) {
    return v.type == VITTE_ARRAY;
}

bool vitte_is_truthy(vitte_value_t v) {
    switch (v.type) {
        case VITTE_NIL: return false;
        case VITTE_BOOL: return v.as.boolean;
        case VITTE_INT: return v.as.integer != 0;
        case VITTE_FLOAT: return v.as.floating != 0.0;
        case VITTE_STRING: return v.as.string.length > 0;
        default: return true;
    }
}

/* Arithmetic operations */
vitte_value_t vitte_add(vitte_value_t left, vitte_value_t right) {
    if (vitte_is_int(left) && vitte_is_int(right)) {
        return vitte_int(left.as.integer + right.as.integer);
    }
    
    double l = vitte_is_float(left) ? left.as.floating : (double)left.as.integer;
    double r = vitte_is_float(right) ? right.as.floating : (double)right.as.integer;
    
    return vitte_float(l + r);
}

vitte_value_t vitte_subtract(vitte_value_t left, vitte_value_t right) {
    if (vitte_is_int(left) && vitte_is_int(right)) {
        return vitte_int(left.as.integer - right.as.integer);
    }
    
    double l = vitte_is_float(left) ? left.as.floating : (double)left.as.integer;
    double r = vitte_is_float(right) ? right.as.floating : (double)right.as.integer;
    
    return vitte_float(l - r);
}

vitte_value_t vitte_multiply(vitte_value_t left, vitte_value_t right) {
    if (vitte_is_int(left) && vitte_is_int(right)) {
        return vitte_int(left.as.integer * right.as.integer);
    }
    
    double l = vitte_is_float(left) ? left.as.floating : (double)left.as.integer;
    double r = vitte_is_float(right) ? right.as.floating : (double)right.as.integer;
    
    return vitte_float(l * r);
}

vitte_value_t vitte_divide(vitte_value_t left, vitte_value_t right) {
    if (vitte_is_int(left) && vitte_is_int(right) && right.as.integer != 0) {
        return vitte_int(left.as.integer / right.as.integer);
    }
    
    double l = vitte_is_float(left) ? left.as.floating : (double)left.as.integer;
    double r = vitte_is_float(right) ? right.as.floating : (double)right.as.integer;
    
    if (r == 0.0) return vitte_float(INFINITY);
    return vitte_float(l / r);
}

vitte_value_t vitte_negate(vitte_value_t value) {
    if (vitte_is_int(value)) {
        return vitte_int(-value.as.integer);
    }
    if (vitte_is_float(value)) {
        return vitte_float(-value.as.floating);
    }
    return vitte_nil();
}

/* Comparison operations */
bool vitte_equal(vitte_value_t left, vitte_value_t right) {
    if (left.type != right.type) return false;
    
    switch (left.type) {
        case VITTE_NIL: return true;
        case VITTE_BOOL: return left.as.boolean == right.as.boolean;
        case VITTE_INT: return left.as.integer == right.as.integer;
        case VITTE_FLOAT: return fabs(left.as.floating - right.as.floating) < 1e-9;
        case VITTE_STRING:
            return strcmp(left.as.string.data, right.as.string.data) == 0;
        default: return false;
    }
}

bool vitte_less(vitte_value_t left, vitte_value_t right) {
    if (vitte_is_int(left) && vitte_is_int(right)) {
        return left.as.integer < right.as.integer;
    }
    
    double l = vitte_is_float(left) ? left.as.floating : (double)left.as.integer;
    double r = vitte_is_float(right) ? right.as.floating : (double)right.as.integer;
    
    return l < r;
}

/* Type name */
const char* vitte_type_name(vitte_type_t type) {
    switch (type) {
        case VITTE_NIL: return "nil";
        case VITTE_BOOL: return "bool";
        case VITTE_INT: return "int";
        case VITTE_FLOAT: return "float";
        case VITTE_STRING: return "string";
        case VITTE_ARRAY: return "array";
        case VITTE_OBJECT: return "object";
        case VITTE_FUNCTION: return "function";
        case VITTE_NATIVE_FUNCTION: return "native_function";
        case VITTE_CLOSURE: return "closure";
        case VITTE_CLASS: return "class";
        case VITTE_INSTANCE: return "instance";
        default: return "unknown";
    }
}

/* Value to string */
void vitte_print(vitte_value_t value) {
    switch (value.type) {
        case VITTE_NIL:
            printf("nil");
            break;
        case VITTE_BOOL:
            printf("%s", value.as.boolean ? "true" : "false");
            break;
        case VITTE_INT:
            printf("%" PRId64, value.as.integer);
            break;
        case VITTE_FLOAT:
            printf("%.6g", value.as.floating);
            break;
        case VITTE_STRING:
            printf("%s", value.as.string.data);
            break;
        case VITTE_ARRAY:
            printf("[array]");
            break;
        default:
            printf("[%s]", vitte_type_name(value.type));
    }
}
