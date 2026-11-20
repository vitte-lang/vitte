#include "lexer/lexer.h"
#include "backend/c17/ast_emitter.h"
#include "parser/parser.h"
#include "unicode/unicode.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void
expect_tokens(
    const char *source,
    const vitte_token_kind_t *kinds,
    size_t kind_count)
{
    vitte_lexer_t lexer;
    size_t index;

    assert(vitte_lexer_init(&lexer, source, strlen(source), UINT32_C(7)));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_lexer_validate(&lexer));
    if (vitte_lexer_token_count(&lexer) != kind_count) {
        (void)fprintf(
            stderr,
            "token count mismatch for '%s' (expected %zu, got %zu)\n",
            source,
            kind_count,
            vitte_lexer_token_count(&lexer));
    }
    assert(vitte_lexer_token_count(&lexer) == kind_count);

    for (index = 0u; index < kind_count; ++index) {
        const vitte_token_t *token = vitte_lexer_token_at(&lexer, index);

        assert(token != NULL);
        assert(token->kind == kinds[index]);
        assert(token->span.file_id == UINT32_C(7));
        assert(token->span.end - token->span.begin == token->length);
        assert(token->line > 0u);
        assert(token->column > 0u);
    }

    assert(vitte_lexer_token_at(&lexer, kind_count - 1u)->kind ==
           VITTE_TOKEN_EOF);
    assert(vitte_lexer_fingerprint(&lexer) != UINT64_C(0));
    vitte_lexer_destroy(&lexer);
}

static uint64_t
lex_fingerprint(const char *source, uint32_t file_id)
{
    vitte_lexer_t lexer;
    uint64_t fingerprint;

    assert(vitte_lexer_init(&lexer, source, strlen(source), file_id));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_lexer_validate(&lexer));
    fingerprint = vitte_lexer_fingerprint(&lexer);
    assert(fingerprint != UINT64_C(0));
    vitte_lexer_destroy(&lexer);
    return fingerprint;
}

static void
expect_lexer_error(
    const char *source,
    size_t source_length,
    vitte_lexer_error_t expected_error)
{
    vitte_lexer_t lexer;
    vitte_lexer_span_t span;
    vitte_lexer_stats_t stats;

    assert(vitte_lexer_init(&lexer, source, source_length, UINT32_C(31)));
    assert(!vitte_lexer_run(&lexer));
    assert(vitte_lexer_state(&lexer) == VITTE_LEXER_STATE_FAILED);
    if (vitte_lexer_last_error(&lexer) != expected_error) {
        (void)fprintf(
            stderr,
            "lexer error mismatch (expected %s, got %s)\n",
            vitte_lexer_error_name(expected_error),
            vitte_lexer_error_name(vitte_lexer_last_error(&lexer)));
    }
    assert(vitte_lexer_last_error(&lexer) == expected_error);
    span = vitte_lexer_error_span(&lexer);
    assert(span.valid);
    assert(span.file_id == UINT32_C(31));
    assert(span.begin <= span.end && span.end <= source_length);
    stats = vitte_lexer_stats(&lexer);
    assert(stats.failed_runs == UINT64_C(1));
    assert(stats.errors != UINT64_C(0));
    vitte_lexer_destroy(&lexer);
}

static void
test_separate_object_emission(void)
{
    static const char source[] = "proc main() -> int { return 0; }";
    static const char object_path[] = "/tmp/vitte-frontend-object-test.o";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    vitte_c17_compile_options_t options;
    FILE *object;

    assert(vitte_lexer_init(&lexer, source, strlen(source), 23u));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_parser_init(&parser, lexer.tokens, lexer.token_count));
    assert(vitte_parser_run(&parser));
    memset(&options, 0, sizeof(options));
    options.source_path = "object-test.vit";
    assert(vitte_c17_compile_ast_object_with_options(
        &parser,
        object_path,
        &options));
    object = fopen(object_path, "rb");
    assert(object != NULL);
    assert(fseek(object, 0L, SEEK_END) == 0);
    assert(ftell(object) > 0L);
    assert(fclose(object) == 0);
    assert(remove(object_path) == 0);
    vitte_parser_destroy(&parser);
    vitte_lexer_destroy(&lexer);
}

