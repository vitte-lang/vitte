#include "ir/ir.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t
build_valid_ir_fingerprint(void)
{
    vitte_ir_context_t context;
    vitte_ir_type_desc_t integer_description;
    vitte_ir_type_desc_t function_description;
    vitte_ir_function_desc_t function_info;
    vitte_ir_type_id_t integer_type;
    vitte_ir_type_id_t function_type;
    vitte_ir_string_id_t function_name;
    vitte_ir_string_id_t block_name;
    vitte_ir_function_id_t function;
    vitte_ir_block_id_t entry;
    vitte_ir_value_id_t result;
    vitte_ir_instruction_id_t terminator;
    const vitte_ir_instruction_t *instruction;
    uint64_t fingerprint;

    memset(&context, 0, sizeof(context));
    assert(vitte_ir_init(&context));
    assert(vitte_ir_is_valid(&context));
    assert(vitte_ir_state(&context) == VITTE_IR_STATE_BUILDING);

    integer_description = vitte_ir_type_desc_empty();
    integer_description.kind = VITTE_IR_TYPE_I64;
    integer_type = vitte_ir_intern_type(&context, &integer_description);
    assert(integer_type != VITTE_IR_INVALID_ID);
    assert(vitte_ir_intern_type(&context, &integer_description) == integer_type);

    function_description = vitte_ir_type_desc_empty();
    function_description.kind = VITTE_IR_TYPE_FUNCTION;
    function_description.return_type = integer_type;
    function_type = vitte_ir_intern_type(&context, &function_description);
    assert(function_type != VITTE_IR_INVALID_ID);

    function_name = vitte_ir_intern_string(&context, "answer", 6u);
    block_name = vitte_ir_intern_string(&context, "entry", 5u);
    assert(function_name != VITTE_IR_INVALID_ID);
    assert(block_name != VITTE_IR_INVALID_ID);
    assert(vitte_ir_intern_string(&context, "answer", 6u) == function_name);

    function_info = vitte_ir_function_desc_empty();
    function_info.name = function_name;
    function_info.type = function_type;
    function = vitte_ir_add_function(&context, &function_info);
    assert(function != VITTE_IR_INVALID_ID);

    entry = vitte_ir_add_block(
        &context,
        function,
        block_name,
        vitte_ir_span_invalid());
    assert(entry != VITTE_IR_INVALID_ID);
    result = vitte_ir_const_integer(&context, integer_type, UINT64_C(42));
    assert(result != VITTE_IR_INVALID_ID);

    terminator = vitte_ir_build_return(
        &context,
        entry,
        result,
        vitte_ir_span_invalid());
    assert(terminator != VITTE_IR_INVALID_ID);
    instruction = vitte_ir_get_instruction(&context, terminator);
    assert(instruction != NULL);
    assert(instruction->opcode == VITTE_IR_OP_RETURN);
    assert(instruction->operand_count == 1u);
    assert(instruction->operands[0] == result);
    assert(vitte_ir_validate(&context));

    fingerprint = vitte_ir_fingerprint(&context);
    assert(fingerprint != UINT64_C(0));
    assert(vitte_ir_seal(&context));
    assert(vitte_ir_state(&context) == VITTE_IR_STATE_SEALED);
    vitte_ir_destroy(&context);
    assert(vitte_ir_state(&context) == VITTE_IR_STATE_INVALID);
    assert(!vitte_ir_is_valid(&context));
    return fingerprint;
}

static void
test_verifier_rejects_unterminated_block(void)
{
    vitte_ir_context_t context;
    vitte_ir_type_desc_t integer_description;
    vitte_ir_type_desc_t function_description;
    vitte_ir_function_desc_t function_info;
    vitte_ir_type_id_t integer_type;
    vitte_ir_type_id_t function_type;
    vitte_ir_string_id_t function_name;
    vitte_ir_function_id_t function;

    memset(&context, 0, sizeof(context));
    assert(vitte_ir_init(&context));

    integer_description = vitte_ir_type_desc_empty();
    integer_description.kind = VITTE_IR_TYPE_I64;
    integer_type = vitte_ir_intern_type(&context, &integer_description);
    assert(integer_type != VITTE_IR_INVALID_ID);
    function_description = vitte_ir_type_desc_empty();
    function_description.kind = VITTE_IR_TYPE_FUNCTION;
    function_description.return_type = integer_type;
    function_type = vitte_ir_intern_type(&context, &function_description);
    assert(function_type != VITTE_IR_INVALID_ID);

    function_name = vitte_ir_intern_string(&context, "missing_return", 14u);
    assert(function_name != VITTE_IR_INVALID_ID);
    function_info = vitte_ir_function_desc_empty();
    function_info.name = function_name;
    function_info.type = function_type;
    function = vitte_ir_add_function(&context, &function_info);
    assert(function != VITTE_IR_INVALID_ID);
    assert(vitte_ir_add_block(
        &context,
        function,
        VITTE_IR_INVALID_ID,
        vitte_ir_span_invalid()) != VITTE_IR_INVALID_ID);

    assert(!vitte_ir_validate(&context));
    assert(vitte_ir_last_error(&context) == VITTE_IR_ERROR_MISSING_TERMINATOR);
    assert(context.stats.validation_failures == UINT64_C(1));
    assert(!vitte_ir_seal(&context));
    assert(vitte_ir_state(&context) == VITTE_IR_STATE_FAILED);
    vitte_ir_destroy(&context);
}

int
main(void)
{
    const uint64_t first = build_valid_ir_fingerprint();
    const uint64_t second = build_valid_ir_fingerprint();

    assert(first == second);
    test_verifier_rejects_unterminated_block();
    (void)puts("IR component unit tests passed.");
    return 0;
}
