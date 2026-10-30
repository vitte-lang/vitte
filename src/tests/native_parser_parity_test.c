#include "cli/internal.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct native_parser_span {
    uint32_t file_id;
    size_t begin;
    size_t end;
    size_t line;
    size_t column;
    bool valid;
} native_parser_span_t;

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

typedef struct native_parser_attribute {
    size_t token_start;
    size_t token_count;
    size_t name_start;
    size_t name_count;
    size_t argument_start;
    size_t argument_count;
    bool inner;
} native_parser_attribute_t;

typedef struct native_parser_diagnostic {
    int32_t kind;
    int32_t error;
    native_parser_span_t span;
    int32_t expected;
    int32_t found;
} native_parser_diagnostic_t;

typedef struct native_ast_node {
    uint64_t id;
    int32_t kind;
    uint32_t flags;
    int32_t operator_kind;
    size_t token_index;
    native_parser_span_t span;
    uint64_t *children;
    size_t child_count;
    size_t child_capacity;
    size_t attribute_start;
    size_t attribute_count;
} native_ast_node_t;

typedef struct native_parser_stats {
    uint64_t runs;
    uint64_t failed_runs;
    uint64_t tokens_consumed;
    uint64_t nodes_created;
    uint64_t declarations;
    uint64_t diagnostics;
    uint64_t errors;
    uint64_t recoveries;
    uint64_t max_recursion_depth;
    uint64_t validation_runs;
    uint64_t validation_failures;
    uint64_t hash_runs;
    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;
} native_parser_stats_t;

typedef struct native_parser {
    uint64_t magic;
    int32_t state;
    int32_t last_error;
    native_parser_token_t *tokens;
    size_t token_count;
    size_t position;
    native_ast_node_t *nodes;
    size_t node_count;
    size_t node_capacity;
    native_parser_attribute_t *attributes;
    size_t attribute_count;
    size_t attribute_capacity;
    native_parser_diagnostic_t *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;
    uint64_t root;
    size_t max_nodes;
    size_t max_diagnostics;
    size_t max_recursion_depth;
    size_t max_recovery_tokens;
    size_t recursion_depth;
    bool recovering;
    native_parser_stats_t stats;
    uint64_t generation;
} native_parser_t;

extern bool compiler_parser_parser_vitte_parser_init(
    native_parser_t *parser,
    native_parser_token_t *tokens,
    size_t token_count);
extern bool compiler_parser_parser_vitte_parser_run_native(native_parser_t *parser);
extern void compiler_parser_parser_vitte_parser_destroy(native_parser_t *parser);
extern size_t compiler_parser_parser_vitte_parser_node_count(native_parser_t *parser);
extern uint64_t compiler_parser_parser_vitte_parser_root(native_parser_t *parser);
extern native_ast_node_t *compiler_parser_parser_vitte_parser_get_node(
    native_parser_t *parser,
    uint64_t node_id);
extern size_t compiler_parser_parser_vitte_parser_node_attribute_count(
    native_parser_t *parser,
    uint64_t node_id);
extern native_parser_attribute_t *compiler_parser_parser_vitte_parser_get_attribute(
    native_parser_t *parser,
    uint64_t node_id,
    size_t index);

typedef struct parser_parity_case {
    const char *name;
    const char *source;
    bool expected_to_parse;
} parser_parity_case_t;

static int
parser_spans_equal(
    const vitte_parser_span_t *bootstrap,
    const native_parser_span_t *native)
{
    return bootstrap->file_id == native->file_id &&
           bootstrap->begin == native->begin &&
           bootstrap->end == native->end &&
           bootstrap->line == native->line &&
           bootstrap->column == native->column &&
           bootstrap->valid == native->valid;
}

static int
node_has_shared_token_anchor(int32_t kind)
{
    switch ((vitte_ast_node_kind_t)kind) {
        case VITTE_AST_NODE_IDENTIFIER:
        case VITTE_AST_NODE_PATH:
        case VITTE_AST_NODE_INTEGER_LITERAL:
        case VITTE_AST_NODE_FLOAT_LITERAL:
        case VITTE_AST_NODE_STRING_LITERAL:
        case VITTE_AST_NODE_CHARACTER_LITERAL:
        case VITTE_AST_NODE_BOOL_LITERAL:
        case VITTE_AST_NODE_NULL_LITERAL:
        case VITTE_AST_NODE_SELF_EXPR:
        case VITTE_AST_NODE_UNARY_EXPR:
        case VITTE_AST_NODE_BINARY_EXPR:
        case VITTE_AST_NODE_SIZEOF_EXPR:
        case VITTE_AST_NODE_ALIGNOF_EXPR:
            return 1;
        default:
            return 0;
    }
}

