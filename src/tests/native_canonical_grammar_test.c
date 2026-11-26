#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef VITTE_CANONICAL_PARSER_QUALIFIED
#define vitte_parser_init compiler_parser_parser_vitte_parser_init
#define vitte_parser_run_native compiler_parser_parser_vitte_parser_run_native
#define vitte_parser_validate compiler_parser_parser_vitte_parser_validate
#define vitte_parser_set_limits compiler_parser_parser_vitte_parser_set_limits
#define vitte_parser_node_count compiler_parser_parser_vitte_parser_node_count
#define vitte_parser_position compiler_parser_parser_vitte_parser_position
#define vitte_parser_diagnostic_count compiler_parser_parser_vitte_parser_diagnostic_count
#define vitte_parser_last_error compiler_parser_parser_vitte_parser_last_error
#define vitte_parser_error_name compiler_parser_parser_vitte_parser_error_name
#define vitte_parser_diagnostic_at compiler_parser_parser_vitte_parser_diagnostic_at
#define vitte_parser_root compiler_parser_parser_vitte_parser_root
#define vitte_parser_destroy compiler_parser_parser_vitte_parser_destroy
#endif

typedef struct native_lexer_span {
    uint64_t file_id;
    size_t begin;
    size_t end;
    bool valid;
} native_lexer_span_t;

typedef struct native_lexer_token {
    int32_t kind;
    native_lexer_span_t span;
    size_t line;
    size_t column;
    const char *lexeme;
    size_t length;
} native_lexer_token_t;

typedef struct native_parser_token_span {
    uint32_t file_id;
    size_t begin;
    size_t end;
    bool valid;
} native_parser_token_span_t;

typedef struct native_parser_token {
    int32_t kind;
    native_parser_token_span_t span;
    size_t line;
    size_t column;
    const char *lexeme;
    size_t length;
} native_parser_token_t;

typedef struct native_parser_span {
    uint32_t file_id;
    size_t begin;
    size_t end;
    size_t line;
    size_t column;
    bool valid;
} native_parser_span_t;

typedef struct native_parser_diagnostic {
    int32_t kind;
    int32_t error;
    native_parser_span_t span;
    int32_t expected;
    int32_t found;
} native_parser_diagnostic_t;

typedef union native_storage {
    max_align_t alignment;
    unsigned char bytes[1024];
} native_storage_t;

extern bool compiler_lexer_lexer_vitte_lexer_init(
    void *lexer,
    const char *source,
    size_t source_length,
    uint64_t file_id);
extern bool compiler_lexer_lexer_vitte_lexer_run(void *lexer);
extern void compiler_lexer_lexer_vitte_lexer_destroy(void *lexer);
extern size_t compiler_lexer_lexer_vitte_lexer_token_count(void *lexer);
extern native_lexer_token_t *compiler_lexer_lexer_vitte_lexer_token_at(
    void *lexer,
    size_t index);
extern const char *compiler_lexer_lexer_vitte_lexer_error_name(int32_t error);
extern int32_t compiler_lexer_lexer_vitte_lexer_last_error(void *lexer);

extern bool vitte_parser_init(
    void *parser,
    const native_parser_token_t *tokens,
    size_t token_count);
extern bool vitte_parser_run_native(void *parser);
extern bool vitte_parser_validate(void *parser);
extern bool vitte_parser_set_limits(
    void *parser,
    size_t max_nodes,
    size_t max_diagnostics,
    size_t max_recursion_depth,
    size_t max_recovery_tokens);
extern size_t vitte_parser_node_count(void *parser);
extern size_t vitte_parser_position(void *parser);
extern size_t vitte_parser_diagnostic_count(void *parser);
extern int32_t vitte_parser_last_error(void *parser);
extern const char *vitte_parser_error_name(int32_t error);
extern native_parser_diagnostic_t *vitte_parser_diagnostic_at(
    void *parser,
    size_t index);
extern const char *compiler_lexer_lexer_vitte_token_kind_name(int32_t kind);
extern uint64_t vitte_parser_root(void *parser);
extern void vitte_parser_destroy(void *parser);

void *
native_allocate(size_t size)
{
    return malloc(size);
}

void
native_deallocate(void *pointer)
{
    free(pointer);
}

static char *
read_source_file(const char *path, size_t *length_out)
{
    FILE *file;
    long file_length;
    char *source;
    size_t length;

    file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0) {
        if (file != NULL) {
            (void)fclose(file);
        }
        return NULL;
    }
    file_length = ftell(file);
    if (file_length < 0L || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    length = (size_t)file_length;
    source = (char *)malloc(length + 1u);
    if (source == NULL ||
        (length != 0u && fread(source, 1u, length, file) != length)) {
        free(source);
        (void)fclose(file);
        return NULL;
    }
    source[length] = '\0';
    if (fclose(file) != 0) {
        free(source);
        return NULL;
    }
    *length_out = length;
    return source;
}

static int
expect_native_lexer_error(
    const char *source,
    size_t source_length,
    const char *expected_error)
{
    native_storage_t lexer_storage = { 0 };
    const char *error_name;

    if (!compiler_lexer_lexer_vitte_lexer_init(
            lexer_storage.bytes,
            source,
            source_length,
            UINT64_C(2))) {
        return 0;
    }
    if (compiler_lexer_lexer_vitte_lexer_run(lexer_storage.bytes)) {
        compiler_lexer_lexer_vitte_lexer_destroy(lexer_storage.bytes);
        return 0;
    }
    error_name = compiler_lexer_lexer_vitte_lexer_error_name(
        compiler_lexer_lexer_vitte_lexer_last_error(lexer_storage.bytes));
    if (error_name == NULL || strcmp(error_name, expected_error) != 0) {
        (void)fprintf(
            stderr,
            "native lexer error mismatch (expected %s, got %s)\n",
            expected_error,
            error_name != NULL ? error_name : "unknown");
        compiler_lexer_lexer_vitte_lexer_destroy(lexer_storage.bytes);
        return 0;
    }
    compiler_lexer_lexer_vitte_lexer_destroy(lexer_storage.bytes);
    return 1;
}