static void
test_pinned_unicode_identifiers(void)
{
    static const char valid_source[] = "\xCE\xBB\xC3\xB8\xCC\x81";
    static const char composed[] = "caf\xC3\xA9";
    static const char decomposed[] = "cafe\xCC\x81";
    static const char invalid_start[] = "\xCC\x81" "a";
    static const char invalid_identifier[] = "\xF0\x9F\x98\x80";
    static const char invalid_utf8[] = "\xC0\xAF";
    vitte_lexer_t lexer;
    size_t error_offset;

    assert(strcmp(vitte_unicode_data_version(), "16.0.0") == 0);
    assert(vitte_unicode_is_identifier_start(0x03BBu));
    assert(vitte_unicode_is_identifier_continue(0x0301u));
    assert(!vitte_unicode_is_identifier_start(0x1F600u));
    assert(vitte_unicode_validate_identifier(
        valid_source, strlen(valid_source), &error_offset));
    assert(vitte_unicode_validate_identifier(
        composed, strlen(composed), &error_offset));
    assert(vitte_unicode_validate_identifier(
        decomposed, strlen(decomposed), &error_offset));
    assert(vitte_unicode_hash_utf8(
        composed, strlen(composed)) !=
           vitte_unicode_hash_utf8(
               decomposed, strlen(decomposed)));
    assert(!vitte_unicode_validate_identifier(
        invalid_start, strlen(invalid_start), &error_offset));
    assert(vitte_unicode_validate_utf8(valid_source, strlen(valid_source),
                                       &error_offset));
    assert(error_offset == strlen(valid_source));
    assert(!vitte_unicode_validate_utf8(invalid_utf8, strlen(invalid_utf8),
                                        &error_offset));
    assert(error_offset == 0u);

    assert(vitte_lexer_init(&lexer, valid_source, strlen(valid_source), 19u));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_lexer_token_at(&lexer, 0u)->kind == VITTE_TOKEN_IDENTIFIER);
    vitte_lexer_destroy(&lexer);

    assert(vitte_lexer_init(&lexer, invalid_identifier,
                            strlen(invalid_identifier), 19u));
    assert(!vitte_lexer_run(&lexer));
    assert(vitte_lexer_last_error(&lexer) == VITTE_LEXER_ERROR_INVALID_IDENTIFIER);
    assert(vitte_lexer_error_span(&lexer).begin == 0u);
    assert(vitte_lexer_error_span(&lexer).end == strlen(invalid_identifier));
    vitte_lexer_destroy(&lexer);
}

static void
test_spans_positions_eof_and_fingerprint(void)
{
    static const char source[] = " \t\xCE\xB1\n  beta";
    vitte_lexer_t lexer;
    const vitte_token_t *alpha;
    const vitte_token_t *beta;
    const vitte_token_t *eof;
    uint64_t first_fingerprint;

    assert(vitte_lexer_init(&lexer, source, strlen(source), UINT32_C(11)));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_lexer_validate(&lexer));
    assert(vitte_lexer_token_count(&lexer) == 3u);

    alpha = vitte_lexer_token_at(&lexer, 0u);
    beta = vitte_lexer_token_at(&lexer, 1u);
    eof = vitte_lexer_token_at(&lexer, 2u);
    assert(alpha != NULL && beta != NULL && eof != NULL);

    assert(alpha->kind == VITTE_TOKEN_IDENTIFIER);
    assert(alpha->span.begin == 2u && alpha->span.end == 4u);
    assert(alpha->line == 1u && alpha->column == 6u);
    assert(alpha->length == 2u);
    assert(vitte_token_text_equal(alpha, "\xCE\xB1", 2u));

    assert(beta->kind == VITTE_TOKEN_IDENTIFIER);
    assert(beta->span.begin == 7u && beta->span.end == 11u);
    assert(beta->line == 2u && beta->column == 3u);
    assert(beta->length == 4u);

    assert(eof->kind == VITTE_TOKEN_EOF);
    assert(eof->span.begin == strlen(source));
    assert(eof->span.end == strlen(source));
    assert(eof->length == 0u);
    assert(eof->line == 2u && eof->column == 7u);
    assert(eof->lexeme == source + strlen(source));

    first_fingerprint = vitte_lexer_fingerprint(&lexer);
    assert(first_fingerprint != UINT64_C(0));
    assert(first_fingerprint == vitte_lexer_fingerprint(&lexer));
    vitte_lexer_destroy(&lexer);

    assert(first_fingerprint == lex_fingerprint(source, UINT32_C(11)));
    assert(first_fingerprint != lex_fingerprint(" \t\xCE\xB1\n  gamma", UINT32_C(11)));
    assert(first_fingerprint != lex_fingerprint(source, UINT32_C(12)));
}