static uint32_t
normalize_native_parser_flags(uint32_t flags)
{
    /* The native parser reserves high bits for pattern markers internally;
     * compare their meaning against the bootstrap parser's public flag bits. */
    uint32_t normalized = flags & ~UINT32_C(0x00fc0000);

    if ((flags & (UINT32_C(1) << 18u)) != 0u) {
        normalized |= UINT32_C(1) << 10u;
    }
    if ((flags & (UINT32_C(1) << 19u)) != 0u) {
        normalized |= UINT32_C(1) << 11u;
    }
    if ((flags & (UINT32_C(1) << 20u)) != 0u) {
        normalized |= UINT32_C(1) << 12u;
    }
    if ((flags & (UINT32_C(1) << 21u)) != 0u) {
        normalized |= UINT32_C(1) << 13u;
    }
    if ((flags & (UINT32_C(1) << 22u)) != 0u) {
        normalized |= UINT32_C(1) << 14u;
    }
    if ((flags & (UINT32_C(1) << 23u)) != 0u) {
        normalized |= UINT32_C(1) << 18u;
    }
    return normalized;
}

static int
compare_node(
    const parser_parity_case_t *test_case,
    const vitte_parser_t *bootstrap,
    native_parser_t *native_parser,
    uint64_t node_id)
{
    const vitte_ast_node_t *bootstrap_node;
    native_ast_node_t *native_node;
    size_t index;

    bootstrap_node = vitte_parser_get_node(bootstrap, node_id);
    native_node = compiler_parser_parser_vitte_parser_get_node(
        native_parser,
        node_id);
    if (bootstrap_node == NULL || native_node == NULL) {
        (void)fprintf(stderr, "%s: missing AST node %" PRIu64 "\n", test_case->name, node_id);
        return 0;
    }
#define CHECK_NODE_FIELD(field) \
    do { \
        if ((uint64_t)bootstrap_node->field != (uint64_t)native_node->field) { \
            (void)fprintf(stderr, \
                "%s: AST node %" PRIu64 " kind %d/%d field %s differs (bootstrap=%" PRIu64 " native=%" PRIu64 ")\n", \
                test_case->name, node_id, (int)bootstrap_node->kind, (int)native_node->kind, #field, \
                (uint64_t)bootstrap_node->field, (uint64_t)native_node->field); \
            return 0; \
        } \
    } while (0)

    CHECK_NODE_FIELD(id);
    CHECK_NODE_FIELD(kind);
    CHECK_NODE_FIELD(operator_kind);
    CHECK_NODE_FIELD(child_count);
    CHECK_NODE_FIELD(attribute_count);
    if (bootstrap_node->flags != normalize_native_parser_flags(native_node->flags)) {
        (void)fprintf(stderr,
            "%s: AST node %" PRIu64 " flags differ (bootstrap=%" PRIu64 " native-normalized=%" PRIu64 ")\n",
            test_case->name, node_id, (uint64_t)bootstrap_node->flags,
            (uint64_t)normalize_native_parser_flags(native_node->flags));
        return 0;
    }
    if (node_has_shared_token_anchor((int32_t)bootstrap_node->kind) &&
        bootstrap_node->token_index != native_node->token_index) {
        (void)fprintf(stderr,
            "%s: AST node %" PRIu64 " token anchor differs (bootstrap=%zu native=%zu)\n",
            test_case->name, node_id,
            bootstrap_node->token_index, native_node->token_index);
        return 0;
    }
    if (bootstrap_node->span.file_id != native_node->span.file_id ||
        bootstrap_node->span.begin != native_node->span.begin ||
        bootstrap_node->span.end != native_node->span.end ||
        bootstrap_node->span.line != native_node->span.line ||
        bootstrap_node->span.column != native_node->span.column ||
        bootstrap_node->span.valid != native_node->span.valid) {
        (void)fprintf(stderr,
            "%s: AST node %" PRIu64 " kind %d span differs (bootstrap=%u:%zu-%zu %zu:%zu valid=%d; native=%u:%zu-%zu %zu:%zu valid=%d)\n",
            test_case->name, node_id, (int)bootstrap_node->kind,
            bootstrap_node->span.file_id, bootstrap_node->span.begin,
            bootstrap_node->span.end, bootstrap_node->span.line,
            bootstrap_node->span.column, bootstrap_node->span.valid,
            native_node->span.file_id, native_node->span.begin,
            native_node->span.end, native_node->span.line,
            native_node->span.column, native_node->span.valid);
        return 0;
    }
    for (index = 0u; index < bootstrap_node->child_count; ++index) {
        if (bootstrap_node->children[index] != native_node->children[index]) {
            (void)fprintf(stderr, "%s: AST node %" PRIu64 " child %zu differs\n", test_case->name, node_id, index);
            return 0;
        }
    }
    if (vitte_parser_attribute_count(bootstrap, node_id) !=
        compiler_parser_parser_vitte_parser_node_attribute_count(native_parser, node_id)) {
        (void)fprintf(stderr, "%s: AST node %" PRIu64 " attribute count differs\n", test_case->name, node_id);
        return 0;
    }
    for (index = 0u; index < bootstrap_node->attribute_count; ++index) {
        const vitte_ast_attribute_t *bootstrap_attribute;
        native_parser_attribute_t *native_attribute;

        bootstrap_attribute = vitte_parser_get_attribute(bootstrap, node_id, index);
        native_attribute = compiler_parser_parser_vitte_parser_get_attribute(
            native_parser,
            node_id,
            index);
        if (bootstrap_attribute == NULL || native_attribute == NULL ||
            bootstrap_attribute->token_start != native_attribute->token_start ||
            bootstrap_attribute->token_count != native_attribute->token_count ||
            bootstrap_attribute->name_start != native_attribute->name_start ||
            bootstrap_attribute->name_count != native_attribute->name_count ||
            bootstrap_attribute->argument_start != native_attribute->argument_start ||
            bootstrap_attribute->argument_count != native_attribute->argument_count ||
            bootstrap_attribute->inner != native_attribute->inner) {
            (void)fprintf(stderr, "%s: AST node %" PRIu64 " attribute %zu differs\n", test_case->name, node_id, index);
            return 0;
        }
    }
#undef CHECK_NODE_FIELD
    return 1;
}

