#ifndef VITTE_TYPES_H
#define VITTE_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Vitte Type System */

typedef enum {
    VITTE_NIL,
    VITTE_BOOL,
    VITTE_INT,
    VITTE_FLOAT,
    VITTE_STRING,
    VITTE_ARRAY,
    VITTE_OBJECT,
    VITTE_FUNCTION,
    VITTE_NATIVE_FUNCTION,
    VITTE_CLOSURE,
    VITTE_CLASS,
    VITTE_INSTANCE,
} vitte_type_t;

typedef struct vitte_value {
    vitte_type_t type;
    union {
        bool boolean;
        int64_t integer;
        double floating;
        void *pointer;
        struct {
            char *data;
            size_t length;
        } string;
    } as;
} vitte_value_t;

typedef struct vitte_array {
    vitte_value_t *values;
    size_t count;
    size_t capacity;
} vitte_array_t;

typedef struct vitte_string {
    char *chars;
    size_t length;
    uint32_t hash;
} vitte_string_t;

typedef struct vitte_object {
    void *data;
    void (*destructor)(void *);
} vitte_object_t;

typedef struct vitte_function {
    const char *name;
    void *bytecode;
    size_t bytecode_size;
    int arg_count;
    int upvalue_count;
} vitte_function_t;

typedef vitte_value_t (*vitte_native_fn)(int argc, vitte_value_t *argv);

typedef struct vitte_closure {
    vitte_function_t *function;
    vitte_value_t *upvalues;
    int upvalue_count;
} vitte_closure_t;

typedef struct vitte_class {
    vitte_string_t *name;
    vitte_value_t *methods;
    size_t method_count;
} vitte_class_t;

typedef struct vitte_instance {
    vitte_class_t *klass;
    void *fields;
} vitte_instance_t;

/* Value creation helpers */
vitte_value_t vitte_nil(void);
vitte_value_t vitte_bool(bool value);
vitte_value_t vitte_int(int64_t value);
vitte_value_t vitte_float(double value);
vitte_value_t vitte_string(const char *value);
vitte_value_t vitte_array(void);

/* Type checking */
bool vitte_is_nil(vitte_value_t v);
bool vitte_is_bool(vitte_value_t v);
bool vitte_is_int(vitte_value_t v);
bool vitte_is_float(vitte_value_t v);
bool vitte_is_string(vitte_value_t v);
bool vitte_is_array(vitte_value_t v);
bool vitte_is_truthy(vitte_value_t v);

/* Value operations */
vitte_value_t vitte_add(vitte_value_t left, vitte_value_t right);
vitte_value_t vitte_subtract(vitte_value_t left, vitte_value_t right);
vitte_value_t vitte_multiply(vitte_value_t left, vitte_value_t right);
vitte_value_t vitte_divide(vitte_value_t left, vitte_value_t right);
vitte_value_t vitte_negate(vitte_value_t value);
bool vitte_equal(vitte_value_t left, vitte_value_t right);
bool vitte_less(vitte_value_t left, vitte_value_t right);

#endif