static int
expect_native_token_lexemes(void)
{
    static const char source[] = "@ # $ :: >>= ??";
    static const char *const expected[] = {
        "@", "#", "$", "::", ">>=", "??", ""
    };
    native_storage_t lexer_storage = { 0 };
    size_t token_count;
    size_t index;
    int result = 0;

    if (!compiler_lexer_lexer_vitte_lexer_init(
            lexer_storage.bytes,
            source,
            sizeof(source) - 1u,
            UINT64_C(4)) ||
        !compiler_lexer_lexer_vitte_lexer_run(lexer_storage.bytes)) {
        goto cleanup;
    }
    token_count = compiler_lexer_lexer_vitte_lexer_token_count(
        lexer_storage.bytes);
    if (token_count != sizeof(expected) / sizeof(expected[0])) {
        goto cleanup;
    }
    for (index = 0u; index < token_count; ++index) {
        native_lexer_token_t *token =
            compiler_lexer_lexer_vitte_lexer_token_at(
                lexer_storage.bytes,
                index);
        size_t expected_length = strlen(expected[index]);

        if (token == NULL || token->length != expected_length ||
            (expected_length != 0u &&
             memcmp(token->lexeme, expected[index], expected_length) != 0) ||
            token->span.end - token->span.begin != token->length) {
            goto cleanup;
        }
    }
    result = 1;

cleanup:
    compiler_lexer_lexer_vitte_lexer_destroy(lexer_storage.bytes);
    return result;
}

static bool
native_keyword_is_reserved(const char *text, size_t length)
{
    static const char *const reserved[] = {
        "share", "region", "compiler", "diagnostic", "bench", "entry",
        "select", "when", "case", "critical", "owned", "borrow",
        "volatile", "is", "kernel", "class", "flags"
    };
    size_t index;

    for (index = 0u; index < sizeof(reserved) / sizeof(reserved[0]); ++index) {
        if (strlen(reserved[index]) == length &&
            memcmp(text, reserved[index], length) == 0) {
            return true;
        }
    }
    return false;
}

static int
expect_native_keyword_mappings(void)
{
    static const char source[] =
        "space use pub const static type opaque form pick trait impl where "
        "proc extern intrinsic macro comptime test let mut set return defer "
        "requires ensures export if else elif while loop for in break continue "
        "match as async await unsafe asm move ref self and or not true false "
        "null assert sizeof alignof print share region compiler diagnostic "
        "bench entry select when case critical owned borrow volatile is kernel "
        "class flags";
    native_storage_t lexer_storage = { 0 };
    size_t source_offset = 0u;
    size_t token_index = 0u;
    size_t token_count;
    int result = 0;

    if (!compiler_lexer_lexer_vitte_lexer_init(
            lexer_storage.bytes,
            source,
            sizeof(source) - 1u,
            UINT64_C(5)) ||
        !compiler_lexer_lexer_vitte_lexer_run(lexer_storage.bytes)) {
        goto cleanup;
    }
    token_count = compiler_lexer_lexer_vitte_lexer_token_count(
        lexer_storage.bytes);
    while (source_offset < sizeof(source) - 1u) {
        size_t word_begin;
        size_t word_length;
        char word[32];
        const char *expected_kind;
        native_lexer_token_t *token;
        const char *kind_name;

        while (source_offset < sizeof(source) - 1u &&
               source[source_offset] == ' ') {
            ++source_offset;
        }
        if (source_offset >= sizeof(source) - 1u) {
            break;
        }
        word_begin = source_offset;
        while (source_offset < sizeof(source) - 1u &&
               source[source_offset] != ' ') {
            ++source_offset;
        }
        word_length = source_offset - word_begin;
        if (word_length >= sizeof(word)) {
            goto cleanup;
        }
        memcpy(word, source + word_begin, word_length);
        word[word_length] = '\0';
        token = compiler_lexer_lexer_vitte_lexer_token_at(
            lexer_storage.bytes,
            token_index++);
        kind_name = token != NULL
            ? compiler_lexer_lexer_vitte_token_kind_name(token->kind)
            : NULL;
        expected_kind = native_keyword_is_reserved(word, word_length)
            ? "reserved_keyword"
            : (strcmp(word, "intrinsic") == 0 ? "extern" : word);
        if (token == NULL || kind_name == NULL ||
            token->length != word_length ||
            memcmp(token->lexeme, source + word_begin, word_length) != 0 ||
            strcmp(kind_name, expected_kind) != 0) {
            (void)fprintf(
                stderr,
                "native keyword token mismatch near '%s' (expected %s, got %s)\n",
                word,
                expected_kind,
                kind_name != NULL ? kind_name : "unknown");
            goto cleanup;
        }
    }
    if (token_count != token_index + 1u) {
        goto cleanup;
    }
    {
        native_lexer_token_t *eof =
            compiler_lexer_lexer_vitte_lexer_token_at(
                lexer_storage.bytes,
                token_index);
        const char *kind_name = eof != NULL
            ? compiler_lexer_lexer_vitte_token_kind_name(eof->kind)
            : NULL;
        result = kind_name != NULL && strcmp(kind_name, "eof") == 0;
    }

cleanup:
    compiler_lexer_lexer_vitte_lexer_destroy(lexer_storage.bytes);
    return result;
}