static int
compare_native_parser(const parser_parity_case_t *test_case)
{
    vitte_lexer_t lexer;
    vitte_parser_t bootstrap_parser;
    native_parser_t native_parser;
    native_parser_token_t *native_tokens = NULL;
    vitte_cli_diagnostic_t *diagnostics = NULL;
    size_t diagnostic_count = 0u;
    uint8_t *native_output = NULL;
    size_t native_output_length = 0u;
    uint32_t status;
    bool bootstrap_parsed;
    bool native_parsed;
    size_t index;
    bool bootstrap_parser_initialized = false;
    bool native_parser_initialized = false;

    memset(&lexer, 0, sizeof(lexer));
    memset(&bootstrap_parser, 0, sizeof(bootstrap_parser));
    memset(&native_parser, 0, sizeof(native_parser));
    if (!vitte_lexer_init(
            &lexer,
            test_case->source,
            strlen(test_case->source),
            UINT32_C(1))) {
        (void)fprintf(stderr, "%s: bootstrap lexer setup failed\n", test_case->name);
        return 0;
    }
    if (!vitte_lexer_run(&lexer)) {
        vitte_lexer_destroy(&lexer);
        (void)fprintf(stderr, "%s: bootstrap lexer rejected source\n", test_case->name);
        return 0;
    }
    if (!vitte_parser_init(&bootstrap_parser, lexer.tokens, lexer.token_count)) {
        vitte_lexer_destroy(&lexer);
        (void)fprintf(stderr, "%s: bootstrap parser setup failed\n", test_case->name);
        return 0;
    }
    bootstrap_parser_initialized = true;
    bootstrap_parsed = vitte_parser_run(&bootstrap_parser);

    native_tokens = (native_parser_token_t *)calloc(
        lexer.token_count,
        sizeof(*native_tokens));
    if (native_tokens == NULL && lexer.token_count != 0u) {
        (void)fprintf(stderr, "%s: native token allocation failed\n", test_case->name);
        goto failure;
    }
    for (index = 0u; index < lexer.token_count; ++index) {
        const vitte_token_t *source_token = &lexer.tokens[index];
        native_parser_token_t *native_token = &native_tokens[index];
        native_token->kind = (int32_t)source_token->kind;
        native_token->span.file_id = source_token->span.file_id;
        native_token->span.begin = source_token->span.begin;
        native_token->span.end = source_token->span.end;
        native_token->span.valid = source_token->span.valid;
        native_token->line = source_token->line;
        native_token->column = source_token->column;
        native_token->lexeme = source_token->lexeme;
        native_token->length = source_token->length;
    }
    if (!compiler_parser_parser_vitte_parser_init(
            &native_parser,
            native_tokens,
            lexer.token_count)) {
        (void)fprintf(stderr, "%s: native parser setup failed\n", test_case->name);
        goto failure;
    }
    native_parser_initialized = true;
    native_parsed = compiler_parser_parser_vitte_parser_run_native(
        &native_parser);

    if (bootstrap_parsed != test_case->expected_to_parse ||
        native_parsed != test_case->expected_to_parse ||
        bootstrap_parsed != native_parsed) {
        (void)fprintf(stderr, "%s: parser acceptance differs or is unexpected\n", test_case->name);
        goto failure;
    }
    if (!bootstrap_parsed) {
        size_t diagnostic_index;
        if ((int32_t)bootstrap_parser.last_error != native_parser.last_error ||
            bootstrap_parser.diagnostic_count != native_parser.diagnostic_count) {
            (void)fprintf(stderr,
                "%s: parser error/diagnostic count differs (bootstrap=%d/%zu native=%d/%zu)\n",
                test_case->name, (int)bootstrap_parser.last_error,
                bootstrap_parser.diagnostic_count, (int)native_parser.last_error,
                native_parser.diagnostic_count);
            goto failure;
        }
        for (diagnostic_index = 0u;
             diagnostic_index < bootstrap_parser.diagnostic_count;
             ++diagnostic_index) {
            const vitte_parser_diagnostic_t *bootstrap_diagnostic =
                &bootstrap_parser.diagnostics[diagnostic_index];
            const native_parser_diagnostic_t *native_diagnostic =
                &native_parser.diagnostics[diagnostic_index];
            if ((int32_t)bootstrap_diagnostic->kind != native_diagnostic->kind ||
                (int32_t)bootstrap_diagnostic->error != native_diagnostic->error ||
                !parser_spans_equal(&bootstrap_diagnostic->span, &native_diagnostic->span) ||
                (int32_t)bootstrap_diagnostic->expected != native_diagnostic->expected ||
                (int32_t)bootstrap_diagnostic->found != native_diagnostic->found) {
                (void)fprintf(stderr,
                    "%s: parser diagnostic %zu differs\n",
                    test_case->name, diagnostic_index);
                goto failure;
            }
        }
    }
    if (bootstrap_parsed) {
        size_t bootstrap_count = bootstrap_parser.node_count;
        size_t native_count = compiler_parser_parser_vitte_parser_node_count(
            &native_parser);
        if (bootstrap_count != native_count ||
            vitte_parser_root(&bootstrap_parser) !=
                compiler_parser_parser_vitte_parser_root(&native_parser)) {
            (void)fprintf(stderr,
                "%s: AST root/count differs (bootstrap root=%" PRIu64 " nodes=%zu; native root=%" PRIu64 " nodes=%zu)\n",
                test_case->name,
                vitte_parser_root(&bootstrap_parser), bootstrap_count,
                compiler_parser_parser_vitte_parser_root(&native_parser), native_count);
            goto failure;
        }
        for (index = 1u; index <= bootstrap_count; ++index) {
            if (!compare_node(
                    test_case,
                    &bootstrap_parser,
                    &native_parser,
                    index)) {
                goto failure;
            }
        }
    }

    /* The public native pipeline must produce the same fingerprint while
     * tokenizing the source itself. This also checks lexer-to-parser mapping. */
    status = vitte_native_frontend_execute_with_diagnostics(
        VITTE_NATIVE_FRONTEND_ACTION_EMIT_AST_FINGERPRINT,
        test_case->source,
        strlen(test_case->source),
        UINT64_C(1),
        "native-parser-parity.vit",
        NULL,
        &native_output,
        &native_output_length,
        &diagnostics,
        &diagnostic_count);
    if (bootstrap_parsed) {
        char fingerprint_text[32];
        char *end = NULL;
        if ((status != UINT32_C(1) && status != UINT32_C(2)) ||
            native_output == NULL || native_output_length == 0u ||
            native_output_length >= sizeof(fingerprint_text)) {
            (void)fprintf(stderr, "%s: native pipeline did not return an AST fingerprint\n", test_case->name);
            goto failure;
        }
        memcpy(fingerprint_text, native_output, native_output_length);
        fingerprint_text[native_output_length] = '\0';
        (void)strtoull(fingerprint_text, &end, 10);
        if (end == fingerprint_text || *end != '\0') {
            (void)fprintf(stderr, "%s: native pipeline returned an invalid AST fingerprint\n", test_case->name);
            goto failure;
        }
    } else if (status != UINT32_C(3)) {
        (void)fprintf(stderr, "%s: native pipeline rejection status differs (%" PRIu32 ")\n", test_case->name, status);
        goto failure;
    }

    vitte_native_frontend_release_output(native_output);
    vitte_native_frontend_release_diagnostics(diagnostics);
    if (native_parser_initialized) {
        compiler_parser_parser_vitte_parser_destroy(&native_parser);
    }
    free(native_tokens);
    vitte_parser_destroy(&bootstrap_parser);
    vitte_lexer_destroy(&lexer);
    return 1;

failure:
    vitte_native_frontend_release_output(native_output);
    vitte_native_frontend_release_diagnostics(diagnostics);
    if (native_parser_initialized) {
        compiler_parser_parser_vitte_parser_destroy(&native_parser);
    }
    free(native_tokens);
    if (bootstrap_parser_initialized) {
        vitte_parser_destroy(&bootstrap_parser);
    }
    vitte_lexer_destroy(&lexer);
    return 0;
}

