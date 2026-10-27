#include "type/type.h"

#include <assert.h>
#include <string.h>

int
main(void)
{
    vitte_type_context_t context;
    vitte_type_id_t i64;
    vitte_type_id_t u8;
    vitte_type_id_t u64;
    vitte_type_id_t f32;
    vitte_type_id_t f64;
    vitte_type_id_t character;
    vitte_type_id_t mutable_pointer;
    vitte_type_id_t immutable_pointer;
    vitte_type_id_t mutable_reference;
    vitte_type_id_t immutable_reference;
    vitte_type_id_t array;
    vitte_type_id_t other_array;
    vitte_type_id_t slice;
    vitte_type_id_t generic;

    assert(vitte_type_init(&context));
    i64 = vitte_type_builtin_i64(&context);
    u8 = vitte_type_builtin_u8(&context);
    u64 = vitte_type_builtin_u64(&context);
    f32 = vitte_type_builtin_f32(&context);
    f64 = vitte_type_builtin_f64(&context);
    character = vitte_type_builtin_char(&context);

    assert(vitte_type_pointer(&context, i64, true, &mutable_pointer));
    assert(vitte_type_pointer(&context, i64, false, &immutable_pointer));
    assert(vitte_type_reference(&context, i64, true, &mutable_reference));
    assert(vitte_type_reference(&context, i64, false, &immutable_reference));
    assert(vitte_type_array(&context, i64, 4u, &array));
    assert(vitte_type_array(&context, i64, 5u, &other_array));
    assert(vitte_type_slice(&context, i64, false, &slice));
    assert(vitte_type_generic_parameter(
        &context, UINT64_C(77), "T", 1u, 0u, &generic));

    assert(vitte_type_conversion(&context, i64, i64) ==
        VITTE_TYPE_CONVERSION_EXACT);
    assert(vitte_type_conversion(&context, f64, u8) ==
        VITTE_TYPE_CONVERSION_SAFE);
    assert(vitte_type_conversion(&context, f64, i64) ==
        VITTE_TYPE_CONVERSION_LOSSY);
    assert(!vitte_type_assignable(&context, f64, i64));
    assert(vitte_type_conversion(&context, f64, f32) ==
        VITTE_TYPE_CONVERSION_SAFE);
    assert(vitte_type_conversion(&context, f32, character) ==
        VITTE_TYPE_CONVERSION_SAFE);
    assert(vitte_type_conversion(&context, u64, i64) ==
        VITTE_TYPE_CONVERSION_LOSSY);
    assert(vitte_type_conversion(&context, immutable_pointer, mutable_pointer) ==
        VITTE_TYPE_CONVERSION_SAFE);
    assert(!vitte_type_assignable(&context, mutable_pointer, immutable_pointer));
    assert(vitte_type_conversion(&context, immutable_reference, mutable_reference) ==
        VITTE_TYPE_CONVERSION_SAFE);
    assert(!vitte_type_assignable(&context, mutable_reference, immutable_reference));
    assert(vitte_type_conversion(&context, slice, array) ==
        VITTE_TYPE_CONVERSION_SAFE);
    assert(vitte_type_conversion(&context, array, other_array) ==
        VITTE_TYPE_CONVERSION_INCOMPATIBLE);
    assert(vitte_type_conversion(&context, generic, generic) ==
        VITTE_TYPE_CONVERSION_EXACT);
    assert(strcmp(vitte_type_conversion_kind_name(
        VITTE_TYPE_CONVERSION_LOSSY), "lossy") == 0);

    vitte_type_destroy(&context);
    return 0;
}