static void
test_literals_comments_and_unicode(void)
{
    static const char source[] =
        "// ignored line comment\n"
        "/* outer /* nested */ comment */\n"
        "alpha_2 0 42 0xff 1.5 \"h\xC3\xA9\\n\" 'x'\n";
    static const vitte_token_kind_t kinds[] = {
        VITTE_TOKEN_IDENTIFIER,
        VITTE_TOKEN_INTEGER_LITERAL,
        VITTE_TOKEN_INTEGER_LITERAL,
        VITTE_TOKEN_INTEGER_LITERAL,
        VITTE_TOKEN_FLOAT_LITERAL,
        VITTE_TOKEN_STRING_LITERAL,
        VITTE_TOKEN_CHARACTER_LITERAL,
        VITTE_TOKEN_EOF
    };

    expect_tokens(source, kinds, sizeof(kinds) / sizeof(kinds[0]));
}

static void
test_numeric_literal_families(void)
{
    static const char source[] =
        "0 42 0b101 0B10 0o7 0O10 0xff 0Xab 1_000 "
        "1.5 1e2 1E+2 0x1p2 0X1.Ap-2";
    static const vitte_token_kind_t kinds[] = {
        VITTE_TOKEN_INTEGER_LITERAL, VITTE_TOKEN_INTEGER_LITERAL,
        VITTE_TOKEN_INTEGER_LITERAL, VITTE_TOKEN_INTEGER_LITERAL,
        VITTE_TOKEN_INTEGER_LITERAL, VITTE_TOKEN_INTEGER_LITERAL,
        VITTE_TOKEN_INTEGER_LITERAL, VITTE_TOKEN_INTEGER_LITERAL,
        VITTE_TOKEN_INTEGER_LITERAL, VITTE_TOKEN_FLOAT_LITERAL,
        VITTE_TOKEN_FLOAT_LITERAL, VITTE_TOKEN_FLOAT_LITERAL,
        VITTE_TOKEN_FLOAT_LITERAL, VITTE_TOKEN_FLOAT_LITERAL,
        VITTE_TOKEN_EOF
    };

    expect_tokens(source, kinds, sizeof(kinds) / sizeof(kinds[0]));
}

static void
test_bom_whitespace_and_crlf_positions(void)
{
    static const char source[] =
        "\xEF\xBB\xBF\f\v\talpha\r\nbeta";
    vitte_lexer_t lexer;
    const vitte_token_t *alpha;
    const vitte_token_t *beta;
    const vitte_token_t *eof;

    assert(vitte_lexer_init(&lexer, source, sizeof(source) - 1u, 32u));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_lexer_validate(&lexer));
    assert(vitte_lexer_token_count(&lexer) == 3u);
    alpha = vitte_lexer_token_at(&lexer, 0u);
    beta = vitte_lexer_token_at(&lexer, 1u);
    eof = vitte_lexer_token_at(&lexer, 2u);
    assert(alpha != NULL && beta != NULL && eof != NULL);
    assert(alpha->kind == VITTE_TOKEN_IDENTIFIER);
    assert(alpha->span.begin == 6u && alpha->span.end == 11u);
    assert(alpha->line == 1u && alpha->column == 7u);
    assert(beta->kind == VITTE_TOKEN_IDENTIFIER);
    assert(beta->span.begin == 13u && beta->span.end == 17u);
    assert(beta->line == 2u && beta->column == 1u);
    assert(eof->line == 2u && eof->column == 5u);
    vitte_lexer_destroy(&lexer);
}