static int
expect_native_parser_result(
    const char *source,
    size_t source_length,
    bool should_parse,
    const char *expected_parser_error,
    bool limit_nodes,
    bool validate_partial_ast)
{
    native_storage_t lexer_storage = { 0 };
    native_storage_t parser_storage = { 0 };
    native_lexer_token_t *lexer_token;
    native_parser_token_t *parser_tokens = NULL;
    size_t token_count;
    size_t index;
    bool parsed;
    int result = 0;

    if (!compiler_lexer_lexer_vitte_lexer_init(
            lexer_storage.bytes,
            source,
            source_length,
            UINT64_C(3)) ||
        !compiler_lexer_lexer_vitte_lexer_run(lexer_storage.bytes)) {
        goto cleanup_lexer;
    }
    token_count = compiler_lexer_lexer_vitte_lexer_token_count(
        lexer_storage.bytes);
    parser_tokens = (native_parser_token_t *)calloc(
        token_count,
        sizeof(*parser_tokens));
    if (parser_tokens == NULL && token_count != 0u) {
        goto cleanup_lexer;
    }
    for (index = 0u; index < token_count; ++index) {
        lexer_token = compiler_lexer_lexer_vitte_lexer_token_at(
            lexer_storage.bytes,
            index);
        if (lexer_token == NULL) {
            goto cleanup_tokens;
        }
        parser_tokens[index].kind = lexer_token->kind;
        parser_tokens[index].span.file_id = (uint32_t)lexer_token->span.file_id;
        parser_tokens[index].span.begin = lexer_token->span.begin;
        parser_tokens[index].span.end = lexer_token->span.end;
        parser_tokens[index].span.valid = lexer_token->span.valid;
        parser_tokens[index].line = lexer_token->line;
        parser_tokens[index].column = lexer_token->column;
        parser_tokens[index].lexeme = lexer_token->lexeme;
        parser_tokens[index].length = lexer_token->length;
    }
    if (!vitte_parser_init(
            parser_storage.bytes,
            parser_tokens,
            token_count)) {
        goto cleanup_tokens;
    }
    if (limit_nodes &&
        !vitte_parser_set_limits(
            parser_storage.bytes,
            1u,
            8u,
            32u,
            32u)) {
        vitte_parser_destroy(parser_storage.bytes);
        goto cleanup_tokens;
    }

    parsed = vitte_parser_run_native(parser_storage.bytes);
    if (should_parse) {
        result = parsed &&
            vitte_parser_diagnostic_count(parser_storage.bytes) == 0u &&
            vitte_parser_validate(parser_storage.bytes);
    } else if (limit_nodes) {
        const char *error_name = vitte_parser_error_name(
            vitte_parser_last_error(parser_storage.bytes));
        result = !parsed && error_name != NULL &&
            strcmp(error_name, "node_limit") == 0;
    } else {
        result = !parsed &&
            vitte_parser_diagnostic_count(parser_storage.bytes) != 0u;
        if (result && expected_parser_error != NULL) {
            native_parser_diagnostic_t *diagnostic =
                vitte_parser_diagnostic_at(parser_storage.bytes, 0u);
            const char *error_name = diagnostic != NULL
                ? vitte_parser_error_name(diagnostic->error)
                : NULL;

            result = error_name != NULL &&
                strcmp(error_name, expected_parser_error) == 0;
        }
        if (result && validate_partial_ast) {
            result = vitte_parser_validate(parser_storage.bytes);
        }
    }
    if (!result) {
        (void)fprintf(
            stderr,
            "native parser regression failed (accepted=%d, node_limit=%d, "
            "diagnostics=%zu, expected=%s, error=%s, source=%.*s)\n",
            should_parse,
            limit_nodes,
            vitte_parser_diagnostic_count(parser_storage.bytes),
            expected_parser_error != NULL ? expected_parser_error : "none",
            vitte_parser_error_name(
                vitte_parser_last_error(parser_storage.bytes)),
            (int)(source_length < 80u ? source_length : 80u), source);
    }
    vitte_parser_destroy(parser_storage.bytes);

cleanup_tokens:
    free(parser_tokens);
cleanup_lexer:
    compiler_lexer_lexer_vitte_lexer_destroy(lexer_storage.bytes);
    return result;
}

