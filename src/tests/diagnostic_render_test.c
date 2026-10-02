#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "diagnostic/diagnostic.h"
#include "diagnostic/registry.h"
#include "diagnostic/render_terminal.h"

int
main(void)
{
    static const char source_name[] = "sample.vit";
    static const char source_text[] =
        "let value = 123;\n"
        "            value\n";
    vitte_ast_span_t use_span;
    vitte_ast_span_t declaration_span;
    vitte_diagnostic_t *diagnostic;
    vitte_diagnostic_t *contract_diagnostic;
    vitte_diagnostic_t storage[2];
    vitte_diagnostic_t help_storage[2];
    vitte_diagnostic_t limit_storage[1];
    vitte_diagnostic_bag_t bag;
    vitte_diagnostic_bag_t help_bag;
    vitte_diagnostic_bag_t limit_bag;
    vitte_diagnostic_terminal_options_t terminal_options;
    vitte_diagnostic_registry_validation_t registry_validation;
    vitte_source_context_t sources;
    vitte_source_id_t source_id;
    FILE *stream;
    char output[4096];
    size_t bytes;

    assert(vitte_source_init(&sources));
    assert(vitte_diagnostic_registry_validate(
        &registry_validation) == VITTE_STATUS_OK);
    assert(registry_validation.valid);
    assert(vitte_source_add_cstr(
        &sources,
        source_name,
        source_name,
        source_text,
        &source_id));

    memset(&use_span, 0, sizeof(use_span));
    use_span.source_name = source_name;
    use_span.source_id = source_id;
    use_span.start_offset = 29u;
    use_span.end_offset = 34u;
    use_span.start_line = 1u;
    use_span.start_column = 12u;
    use_span.end_line = 1u;
    use_span.end_column = 17u;
    use_span.valid = true;

    declaration_span = use_span;
    declaration_span.start_offset = 4u;
    declaration_span.end_offset = 9u;
    declaration_span.start_line = 0u;
    declaration_span.start_column = 4u;
    declaration_span.end_line = 0u;
    declaration_span.end_column = 9u;

    assert(vitte_diagnostic_bag_init(&bag, storage, 2u) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_emit(
        &bag,
        VITTE_DIAGNOSTIC_ERROR,
        VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK,
        "VITTE_TYPE_E_MISMATCH",
        "type incompatible",
        &use_span) == VITTE_STATUS_OK);
    diagnostic = vitte_diagnostic_at_mut(&bag, 0u);
    assert(diagnostic != NULL);
    vitte_diagnostic_set_subject(diagnostic, "expected", "int", "string");
    vitte_diagnostic_set_context(
        diagnostic,
        NULL,
        NULL,
        "clamp",
        "argument 1 of procedure call");
    assert(vitte_diagnostic_add_location(
        diagnostic,
        VITTE_DIAGNOSTIC_LOCATION_DECLARATION,
        &declaration_span,
        "parameter declared here") == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_cause(
        diagnostic,
        "string cannot satisfy integer parameter",
        &use_span) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_note(
        diagnostic,
        "the parameter is statically typed",
        NULL) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_help(
        diagnostic,
        "provide an integer literal",
        NULL) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_suggestion(
        diagnostic,
        "remove the quotes",
        &use_span,
        "42",
        VITTE_DIAGNOSTIC_APPLICABILITY_MACHINE) == VITTE_STATUS_OK);

    assert(vitte_diagnostic_emit(
        &bag,
        VITTE_DIAGNOSTIC_ERROR,
        VITTE_DIAGNOSTIC_ORIGIN_CONTRACT,
        "VITTE_CONTRACT_E_POSTCONDITION",
        "postcondition failed",
        &use_span) == VITTE_STATUS_OK);
    contract_diagnostic = vitte_diagnostic_at_mut(&bag, 1u);
    assert(contract_diagnostic != NULL);
    assert(vitte_diagnostic_set_contract(
        contract_diagnostic,
        VITTE_DIAGNOSTIC_CONTRACT_ENSURES,
        "result >= minimum",
        "clamp",
        "condition evaluated to false",
        &use_span,
        NULL,
        NULL) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_note(
        contract_diagnostic,
        "`minimum`: 0",
        NULL) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_note(
        contract_diagnostic,
        "`maximum`: 100",
        NULL) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_note(
        contract_diagnostic,
        "`result`: -1",
        NULL) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_cause(
        contract_diagnostic,
        "returned value produced the violation",
        &use_span) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_add_help(
        contract_diagnostic,
        "every successful return from `clamp` must satisfy this condition",
        NULL) == VITTE_STATUS_OK);

    stream = tmpfile();
    assert(stream != NULL);
    vitte_diagnostic_terminal_options_init(&terminal_options);
    terminal_options.color = false;
    terminal_options.sources = &sources;
    assert(vitte_diagnostic_render_terminal(
        stream,
        &bag,
        &terminal_options) == VITTE_STATUS_OK);
    rewind(stream);
    bytes = fread(output, 1u, sizeof(output) - 1u, stream);
    output[bytes] = '\0';
    assert(strstr(output, "sample.vit:2:13") != NULL);
    assert(strstr(output, "error[E0501]: type incompatible") != NULL);
    assert(strstr(output, "error[E0702]: postcondition failed") != NULL);
    assert(strstr(output, "            value") != NULL);
    assert(strstr(output, "^~~~~") != NULL);
    assert(strstr(output, "^~~~~ condition evaluated to false") != NULL);
    assert(strstr(output, "type incompatible") != NULL);
    assert(strstr(output, "procedure: clamp") != NULL);
    assert(strstr(output, "procedure: clamp") < strstr(output, "--> sample.vit"));
    assert(strstr(output, "symbol: expected") != NULL);
    assert(strstr(output, "type: expected `int`, found `string`") != NULL);
    assert(strstr(output, "argument 1 of procedure call") != NULL);
    assert(strstr(output, "parameter declared here") != NULL);
    assert(strstr(output, "caused by: string cannot satisfy integer parameter") != NULL);
    assert(strstr(output, "note: the parameter is statically typed") != NULL);
    assert(strstr(output, "help: provide an integer literal") != NULL);
    assert(strstr(output, "help: remove the quotes [machine-applicable]") != NULL);
    assert(strstr(output, "+ 42") != NULL);
    assert(strstr(output, "postcondition failed") != NULL);
    assert(strstr(output, "phase: contract") != NULL);
    assert(strstr(output, "contract: ensures `result >= minimum`") != NULL);
    assert(strstr(output, "condition evaluated to false") != NULL);
    assert(strstr(output, "`minimum`: 0") != NULL);
    assert(strstr(output, "`maximum`: 100") != NULL);
    assert(strstr(output, "`result`: -1") != NULL);
    assert(strstr(output, "returned value produced the violation") != NULL);
    assert(strstr(output, "every successful return from `clamp` must satisfy this condition") != NULL);
    assert(strstr(output, "`result`: -1") < strstr(output, "returned value produced the violation"));
    assert(strstr(output, "returned value produced the violation") <
        strstr(output, "every successful return from `clamp` must satisfy this condition"));
    assert(fclose(stream) == 0);

    assert(bag.count == 2u);
    assert(bag.counts.error_count == 2u);
    assert(vitte_diagnostic_has_errors(&bag));

    assert(vitte_diagnostic_bag_init(
        &help_bag,
        help_storage,
        2u) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_emit(
        &help_bag,
        VITTE_DIAGNOSTIC_ERROR,
        VITTE_DIAGNOSTIC_ORIGIN_PARSER,
        "VITTE_PARSER_E_EXPECTED_EXPRESSION",
        "expected expression",
        NULL) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_emit(
        &help_bag,
        VITTE_DIAGNOSTIC_WARNING,
        VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK,
        "VITTE_WARNING_TRUNCATION",
        "possible value truncation",
        NULL) == VITTE_STATUS_OK);

    stream = tmpfile();
    assert(stream != NULL);
    assert(vitte_diagnostic_render_terminal(
        stream,
        &help_bag,
        &terminal_options) == VITTE_STATUS_OK);
    rewind(stream);
    bytes = fread(output, 1u, sizeof(output) - 1u, stream);
    output[bytes] = '\0';
    assert(strstr(output, "help: Add a valid expression") != NULL);
    assert(strstr(output, "vitte explain E0203") != NULL);
    assert(strstr(output, "help: Verify the value range and signedness") != NULL);
    assert(fclose(stream) == 0);

    assert(vitte_diagnostic_bag_init(&limit_bag, limit_storage, 1u) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_emit(
        &limit_bag,
        VITTE_DIAGNOSTIC_ERROR,
        VITTE_DIAGNOSTIC_ORIGIN_INTERNAL,
        VITTE_INFRA_E_INTERNAL,
        "first error",
        NULL) == VITTE_STATUS_OK);
    assert(vitte_diagnostic_emit(
        &limit_bag,
        VITTE_DIAGNOSTIC_ERROR,
        VITTE_DIAGNOSTIC_ORIGIN_INTERNAL,
        VITTE_INFRA_E_LIMIT,
        "second error",
        NULL) == VITTE_STATUS_ERROR_INVALID_STATE);
    assert(limit_bag.count == 1u);
    assert(limit_bag.counts.error_count == 1u);
    assert(strcmp(limit_storage[0].short_code, "E0001") == 0);

    vitte_diagnostic_bag_reset(&bag);
    vitte_diagnostic_bag_reset(&help_bag);
    vitte_diagnostic_bag_reset(&limit_bag);
    vitte_source_destroy(&sources);
    return 0;
}