static void
test_lexer_error_matrix(void)
{
    static const char invalid_utf8[] = { (char)0xC0, (char)0xAF };
    static const char invalid_escape[] = "\"bad\\q\"";
    static const char unterminated_string[] = "\"unfinished";
    static const char invalid_character_empty[] = "''";
    static const char invalid_character_many[] = "'ab'";
    static const char unterminated_comment[] = "/* unfinished /* nested */";
    static const char invalid_binary[] = "0b102";
    static const char invalid_separators[] = "1__2";
    static const char invalid_hex_float[] = "0x1.0";
    static const char integer_overflow[] = "18446744073709551616";
    static const char invalid_scalar[] = "\xF0\x9F\x98\x80";
    static const char invalid_nul[] = { '\0' };

    expect_lexer_error(invalid_utf8, sizeof(invalid_utf8),
                       VITTE_LEXER_ERROR_INVALID_UTF8);
    expect_lexer_error(invalid_escape, sizeof(invalid_escape) - 1u,
                       VITTE_LEXER_ERROR_INVALID_ESCAPE);
    expect_lexer_error(unterminated_string,
                       sizeof(unterminated_string) - 1u,
                       VITTE_LEXER_ERROR_UNTERMINATED_STRING);
    expect_lexer_error(invalid_character_empty,
                       sizeof(invalid_character_empty) - 1u,
                       VITTE_LEXER_ERROR_INVALID_CHARACTER);
    expect_lexer_error(invalid_character_many,
                       sizeof(invalid_character_many) - 1u,
                       VITTE_LEXER_ERROR_INVALID_CHARACTER);
    expect_lexer_error(unterminated_comment,
                       sizeof(unterminated_comment) - 1u,
                       VITTE_LEXER_ERROR_UNTERMINATED_COMMENT);
    expect_lexer_error(invalid_binary, sizeof(invalid_binary) - 1u,
                       VITTE_LEXER_ERROR_INVALID_NUMBER);
    expect_lexer_error(invalid_separators,
                       sizeof(invalid_separators) - 1u,
                       VITTE_LEXER_ERROR_INVALID_NUMBER);
    expect_lexer_error(invalid_hex_float,
                       sizeof(invalid_hex_float) - 1u,
                       VITTE_LEXER_ERROR_INVALID_NUMBER);
    expect_lexer_error(integer_overflow, sizeof(integer_overflow) - 1u,
                       VITTE_LEXER_ERROR_NUMBER_OVERFLOW);
    expect_lexer_error(invalid_scalar, sizeof(invalid_scalar) - 1u,
                       VITTE_LEXER_ERROR_INVALID_IDENTIFIER);
    expect_lexer_error(invalid_nul, sizeof(invalid_nul),
                       VITTE_LEXER_ERROR_INVALID_TOKEN);
}