static int
run_native_regression_cases(void)
{
    static const char valid_source[] =
        "space tests::native_valid; proc valid() { let items = [1, 2, 3]; return; }";
    static const char malformed_source[] =
        "proc malformed() { let value = ; }";
    static const char malformed_defer[] = "proc bad() { defer; }";
    static const char malformed_assert[] = "proc bad() { assert true; }";
    static const char malformed_unsafe[] = "proc bad() { unsafe value; }";
    static const char malformed_assembly[] = "proc bad() { asm(,); }";
    static const char malformed_if[] = "proc bad() { if; }";
    static const char dangling_else[] = "proc bad() { else {} }";
    static const char malformed_while[] = "proc bad() { while true; }";
    static const char malformed_loop[] = "proc bad() { loop; }";
    static const char malformed_for[] = "proc bad() { for item values {} }";
    static const char malformed_match[] = "proc bad() { match; }";
    static const char malformed_match_arm[] =
        "proc bad() { match true { true false => 0, } }";
    static const char malformed_tuple_pattern[] =
        "proc bad() { match true { (true,, false) => 1, } }";
    static const char malformed_struct_pattern_with_rest[] =
        "proc bad() { match true { Point { .., x: } => 1, } }";
    static const char invalid_range_pattern[] =
        "proc bad() { match value { item..4 => 0, } }";
    static const char malformed_assignment_expression[] =
        "proc bad() { let value = target = ; }";
    static const char malformed_coalescing_expression[] =
        "proc bad() { let value = target ?? ; }";
    static const char malformed_logical_expression[] =
        "proc bad() { let value = target and ; }";
    static const char malformed_bitwise_expression[] =
        "proc bad() { let value = target | ; }";
    static const char malformed_comparison_expression[] =
        "proc bad() { let value = target == ; }";
    static const char malformed_shift_expression[] =
        "proc bad() { let value = target << ; }";
    static const char malformed_arithmetic_expression[] =
        "proc bad() { let value = target + ; }";
    static const char malformed_range_expression[] =
        "proc bad() { let value = target .. ; }";
    static const char malformed_unary_expression[] =
        "proc bad() { let value = -; }";
    static const char malformed_cast_expression[] =
        "proc bad() { let value = target as ; }";
    static const char cast_missing_value[] =
        "proc bad() { let value = as int; }";
    static const char malformed_precedence_operator[] =
        "proc bad() { let value = left + right * / divisor; }";
    static const char malformed_call_suffix[] =
        "proc bad() { let value = invoke(1; }";
    static const char malformed_index_suffix[] =
        "proc bad() { let value = items[]; }";
    static const char malformed_field_suffix[] =
        "proc bad() { let value = item.; }";
    static const char malformed_optional_postfix[] =
        "proc bad() { let value = ?item; }";
    static const char malformed_form_initializer[] =
        "proc bad() { let value = Point { x: }; }";
    static const char missing_if_expression_else[] =
        "proc bad() { let value = if ready { 1 }; }";
    static const char if_condition_preserves_operator_error[] =
        "proc bad() { let value = if ready + ; { 1 } else { 0 }; }";
    static const char malformed_block_expression[] =
        "proc bad() { let value = { 1 2 }; }";
    static const char empty_generic_arguments[] = "type Bad = Map<>;";
    static const char recovery_source[] =
        "proc malformed() { let value = ; return; } proc tail() {}";
    static const char missing_module_path[] = "space ;";
    static const char duplicate_module_header[] =
        "space first; space second;";
    static const char misplaced_module_header[] =
        "proc before() {} space late;";
    static const char malformed_use[] = "use ;";
    static const char invalid_use_alias[] = "use core as ;";
    static const char malformed_import_group[] =
        "use core::{first second};";
    static const char invalid_nested_glob[] =
        "use core::{outer::{*}};";
    static const char malformed_export[] = "export ;";
    static const char empty_export_group[] = "export {};";
    static const char invalid_export_alias[] = "export item as ;";
    static const char trailing_use_tokens[] = "use core extra;";
    static const char trailing_export_tokens[] = "export item extra;";
    static const char missing_module_header[] = "use core;";
    static const char missing_export_header[] = "export item;";
    static const char missing_value_name[] = "const = 1;";
    static const char missing_value_equal[] = "const LIMIT 10;";
    static const char missing_value_expression[] = "const LIMIT = ;";
    static const char missing_value_semicolon[] = "static mut COUNT = 1";
    static const char missing_type_alias[] = "type Alias = ;";
    static const char invalid_type_bound[] = "type Item: Trait + = int;";
    static const char invalid_procedure_type[] =
        "type Bad = proc(int bool);";
    static const char invalid_reference_type[] = "type Bad = &;";
    static const char invalid_pointer_type[] = "type Bad = *;";
    static const char invalid_array_type[] = "type Bad = [int; ];";
    static const char invalid_slice_type[] = "type Bad = [];";
    static const char invalid_optional_type[] = "type Bad = ?;";
    static const char invalid_type_path[] = "type Bad = Item::;";
    static const char invalid_type_separator[] =
        "type Bad = Item: Other;";
    static const char invalid_angle_type_arguments[] =
        "type Bad = Map<int bool>;";
    static const char invalid_bracket_type_arguments[] =
        "type Bad = Map[int bool];";
    static const char invalid_let_pattern[] = "proc bad() { let = 1; }";
    static const char invalid_let_type[] = "proc bad() { let value: ; }";
    static const char invalid_let_initializer[] =
        "proc bad() { let value = ; }";
    static const char missing_set_operator[] = "proc bad() { set value; }";
    static const char invalid_set_operator[] =
        "proc bad() { set value == 1; }";
    static const char invalid_set_value[] = "proc bad() { set value = ; }";
    static const char malformed_return[] = "proc bad() { return 1 2; }";
    static const char malformed_jump[] = "proc bad() { break value; }";
    static const char missing_let_terminator[] =
        "proc bad() { let value = 1 }";
    static const char missing_set_terminator[] =
        "proc bad() { set value = 1 }";
    static const char missing_jump_terminator[] = "proc bad() { break }";
    static const char return_outside_procedure[] = "return 1;";
    static const char jump_outside_procedure[] = "break;";
    static const char return_without_final_semicolon[] =
        "proc ok() { return }";
    static const char missing_opaque_alias[] = "opaque type Handle = ;";
    static const char invalid_opaque_bound[] = "opaque type Handle: Trait;";
    static const char missing_form_name[] = "form { value: int }";
    static const char malformed_form_field[] = "form Point { value int }";
    static const char duplicate_form_field[] =
        "form Point { value: int, value: bool }";
    static const char missing_form_separator[] =
        "form Point { x: int y: int }";
    static const char missing_form_closer[] = "form Point { x: int";
    static const char malformed_pick[] = "pick { A }";
    static const char duplicate_pick_variant[] = "pick Choice { Some, Some }";
    static const char invalid_pick_payload[] = "pick Choice { Some(,) }";
    static const char malformed_trait[] = "trait Display";
    static const char malformed_impl[] = "impl Point";
    static const char missing_impl_target[] = "impl Display for {}";
    static const char invalid_impl_for_clause[] =
        "impl Display for Point extra {}";
    static const char malformed_procedure[] = "proc () {}";
    static const char malformed_procedure_parameters[] = "proc f(x int) {}";
    static const char missing_procedure_parameter_closer[] = "proc f(x: int {}";
    static const char invalid_procedure_return[] = "proc f() -> {}";
    static const char duplicate_procedure_modifier[] =
        "async async proc f() {}";
    static const char misplaced_procedure_modifier[] = "async form Point {}";
    static const char invalid_procedure_body[] = "proc f() return 1;";
    static const char invalid_external_procedure_body[] =
        "extern proc f() {}";
    static const char missing_external_procedure_semicolon[] =
        "extern proc f()";
    static const char valid_declarations[] =
        "const LIMIT: int = 10; static mut COUNT: int = 0; "
        "type Alias = int; type Bound: Trait + Debug; "
        "opaque type Handle; opaque type Raw = int; "
        "form Point { x: int, y: int; } "
        "pick Option<T> { None, Some(T), Empty() } "
        "trait Marker { proc marker(); } "
        "impl Marker for Point { proc marker() {} } "
        "async proc run(value: int) -> int { return value; } "
        "unsafe comptime proc restricted() {} "
        "extern proc c_call(value: int);";
    static const char node_limit_source[] =
        "space tests::node_limit; proc limited() {}";
    static const char invalid_utf8[] = { (char)0xC0, (char)0xAF, '\0' };
    static const char invalid_number[] = "0b102";
    static const char invalid_escape[] = "\"bad\\q\"";

    return expect_native_lexer_error(
               invalid_utf8, sizeof(invalid_utf8) - 1u, "invalid_utf8") &&
        expect_native_lexer_error(
               invalid_number, sizeof(invalid_number) - 1u,
               "invalid_numeric_base") &&
        expect_native_lexer_error(
               invalid_escape, sizeof(invalid_escape) - 1u,
               "invalid_escape") &&
        expect_native_token_lexemes() &&
        expect_native_keyword_mappings() &&
        expect_native_parser_result(
               valid_source, sizeof(valid_source) - 1u,
               true, NULL, false, false) &&
        expect_native_parser_result(
               malformed_source, sizeof(malformed_source) - 1u,
               false, NULL, false, true) &&
        expect_native_parser_result(
               recovery_source, sizeof(recovery_source) - 1u,
               false, NULL, false, true) &&
        expect_native_parser_result(
               malformed_defer, sizeof(malformed_defer) - 1u,
               false, "invalid_defer_statement", false, true) &&
        expect_native_parser_result(
               malformed_assert, sizeof(malformed_assert) - 1u,
               false, "invalid_assert_statement", false, true) &&
        expect_native_parser_result(
               malformed_unsafe, sizeof(malformed_unsafe) - 1u,
               false, "invalid_unsafe_statement", false, true) &&
        expect_native_parser_result(
               malformed_assembly, sizeof(malformed_assembly) - 1u,
               false, "invalid_expression_list", false, true) &&
        expect_native_parser_result(
               malformed_if, sizeof(malformed_if) - 1u,
               false, "invalid_control_flow_statement", false, true) &&
        expect_native_parser_result(
               dangling_else, sizeof(dangling_else) - 1u,
               false, "invalid_control_flow_statement", false, true) &&
        expect_native_parser_result(
               malformed_while, sizeof(malformed_while) - 1u,
               false, "invalid_control_flow_statement", false, true) &&
        expect_native_parser_result(
               malformed_loop, sizeof(malformed_loop) - 1u,
               false, "invalid_control_flow_statement", false, true) &&
        expect_native_parser_result(
               malformed_for, sizeof(malformed_for) - 1u,
               false, "invalid_control_flow_statement", false, true) &&
        expect_native_parser_result(
               malformed_match, sizeof(malformed_match) - 1u,
               false, "invalid_match_statement", false, true) &&
        expect_native_parser_result(
               malformed_match_arm, sizeof(malformed_match_arm) - 1u,
               false, "invalid_match_arm", false, true) &&
        expect_native_parser_result(
               malformed_tuple_pattern, sizeof(malformed_tuple_pattern) - 1u,
               false, "invalid_pattern", false, true) &&
        expect_native_parser_result(
               malformed_struct_pattern_with_rest,
               sizeof(malformed_struct_pattern_with_rest) - 1u,
               false, "invalid_pattern", false, true) &&
        expect_native_parser_result(
               invalid_range_pattern, sizeof(invalid_range_pattern) - 1u,
               false, "invalid_range_pattern", false, true) &&
        expect_native_parser_result(
               malformed_assignment_expression,
               sizeof(malformed_assignment_expression) - 1u,
               false, "invalid_assignment_expression", false, true) &&
        expect_native_parser_result(
               malformed_coalescing_expression,
               sizeof(malformed_coalescing_expression) - 1u,
               false, "invalid_coalescing_expression", false, true) &&
        expect_native_parser_result(
               malformed_logical_expression,
               sizeof(malformed_logical_expression) - 1u,
               false, "invalid_logical_expression", false, true) &&
        expect_native_parser_result(
               malformed_bitwise_expression,
               sizeof(malformed_bitwise_expression) - 1u,
               false, "invalid_bitwise_expression", false, true) &&
        expect_native_parser_result(
               malformed_comparison_expression,
               sizeof(malformed_comparison_expression) - 1u,
               false, "invalid_comparison_expression", false, true) &&
        expect_native_parser_result(
               malformed_shift_expression,
               sizeof(malformed_shift_expression) - 1u,
               false, "invalid_shift_expression", false, true) &&
        expect_native_parser_result(
               malformed_arithmetic_expression,
               sizeof(malformed_arithmetic_expression) - 1u,
               false, "invalid_arithmetic_expression", false, true) &&
        expect_native_parser_result(
               malformed_range_expression,
               sizeof(malformed_range_expression) - 1u,
               false, "invalid_range_expression", false, true) &&
        expect_native_parser_result(
               malformed_unary_expression,
               sizeof(malformed_unary_expression) - 1u,
               false, "invalid_unary_expression", false, true) &&
        expect_native_parser_result(
               malformed_cast_expression,
               sizeof(malformed_cast_expression) - 1u,
               false, "invalid_cast_expression", false, true) &&
        expect_native_parser_result(
               cast_missing_value, sizeof(cast_missing_value) - 1u,
               false, "invalid_cast_expression", false, true) &&
        expect_native_parser_result(
               malformed_precedence_operator,
               sizeof(malformed_precedence_operator) - 1u,
               false, "invalid_arithmetic_expression", false, true) &&
        expect_native_parser_result(
               malformed_call_suffix, sizeof(malformed_call_suffix) - 1u,
               false, "invalid_postfix_expression", false, true) &&
        expect_native_parser_result(
               malformed_index_suffix, sizeof(malformed_index_suffix) - 1u,
               false, "invalid_postfix_expression", false, true) &&
        expect_native_parser_result(
               malformed_field_suffix, sizeof(malformed_field_suffix) - 1u,
               false, "invalid_postfix_expression", false, true) &&
        expect_native_parser_result(
               malformed_optional_postfix,
               sizeof(malformed_optional_postfix) - 1u,
               false, "invalid_postfix_expression", false, true) &&
        expect_native_parser_result(
               malformed_form_initializer,
               sizeof(malformed_form_initializer) - 1u,
               false, "invalid_postfix_expression", false, true) &&
        expect_native_parser_result(
               missing_if_expression_else,
               sizeof(missing_if_expression_else) - 1u,
               false, "invalid_if_expression", false, true) &&
        expect_native_parser_result(
               if_condition_preserves_operator_error,
               sizeof(if_condition_preserves_operator_error) - 1u,
               false, "invalid_arithmetic_expression", false, true) &&
        expect_native_parser_result(
               malformed_block_expression,
               sizeof(malformed_block_expression) - 1u,
               false, "invalid_block_expression", false, true) &&
        expect_native_parser_result(
               empty_generic_arguments, sizeof(empty_generic_arguments) - 1u,
               true, NULL, false, false) &&
        expect_native_parser_result(
               node_limit_source, sizeof(node_limit_source) - 1u,
               false, NULL, true, false) &&
        expect_native_parser_result(
               missing_module_path, sizeof(missing_module_path) - 1u,
               false, "invalid_module_path", false, true) &&
        expect_native_parser_result(
               duplicate_module_header, sizeof(duplicate_module_header) - 1u,
               false, "duplicate_module_header", false, true) &&
        expect_native_parser_result(
               misplaced_module_header, sizeof(misplaced_module_header) - 1u,
               false, "misplaced_module_header", false, true) &&
        expect_native_parser_result(
               malformed_use, sizeof(malformed_use) - 1u,
               false, "invalid_use_declaration", false, true) &&
        expect_native_parser_result(
               invalid_use_alias, sizeof(invalid_use_alias) - 1u,
               false, "invalid_use_alias", false, true) &&
        expect_native_parser_result(
               malformed_import_group, sizeof(malformed_import_group) - 1u,
               false, "invalid_import_group", false, true) &&
        expect_native_parser_result(
               invalid_nested_glob, sizeof(invalid_nested_glob) - 1u,
               false, "invalid_glob_import", false, true) &&
        expect_native_parser_result(
               malformed_export, sizeof(malformed_export) - 1u,
               false, "invalid_export_clause", false, true) &&
        expect_native_parser_result(
               empty_export_group, sizeof(empty_export_group) - 1u,
               false, "empty_export_group", false, true) &&
        expect_native_parser_result(
               invalid_export_alias, sizeof(invalid_export_alias) - 1u,
               false, "invalid_export_alias", false, true) &&
        expect_native_parser_result(
               trailing_use_tokens, sizeof(trailing_use_tokens) - 1u,
               false, "trailing_tokens", false, true) &&
        expect_native_parser_result(
               trailing_export_tokens, sizeof(trailing_export_tokens) - 1u,
               false, "trailing_tokens", false, true) &&
        expect_native_parser_result(
               missing_module_header, sizeof(missing_module_header) - 1u,
               false, "missing_module_header", false, true) &&
        expect_native_parser_result(
               missing_export_header, sizeof(missing_export_header) - 1u,
               false, "missing_module_header", false, true) &&
        expect_native_parser_result(
               valid_declarations, sizeof(valid_declarations) - 1u,
               true, NULL, false, false) &&
        expect_native_parser_result(
               missing_value_name, sizeof(missing_value_name) - 1u,
               false, "invalid_value_declaration", false, true) &&
        expect_native_parser_result(
               missing_value_equal, sizeof(missing_value_equal) - 1u,
               false, "missing_value_initializer", false, true) &&
        expect_native_parser_result(
               missing_value_expression, sizeof(missing_value_expression) - 1u,
               false, "missing_value_initializer", false, true) &&
        expect_native_parser_result(
               missing_value_semicolon, sizeof(missing_value_semicolon) - 1u,
               false, "invalid_value_declaration", false, true) &&
        expect_native_parser_result(
               missing_type_alias, sizeof(missing_type_alias) - 1u,
               false, "invalid_type_declaration", false, true) &&
        expect_native_parser_result(
               invalid_type_bound, sizeof(invalid_type_bound) - 1u,
               false, "invalid_type_declaration", false, true) &&
        expect_native_parser_result(
               invalid_procedure_type, sizeof(invalid_procedure_type) - 1u,
               false, "invalid_procedure_type", false, true) &&
        expect_native_parser_result(
               invalid_reference_type, sizeof(invalid_reference_type) - 1u,
               false, "invalid_reference_type", false, true) &&
        expect_native_parser_result(
               invalid_pointer_type, sizeof(invalid_pointer_type) - 1u,
               false, "invalid_pointer_type", false, true) &&
        expect_native_parser_result(
               invalid_array_type, sizeof(invalid_array_type) - 1u,
               false, "invalid_array_type", false, true) &&
        expect_native_parser_result(
               invalid_slice_type, sizeof(invalid_slice_type) - 1u,
               false, "invalid_slice_type", false, true) &&
        expect_native_parser_result(
               invalid_optional_type, sizeof(invalid_optional_type) - 1u,
               false, "invalid_optional_type", false, true) &&
        expect_native_parser_result(
               invalid_type_path, sizeof(invalid_type_path) - 1u,
               false, "invalid_type_path", false, true) &&
        expect_native_parser_result(
               invalid_type_separator, sizeof(invalid_type_separator) - 1u,
               false, "invalid_type_path", false, true) &&
        expect_native_parser_result(
               invalid_angle_type_arguments,
               sizeof(invalid_angle_type_arguments) - 1u,
               false, "invalid_generic_arguments", false, true) &&
        expect_native_parser_result(
               invalid_bracket_type_arguments,
               sizeof(invalid_bracket_type_arguments) - 1u,
               false, "invalid_generic_arguments", false, true) &&
        expect_native_parser_result(
               invalid_let_pattern, sizeof(invalid_let_pattern) - 1u,
               false, "invalid_let_statement", false, true) &&
        expect_native_parser_result(
               invalid_let_type, sizeof(invalid_let_type) - 1u,
               false, "invalid_let_statement", false, true) &&
        expect_native_parser_result(
               invalid_let_initializer,
               sizeof(invalid_let_initializer) - 1u,
               false, "invalid_let_statement", false, true) &&
        expect_native_parser_result(
               missing_set_operator, sizeof(missing_set_operator) - 1u,
               false, "invalid_assignment_operator", false, true) &&
        expect_native_parser_result(
               invalid_set_operator, sizeof(invalid_set_operator) - 1u,
               false, "invalid_assignment_operator", false, true) &&
        expect_native_parser_result(
               invalid_set_value, sizeof(invalid_set_value) - 1u,
               false, "invalid_set_statement", false, true) &&
        expect_native_parser_result(
               malformed_return, sizeof(malformed_return) - 1u,
               false, "invalid_return_statement", false, true) &&
        expect_native_parser_result(
               malformed_jump, sizeof(malformed_jump) - 1u,
               false, "invalid_jump_statement", false, true) &&
        expect_native_parser_result(
               missing_let_terminator,
               sizeof(missing_let_terminator) - 1u,
               false, "invalid_let_statement", false, true) &&
        expect_native_parser_result(
               missing_set_terminator,
               sizeof(missing_set_terminator) - 1u,
               false, "invalid_set_statement", false, true) &&
        expect_native_parser_result(
               missing_jump_terminator,
               sizeof(missing_jump_terminator) - 1u,
               false, "invalid_jump_statement", false, true) &&
        expect_native_parser_result(
               return_outside_procedure,
               sizeof(return_outside_procedure) - 1u,
               false, "invalid_return_statement", false, true) &&
        expect_native_parser_result(
               jump_outside_procedure,
               sizeof(jump_outside_procedure) - 1u,
               false, "invalid_jump_statement", false, true) &&
        expect_native_parser_result(
               return_without_final_semicolon,
               sizeof(return_without_final_semicolon) - 1u,
               true, NULL, false, false) &&
        expect_native_parser_result(
               missing_opaque_alias, sizeof(missing_opaque_alias) - 1u,
               false, "invalid_opaque_declaration", false, true) &&
        expect_native_parser_result(
               invalid_opaque_bound, sizeof(invalid_opaque_bound) - 1u,
               false, "invalid_opaque_declaration", false, true) &&
        expect_native_parser_result(
               missing_form_name, sizeof(missing_form_name) - 1u,
               false, "invalid_form_declaration", false, true) &&
        expect_native_parser_result(
               malformed_form_field, sizeof(malformed_form_field) - 1u,
               false, "invalid_form_declaration", false, true) &&
        expect_native_parser_result(
               duplicate_form_field, sizeof(duplicate_form_field) - 1u,
               false, "duplicate_form_field", false, true) &&
        expect_native_parser_result(
               missing_form_separator, sizeof(missing_form_separator) - 1u,
               false, "invalid_form_field_separator", false, true) &&
        expect_native_parser_result(
               missing_form_closer, sizeof(missing_form_closer) - 1u,
               false, "invalid_form_declaration", false, true) &&
        expect_native_parser_result(
               malformed_pick, sizeof(malformed_pick) - 1u,
               false, "invalid_pick_declaration", false, true) &&
        expect_native_parser_result(
               duplicate_pick_variant, sizeof(duplicate_pick_variant) - 1u,
               false, "duplicate_pick_variant", false, true) &&
        expect_native_parser_result(
               invalid_pick_payload, sizeof(invalid_pick_payload) - 1u,
               false, "invalid_pick_variant_payload", false, true) &&
        expect_native_parser_result(
               malformed_trait, sizeof(malformed_trait) - 1u,
               false, "invalid_trait_declaration", false, true) &&
        expect_native_parser_result(
               malformed_impl, sizeof(malformed_impl) - 1u,
               false, "invalid_impl_declaration", false, true) &&
        expect_native_parser_result(
               missing_impl_target, sizeof(missing_impl_target) - 1u,
               false, "missing_impl_target_type", false, true) &&
        expect_native_parser_result(
               invalid_impl_for_clause, sizeof(invalid_impl_for_clause) - 1u,
               false, "invalid_impl_for_clause", false, true) &&
        expect_native_parser_result(
               malformed_procedure, sizeof(malformed_procedure) - 1u,
               false, "invalid_procedure_declaration", false, true) &&
        expect_native_parser_result(
               malformed_procedure_parameters,
               sizeof(malformed_procedure_parameters) - 1u,
               false, "invalid_procedure_parameters", false, true) &&
        expect_native_parser_result(
               missing_procedure_parameter_closer,
               sizeof(missing_procedure_parameter_closer) - 1u,
               false, "invalid_procedure_parameters", false, true) &&
        expect_native_parser_result(
               invalid_procedure_return, sizeof(invalid_procedure_return) - 1u,
               false, "invalid_procedure_return_type", false, true) &&
        expect_native_parser_result(
               duplicate_procedure_modifier,
               sizeof(duplicate_procedure_modifier) - 1u,
               false, "invalid_procedure_modifier", false, true) &&
        expect_native_parser_result(
               misplaced_procedure_modifier,
               sizeof(misplaced_procedure_modifier) - 1u,
               false, "invalid_procedure_modifier", false, true) &&
        expect_native_parser_result(
               invalid_procedure_body, sizeof(invalid_procedure_body) - 1u,
               false, "invalid_procedure_body_form", false, true) &&
        expect_native_parser_result(
               invalid_external_procedure_body,
               sizeof(invalid_external_procedure_body) - 1u,
               false, "invalid_procedure_body_form", false, true) &&
        expect_native_parser_result(
               missing_external_procedure_semicolon,
               sizeof(missing_external_procedure_semicolon) - 1u,
               false, "invalid_procedure_body_form", false, true);
}