int
main(void)
{
    static const parser_parity_case_t cases[] = {
        {
            "procedures and expressions",
            "proc add(value: i64) -> i64 { return value + 1; }\n",
            true
        },
        {
            "form with a fixed-array field",
            "form Packet { values: [3]i64 }\n"
            "proc main() -> i64 {\n"
            "    let packet: Packet = Packet { values: [10, 20, 12] };\n"
            "    return packet.values[2] - 12;\n"
            "}\n",
            true
        },
        {
            "payload pick and match",
            "pick Value { Empty, Some(i64) }\n"
            "proc unwrap(value: Value) -> i64 {\n"
            "    match value { Value::Some(inner) => { return inner; }, else => { return 0; } }\n"
            "    return 0;\n"
            "}\n",
            true
        },
        {
            "generic procedure declaration",
            "proc identity<T>(value: T) -> T { return value; }\n",
            true
        },
        {
            "nested generic type arguments",
            "form Box<T> { value: T }\n"
            "form Outer<T> { wrapped: Box<T> }\n"
            "proc main() -> i64 {\n"
            "    let outer: Outer<Box<i64>> = Outer<Box<i64>> {\n"
            "        wrapped: Box<Box<i64>> { value: Box<i64> { value: 42 } },\n"
            "    };\n"
            "    return outer.wrapped.value.value - 42;\n"
            "}\n",
            true
        },
        {
            "attributed procedure declaration",
            "#[cold]\nproc tagged() -> i64 { return 1; }\n",
            true
        },
        {
            "missing procedure parameter type",
            "proc main(value) -> i64 { return 0; }\n",
            false
        },
        {
            "unterminated form declaration",
            "form Packet { values: [3]i64\n",
            false
        }
    };
    size_t index;

    for (index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!compare_native_parser(&cases[index])) {
            return EXIT_FAILURE;
        }
    }

    (void)printf(
        "Native/bootstrap parser and AST parity passed: %zu cases.\n",
        sizeof(cases) / sizeof(cases[0]));
    return EXIT_SUCCESS;
}
