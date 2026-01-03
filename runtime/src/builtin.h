#ifndef VITTE_BUILTIN_H
#define VITTE_BUILTIN_H

#include <inttypes.h>

typedef struct vitte_value vitte_value_t;

/* Builtin function pointer */
typedef vitte_value_t (*builtin_fn_t)(vitte_value_t *args, int arg_count);

/* Builtin function registry entry */
typedef struct {
    const char *name;
    builtin_fn_t func;
} builtin_func_t;

/* String functions */
vitte_value_t builtin_strlen(vitte_value_t *args, int arg_count);
vitte_value_t builtin_substr(vitte_value_t *args, int arg_count);
vitte_value_t builtin_strcat(vitte_value_t *args, int arg_count);
vitte_value_t builtin_strtoupper(vitte_value_t *args, int arg_count);

/* Math functions */
vitte_value_t builtin_sqrt(vitte_value_t *args, int arg_count);
vitte_value_t builtin_abs(vitte_value_t *args, int arg_count);
vitte_value_t builtin_floor(vitte_value_t *args, int arg_count);
vitte_value_t builtin_ceil(vitte_value_t *args, int arg_count);
vitte_value_t builtin_pow(vitte_value_t *args, int arg_count);

/* Array functions */
vitte_value_t builtin_array_len(vitte_value_t *args, int arg_count);
vitte_value_t builtin_array_push(vitte_value_t *args, int arg_count);
vitte_value_t builtin_array_pop(vitte_value_t *args, int arg_count);

/* Type conversion functions */
vitte_value_t builtin_to_int(vitte_value_t *args, int arg_count);
vitte_value_t builtin_to_float(vitte_value_t *args, int arg_count);
vitte_value_t builtin_to_string(vitte_value_t *args, int arg_count);
vitte_value_t builtin_type_of(vitte_value_t *args, int arg_count);

/* Registry functions */
builtin_func_t* builtin_find(const char *name);
int builtin_count(void);

#endif