static void
test_limits_reset_and_empty_source(void)
{
    static const char one_token[] = "name";
    static const char two_tokens[] = "one two";
    static const char nested_comment[] = "/* /* nested */ */";
    static const char oversized[] = "abc";
    vitte_lexer_t lexer;
    uint64_t generation;

    assert(vitte_lexer_init(&lexer, "", 0u, 33u));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_lexer_token_count(&lexer) == 1u);
    assert(vitte_lexer_token_at(&lexer, 0u)->kind == VITTE_TOKEN_EOF);
    vitte_lexer_destroy(&lexer);

    assert(vitte_lexer_init(&lexer, two_tokens, sizeof(two_tokens) - 1u, 34u));
    assert(vitte_lexer_set_limits(
        &lexer, 1u, sizeof(two_tokens) - 1u, 4u));
    assert(!vitte_lexer_run(&lexer));
    assert(vitte_lexer_last_error(&lexer) == VITTE_LEXER_ERROR_TOKEN_LIMIT);
    assert(vitte_lexer_error_span(&lexer).begin == 4u);
    generation = vitte_lexer_generation(&lexer);
    assert(vitte_lexer_reset(&lexer, one_token, sizeof(one_token) - 1u, 35u));
    assert(vitte_lexer_generation(&lexer) == generation + UINT64_C(1));
    assert(vitte_lexer_set_limits(&lexer, 2u, sizeof(two_tokens) - 1u, 4u));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_lexer_validate(&lexer));
    assert(vitte_lexer_token_count(&lexer) == 2u);
    vitte_lexer_destroy(&lexer);

    assert(vitte_lexer_init(
        &lexer, nested_comment, sizeof(nested_comment) - 1u, 36u));
    assert(vitte_lexer_set_limits(
        &lexer, 32u, sizeof(nested_comment) - 1u, 1u));
    assert(!vitte_lexer_run(&lexer));
    assert(vitte_lexer_last_error(&lexer) == VITTE_LEXER_ERROR_COMMENT_DEPTH);
    vitte_lexer_destroy(&lexer);

    assert(vitte_lexer_init(&lexer, "a", 1u, 37u));
    assert(vitte_lexer_set_limits(&lexer, 8u, 1u, 4u));
    assert(!vitte_lexer_reset(&lexer, oversized, sizeof(oversized) - 1u, 38u));
    assert(vitte_lexer_last_error(&lexer) == VITTE_LEXER_ERROR_SOURCE_TOO_LARGE);
    assert(vitte_lexer_state(&lexer) == VITTE_LEXER_STATE_FAILED);
    vitte_lexer_destroy(&lexer);
}