int
main(int argc, char **argv)
{
    native_storage_t lexer_storage = { 0 };
    native_storage_t parser_storage = { 0 };
    native_lexer_token_t *lexer_token;
    native_parser_token_t *parser_tokens = NULL;
    char *source;
    size_t source_length = 0u;
    size_t token_count;
    size_t index;
    int result = EXIT_FAILURE;

    if (argc != 2) {
        (void)fprintf(stderr, "usage: %s CANONICAL_GRAMMAR_FIXTURE\n", argv[0]);
        return EXIT_FAILURE;
    }
    source = read_source_file(argv[1], &source_length);
    if (source == NULL) {
        (void)fprintf(stderr, "could not read fixture: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    if (!compiler_lexer_lexer_vitte_lexer_init(
            lexer_storage.bytes,
            source,
            source_length,
            UINT64_C(1)) ||
        !compiler_lexer_lexer_vitte_lexer_run(lexer_storage.bytes)) {
        (void)fprintf(stderr, "native lexer rejected canonical fixture\n");
        goto cleanup_source;
    }

    token_count = compiler_lexer_lexer_vitte_lexer_token_count(
        lexer_storage.bytes);
    parser_tokens = (native_parser_token_t *)calloc(
        token_count,
        sizeof(*parser_tokens));
    if (parser_tokens == NULL && token_count != 0u) {
        (void)fprintf(stderr, "could not allocate native parser tokens\n");
        goto cleanup_lexer;
    }
    for (index = 0u; index < token_count; ++index) {
        lexer_token = compiler_lexer_lexer_vitte_lexer_token_at(
            lexer_storage.bytes,
            index);
        if (lexer_token == NULL) {
            (void)fprintf(stderr, "native lexer returned a missing token\n");
            goto cleanup_tokens;
        }
        parser_tokens[index].kind = lexer_token->kind;
        parser_tokens[index].span.file_id = (uint32_t)lexer_token->span.file_id;
        parser_tokens[index].span.begin = lexer_token->span.begin;
        parser_tokens[index].span.end = lexer_token->span.end;
        parser_tokens[index].span.valid = lexer_token->span.valid;
        parser_tokens[index].line = lexer_token->line;
        parser_tokens[index].column = lexer_token->column;
        parser_tokens[index].lexeme = lexer_token->lexeme;
        parser_tokens[index].length = lexer_token->length;
    }

    if (!vitte_parser_init(
            parser_storage.bytes,
            parser_tokens,
            token_count)) {
        (void)fprintf(stderr, "native parser initialization failed\n");
        goto cleanup_tokens;
    }
    if (!vitte_parser_run_native(parser_storage.bytes)) {
        const size_t position = vitte_parser_position(parser_storage.bytes);
        const char *error_name = vitte_parser_error_name(
            vitte_parser_last_error(parser_storage.bytes));
        native_parser_diagnostic_t *diagnostic =
            vitte_parser_diagnostic_at(parser_storage.bytes, 0u);
        const char *expected_name = diagnostic != NULL
            ? compiler_lexer_lexer_vitte_token_kind_name(diagnostic->expected)
            : NULL;

        lexer_token = compiler_lexer_lexer_vitte_lexer_token_at(
            lexer_storage.bytes,
            position);
        (void)fprintf(
            stderr,
            "native parser rejected canonical fixture: %s at %zu:%zu (diagnostic %zu:%zu) near '%.*s'; expected %s\n",
            error_name != NULL ? error_name : "unknown",
            lexer_token != NULL ? lexer_token->line : 0u,
            lexer_token != NULL ? lexer_token->column : 0u,
            diagnostic != NULL ? diagnostic->span.line : 0u,
            diagnostic != NULL ? diagnostic->span.column : 0u,
            lexer_token != NULL ? (int)lexer_token->length : 0,
            lexer_token != NULL && lexer_token->lexeme != NULL
                ? lexer_token->lexeme
                : "",
            expected_name != NULL ? expected_name : "unknown");
        vitte_parser_destroy(parser_storage.bytes);
        goto cleanup_tokens;
    }
    if (!vitte_parser_validate(parser_storage.bytes) ||
        vitte_parser_root(parser_storage.bytes) == 0u) {
        (void)fprintf(stderr, "native parser produced an invalid AST\n");
        vitte_parser_destroy(parser_storage.bytes);
        goto cleanup_tokens;
    }

    (void)printf(
        "Native lexer/parser accepted canonical grammar fixture; AST nodes: %zu.\n",
        vitte_parser_node_count(parser_storage.bytes));
    vitte_parser_destroy(parser_storage.bytes);
    if (!run_native_regression_cases()) {
        goto cleanup_tokens;
    }
    result = EXIT_SUCCESS;

cleanup_tokens:
    free(parser_tokens);
cleanup_lexer:
    compiler_lexer_lexer_vitte_lexer_destroy(lexer_storage.bytes);
cleanup_source:
    free(source);
    return result;
}
