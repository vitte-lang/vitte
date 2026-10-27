#include "arena/arena.h"
#include "import/import.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void
test_arena_failures(void)
{
    vitte_arena_context_t arena;

    assert(vitte_arena_context_init_default(&arena));
    assert(vitte_arena_context_set_memory_limit(&arena, 8u));
    assert(vitte_arena_context_alloc(&arena, 16u) == NULL);
    assert(vitte_arena_context_limit_exceeded(&arena));
    assert(vitte_arena_context_validate(&arena));
    assert(vitte_arena_context_alloc_array(
        &arena, SIZE_MAX, 2u, 0u) == NULL);
    assert(vitte_arena_context_last_error(&arena) ==
           VITTE_ARENA_CONTEXT_ERROR_OVERFLOW);
    vitte_arena_context_destroy(&arena);
}

static void
test_lexer_limits(void)
{
    static const char nested_comment[] = "/* outer /* inner */";
    static const char invalid_utf8[] = "\xC0\xAF";
    char *large_source;
    vitte_lexer_t lexer;

    large_source = (char *)malloc(
        VITTE_LEXER_DEFAULT_MAX_SOURCE_BYTES + 1u);
    assert(large_source != NULL);
    memset(large_source, 'a',
           VITTE_LEXER_DEFAULT_MAX_SOURCE_BYTES + 1u);
    assert(!vitte_lexer_init(
        &lexer,
        large_source,
        VITTE_LEXER_DEFAULT_MAX_SOURCE_BYTES + 1u,
        UINT32_C(201)));
    assert(vitte_lexer_last_error(&lexer) ==
           VITTE_LEXER_ERROR_SOURCE_TOO_LARGE);
    vitte_lexer_destroy(&lexer);
    free(large_source);

    assert(vitte_lexer_init(
        &lexer,
        "a b c d e",
        strlen("a b c d e"),
        UINT32_C(202)));
    assert(vitte_lexer_set_limits(&lexer, 3u, 4096u, 16u));
    assert(!vitte_lexer_run(&lexer));
    assert(vitte_lexer_last_error(&lexer) == VITTE_LEXER_ERROR_TOKEN_LIMIT);
    vitte_lexer_destroy(&lexer);

    assert(vitte_lexer_init(
        &lexer,
        nested_comment,
        sizeof(nested_comment) - 1u,
        UINT32_C(203)));
    assert(vitte_lexer_set_limits(&lexer, 4096u, 4096u, 1u));
    assert(!vitte_lexer_run(&lexer));
    assert(vitte_lexer_last_error(&lexer) ==
           VITTE_LEXER_ERROR_COMMENT_DEPTH);
    vitte_lexer_destroy(&lexer);

    assert(vitte_lexer_init(
        &lexer,
        invalid_utf8,
        sizeof(invalid_utf8) - 1u,
        UINT32_C(204)));
    assert(!vitte_lexer_run(&lexer));
    assert(vitte_lexer_last_error(&lexer) == VITTE_LEXER_ERROR_INVALID_UTF8);
    vitte_lexer_destroy(&lexer);
}

static void
test_parser_limits(void)
{
    static const char source[] = "proc main() { (((((0))))); }";
    vitte_lexer_t lexer;
    vitte_parser_t parser;

    assert(vitte_lexer_init(&lexer, source, strlen(source), UINT32_C(205)));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_parser_init(&parser, lexer.tokens, lexer.token_count));
    assert(vitte_parser_set_limits(&parser, 4096u, 128u, 2u, 64u));
    assert(!vitte_parser_run(&parser));
    assert(vitte_parser_last_error(&parser) ==
           VITTE_PARSER_ERROR_RECURSION_LIMIT);
    vitte_parser_destroy(&parser);
    vitte_lexer_destroy(&lexer);
}

static void
test_import_limits(void)
{
    vitte_import_context_t context;
    vitte_import_string_id_t first_name;
    vitte_import_string_id_t second_name;
    vitte_import_module_desc_t first;
    vitte_import_module_desc_t second;

    assert(vitte_import_init(&context));
    assert(vitte_import_set_limits(&context, 1u, 1u, 64u));

    first_name = vitte_import_intern_string(&context, "root", 4u);
    assert(first_name != VITTE_IMPORT_INVALID_ID);
    first = vitte_import_module_desc_empty();
    first.name = first_name;
    first.is_root = true;
    assert(vitte_import_add_module(&context, &first) !=
           VITTE_IMPORT_INVALID_ID);

    second = vitte_import_module_desc_empty();
    second_name = vitte_import_intern_string(&context, "other", 5u);
    assert(second_name != VITTE_IMPORT_INVALID_ID);
    second.name = second_name;
    assert(vitte_import_add_module(&context, &second) ==
           VITTE_IMPORT_INVALID_ID);
    assert(vitte_import_last_error(&context) ==
           VITTE_IMPORT_ERROR_TOO_MANY_MODULES);
    assert(vitte_import_is_valid(&context));
    vitte_import_destroy(&context);
}

int
main(void)
{
    test_arena_failures();
    test_lexer_limits();
    test_parser_limits();
    test_import_limits();
    return 0;
}
