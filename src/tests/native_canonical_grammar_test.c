#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        if (result && validate_partial_ast) {
            result = vitte_parser_validate(parser_storage.bytes);
        }
    }
    if (!result) {
        (void)fprintf(
            stderr,
            "native parser regression failed (accepted=%d, node_limit=%d, "
            "diagnostics=%zu, error=%s)\n",
            should_parse,
            limit_nodes,
            vitte_parser_diagnostic_count(parser_storage.bytes),
            vitte_parser_error_name(
                vitte_parser_last_error(parser_storage.bytes)));
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
        "proc valid() { let items = [1, 2, 3]; return; }";
    static const char malformed_source[] =
        "proc malformed() { let value = ; }";
    static const char recovery_source[] =
        "proc malformed() { let value = ; return; } proc tail() {}";
    static const char node_limit_source[] = "proc limited() {}";
    static const char invalid_utf8[] = { (char)0xC0, (char)0xAF, '\0' };
    static const char invalid_number[] = "0b102";
    static const char invalid_escape[] = "\"bad\\q\"";

    return expect_native_lexer_error(
               invalid_utf8, sizeof(invalid_utf8) - 1u, "invalid_utf8") &&
        expect_native_lexer_error(
               invalid_number, sizeof(invalid_number) - 1u,
               "invalid_number") &&
        expect_native_lexer_error(
               invalid_escape, sizeof(invalid_escape) - 1u,
               "invalid_escape") &&
        expect_native_token_lexemes() &&
        expect_native_keyword_mappings() &&
        expect_native_parser_result(
               valid_source, sizeof(valid_source) - 1u, true, false, false) &&
        expect_native_parser_result(
               malformed_source, sizeof(malformed_source) - 1u,
               false, false, true) &&
        expect_native_parser_result(
               recovery_source, sizeof(recovery_source) - 1u,
               false, false, true) &&
        expect_native_parser_result(
               node_limit_source, sizeof(node_limit_source) - 1u,
               false, true, false);
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