static void
test_keywords(void)
{
    typedef struct keyword_case {
        const char *text;
        vitte_token_kind_t kind;
    } keyword_case_t;
    static const keyword_case_t keywords[] = {
        { "space", VITTE_TOKEN_KW_SPACE }, { "use", VITTE_TOKEN_KW_USE },
        { "pub", VITTE_TOKEN_KW_PUB }, { "const", VITTE_TOKEN_KW_CONST },
        { "static", VITTE_TOKEN_KW_STATIC }, { "type", VITTE_TOKEN_KW_TYPE },
        { "opaque", VITTE_TOKEN_KW_OPAQUE }, { "form", VITTE_TOKEN_KW_FORM },
        { "pick", VITTE_TOKEN_KW_PICK }, { "trait", VITTE_TOKEN_KW_TRAIT },
        { "impl", VITTE_TOKEN_KW_IMPL }, { "where", VITTE_TOKEN_KW_WHERE },
        { "proc", VITTE_TOKEN_KW_PROC }, { "extern", VITTE_TOKEN_KW_EXTERN },
        { "intrinsic", VITTE_TOKEN_KW_EXTERN }, { "macro", VITTE_TOKEN_KW_MACRO },
        { "comptime", VITTE_TOKEN_KW_COMPTIME }, { "test", VITTE_TOKEN_KW_TEST },
        { "let", VITTE_TOKEN_KW_LET }, { "mut", VITTE_TOKEN_KW_MUT },
        { "set", VITTE_TOKEN_KW_SET }, { "return", VITTE_TOKEN_KW_RETURN },
        { "defer", VITTE_TOKEN_KW_DEFER }, { "requires", VITTE_TOKEN_KW_REQUIRES },
        { "ensures", VITTE_TOKEN_KW_ENSURES }, { "export", VITTE_TOKEN_KW_EXPORT },
        { "if", VITTE_TOKEN_KW_IF }, { "else", VITTE_TOKEN_KW_ELSE },
        { "elif", VITTE_TOKEN_KW_ELIF }, { "while", VITTE_TOKEN_KW_WHILE },
        { "loop", VITTE_TOKEN_KW_LOOP }, { "for", VITTE_TOKEN_KW_FOR },
        { "in", VITTE_TOKEN_KW_IN }, { "break", VITTE_TOKEN_KW_BREAK },
        { "continue", VITTE_TOKEN_KW_CONTINUE }, { "match", VITTE_TOKEN_KW_MATCH },
        { "as", VITTE_TOKEN_KW_AS }, { "async", VITTE_TOKEN_KW_ASYNC },
        { "await", VITTE_TOKEN_KW_AWAIT }, { "unsafe", VITTE_TOKEN_KW_UNSAFE },
        { "asm", VITTE_TOKEN_KW_ASM }, { "move", VITTE_TOKEN_KW_MOVE },
        { "ref", VITTE_TOKEN_KW_REF }, { "self", VITTE_TOKEN_KW_SELF },
        { "and", VITTE_TOKEN_KW_AND }, { "or", VITTE_TOKEN_KW_OR },
        { "not", VITTE_TOKEN_KW_NOT }, { "true", VITTE_TOKEN_KW_TRUE },
        { "false", VITTE_TOKEN_KW_FALSE }, { "null", VITTE_TOKEN_KW_NULL },
        { "assert", VITTE_TOKEN_KW_ASSERT }, { "print", VITTE_TOKEN_KW_PRINT },
        { "sizeof", VITTE_TOKEN_KW_SIZEOF }, { "alignof", VITTE_TOKEN_KW_ALIGNOF },
        { "share", VITTE_TOKEN_KW_RESERVED }, { "region", VITTE_TOKEN_KW_RESERVED },
        { "compiler", VITTE_TOKEN_KW_RESERVED },
        { "diagnostic", VITTE_TOKEN_KW_RESERVED },
        { "bench", VITTE_TOKEN_KW_RESERVED }, { "entry", VITTE_TOKEN_KW_RESERVED },
        { "select", VITTE_TOKEN_KW_RESERVED }, { "when", VITTE_TOKEN_KW_RESERVED },
        { "case", VITTE_TOKEN_KW_RESERVED }, { "critical", VITTE_TOKEN_KW_RESERVED },
        { "owned", VITTE_TOKEN_KW_RESERVED }, { "borrow", VITTE_TOKEN_KW_RESERVED },
        { "volatile", VITTE_TOKEN_KW_RESERVED }, { "is", VITTE_TOKEN_KW_RESERVED },
        { "kernel", VITTE_TOKEN_KW_RESERVED }, { "class", VITTE_TOKEN_KW_RESERVED },
        { "flags", VITTE_TOKEN_KW_RESERVED }
    };
    char source[768];
    size_t source_length = 0u;
    size_t index;
    vitte_lexer_t lexer;

    for (index = 0u; index < sizeof(keywords) / sizeof(keywords[0]); ++index) {
        size_t length = strlen(keywords[index].text);
        assert(source_length + length + 1u < sizeof(source));
        memcpy(source + source_length, keywords[index].text, length);
        source_length += length;
        source[source_length++] = ' ';
    }
    assert(vitte_lexer_init(&lexer, source, source_length, UINT32_C(8)));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_lexer_validate(&lexer));
    assert(vitte_lexer_token_count(&lexer) ==
           sizeof(keywords) / sizeof(keywords[0]) + 1u);
    for (index = 0u; index < sizeof(keywords) / sizeof(keywords[0]); ++index) {
        const vitte_token_t *token = vitte_lexer_token_at(&lexer, index);
        assert(token != NULL);
        assert(token->kind == keywords[index].kind);
        assert(vitte_token_text_equal(
            token, keywords[index].text, strlen(keywords[index].text)));
    }
    assert(vitte_lexer_token_at(&lexer, index)->kind == VITTE_TOKEN_EOF);
    vitte_lexer_destroy(&lexer);
}

