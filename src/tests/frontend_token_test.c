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
    assert(vitte_lexer_last_error(&lexer) == VITTE_LEXER_ERROR_INVALID_TOKEN);
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
test_keywords(void)
{
    static const char source[] =
        "space use pub const static type opaque form pick trait impl "
        "where proc extern macro comptime test let mut set return defer "
        "requires ensures export if else elif while loop for in break continue "
        "match as async await unsafe asm move ref self and or not true false "
        "null assert print";
    static const vitte_token_kind_t kinds[] = {
        VITTE_TOKEN_KW_SPACE, VITTE_TOKEN_KW_USE, VITTE_TOKEN_KW_PUB,
        VITTE_TOKEN_KW_CONST, VITTE_TOKEN_KW_STATIC, VITTE_TOKEN_KW_TYPE,
        VITTE_TOKEN_KW_OPAQUE, VITTE_TOKEN_KW_FORM, VITTE_TOKEN_KW_PICK,
        VITTE_TOKEN_KW_TRAIT, VITTE_TOKEN_KW_IMPL,
        VITTE_TOKEN_KW_WHERE, VITTE_TOKEN_KW_PROC, VITTE_TOKEN_KW_EXTERN,
        VITTE_TOKEN_KW_MACRO, VITTE_TOKEN_KW_COMPTIME, VITTE_TOKEN_KW_TEST,
        VITTE_TOKEN_KW_LET, VITTE_TOKEN_KW_MUT, VITTE_TOKEN_KW_SET,
        VITTE_TOKEN_KW_RETURN, VITTE_TOKEN_KW_DEFER,
        VITTE_TOKEN_KW_REQUIRES, VITTE_TOKEN_KW_ENSURES,
        VITTE_TOKEN_KW_EXPORT, VITTE_TOKEN_KW_IF, VITTE_TOKEN_KW_ELSE,
        VITTE_TOKEN_KW_ELIF, VITTE_TOKEN_KW_WHILE, VITTE_TOKEN_KW_LOOP,
        VITTE_TOKEN_KW_FOR, VITTE_TOKEN_KW_IN, VITTE_TOKEN_KW_BREAK,
        VITTE_TOKEN_KW_CONTINUE, VITTE_TOKEN_KW_MATCH, VITTE_TOKEN_KW_AS,
        VITTE_TOKEN_KW_ASYNC, VITTE_TOKEN_KW_AWAIT, VITTE_TOKEN_KW_UNSAFE,
        VITTE_TOKEN_KW_ASM, VITTE_TOKEN_KW_MOVE, VITTE_TOKEN_KW_REF,
        VITTE_TOKEN_KW_SELF, VITTE_TOKEN_KW_AND, VITTE_TOKEN_KW_OR,
        VITTE_TOKEN_KW_NOT, VITTE_TOKEN_KW_TRUE, VITTE_TOKEN_KW_FALSE,
        VITTE_TOKEN_KW_NULL, VITTE_TOKEN_KW_ASSERT,
        VITTE_TOKEN_KW_PRINT, VITTE_TOKEN_EOF
    };

    expect_tokens(source, kinds, sizeof(kinds) / sizeof(kinds[0]));
}

static void
test_maximal_munch_operators(void)
{
    static const char source[] =
        "( ) { } [ ] , ; : :: . .. ..= -> => + - * / % & | ^ ~ ! = == != "
        "< <= > >= << >> += -= *= /= %= &= |= ^= <<= >>= && || ? ??";
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
        VITTE_TOKEN_QUESTION_QUESTION, VITTE_TOKEN_EOF
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
    test_keywords();
    test_maximal_munch_operators();
    (void)puts("frontend_token_test: OK");
    return 0;
}