static void
test_keyword_prefixes_remain_identifiers(void)
{
    static const char source[] =
        "spacecraft intrinsic_value _return value2 compilerish flags_";
    static const vitte_token_kind_t kinds[] = {
        VITTE_TOKEN_IDENTIFIER, VITTE_TOKEN_IDENTIFIER,
        VITTE_TOKEN_IDENTIFIER, VITTE_TOKEN_IDENTIFIER,
        VITTE_TOKEN_IDENTIFIER, VITTE_TOKEN_IDENTIFIER,
        VITTE_TOKEN_EOF
    };

    expect_tokens(source, kinds, sizeof(kinds) / sizeof(kinds[0]));
}

static void
test_maximal_munch_operators(void)
{
    static const char source[] =
        "( ) { } [ ] , ; : :: . .. ..= -> => + - * / % & | ^ ~ ! = == != "
        "< <= > >= << >> += -= *= /= %= &= |= ^= <<= >>= && || ? ?? # @ $";
    static const vitte_token_kind_t kinds[] = {
        VITTE_TOKEN_LEFT_PAREN, VITTE_TOKEN_RIGHT_PAREN,
        VITTE_TOKEN_LEFT_BRACE, VITTE_TOKEN_RIGHT_BRACE,
        VITTE_TOKEN_LEFT_BRACKET, VITTE_TOKEN_RIGHT_BRACKET,
        VITTE_TOKEN_COMMA, VITTE_TOKEN_SEMICOLON, VITTE_TOKEN_COLON,
        VITTE_TOKEN_COLON_COLON, VITTE_TOKEN_DOT, VITTE_TOKEN_DOT_DOT,
        VITTE_TOKEN_DOT_DOT_EQUAL, VITTE_TOKEN_ARROW, VITTE_TOKEN_FAT_ARROW,
        VITTE_TOKEN_PLUS, VITTE_TOKEN_MINUS, VITTE_TOKEN_STAR,
        VITTE_TOKEN_SLASH, VITTE_TOKEN_PERCENT, VITTE_TOKEN_AMP,
        VITTE_TOKEN_PIPE, VITTE_TOKEN_CARET, VITTE_TOKEN_TILDE,
        VITTE_TOKEN_BANG, VITTE_TOKEN_EQUAL, VITTE_TOKEN_EQUAL_EQUAL,
        VITTE_TOKEN_BANG_EQUAL, VITTE_TOKEN_LESS, VITTE_TOKEN_LESS_EQUAL,
        VITTE_TOKEN_GREATER, VITTE_TOKEN_GREATER_EQUAL,
        VITTE_TOKEN_SHIFT_LEFT, VITTE_TOKEN_SHIFT_RIGHT,
        VITTE_TOKEN_PLUS_EQUAL, VITTE_TOKEN_MINUS_EQUAL,
        VITTE_TOKEN_STAR_EQUAL, VITTE_TOKEN_SLASH_EQUAL,
        VITTE_TOKEN_PERCENT_EQUAL, VITTE_TOKEN_AMP_EQUAL,
        VITTE_TOKEN_PIPE_EQUAL, VITTE_TOKEN_CARET_EQUAL,
        VITTE_TOKEN_SHIFT_LEFT_EQUAL, VITTE_TOKEN_SHIFT_RIGHT_EQUAL,
        VITTE_TOKEN_AMP_AMP, VITTE_TOKEN_PIPE_PIPE, VITTE_TOKEN_QUESTION,
        VITTE_TOKEN_QUESTION_QUESTION, VITTE_TOKEN_HASH,
        VITTE_TOKEN_AT, VITTE_TOKEN_DOLLAR,
        VITTE_TOKEN_EOF
    };

    expect_tokens(source, kinds, sizeof(kinds) / sizeof(kinds[0]));
}

int
main(void)
{
    test_separate_object_emission();
    test_pinned_unicode_identifiers();
    test_spans_positions_eof_and_fingerprint();
    test_literals_comments_and_unicode();
    test_numeric_literal_families();
    test_bom_whitespace_and_crlf_positions();
    test_lexer_error_matrix();
    test_limits_reset_and_empty_source();
    test_keywords();
    test_keyword_prefixes_remain_identifiers();
    test_maximal_munch_operators();
    (void)puts("frontend_token_test: OK");
    return 0;
}
