#ifndef VITTE_LEXER_LEXER_H
#define VITTE_LEXER_LEXER_H

/*
 * Vitte Compiler
 * src/lexer/lexer.h
 *
 * Canonical public lexer contract.
 *
 * Properties:
 *   - ISO C17;
 *   - deterministic tokenization;
 *   - byte-accurate source spans;
 *   - line/column tracking;
 *   - UTF-8 source support;
 *   - Vitte canonical keywords;
 *   - integer/float/string/character literals;
 *   - nested block comments;
 *   - maximal-munch operators;
 *   - resource limits;
 *   - structural validation;
 *   - deterministic fingerprints;
 *   - statistics;
 *   - no filesystem dependency;
 *   - no parser dependency.
 *
 * Source ownership
 * ----------------
 *
 * The lexer does NOT own source.
 *
 * The source buffer supplied to vitte_lexer_init() or
 * vitte_lexer_reset() must remain valid and unchanged while tokens
 * are being used because token.lexeme points directly into it.
 *
 * Token ownership
 * ---------------
 *
 * Token storage is owned by vitte_lexer_t and released by reset()
 * or destroy().
 *
 * Coordinates
 * -----------
 *
 * Source spans use half-open byte ranges:
 *
 *     [begin, end)
 *
 * line/column values are 1-based.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

#define VITTE_LEXER_API_VERSION_MAJOR 1u
#define VITTE_LEXER_API_VERSION_MINOR 0u
#define VITTE_LEXER_API_VERSION_PATCH 0u

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_LEXER_MAGIC \
    UINT64_C(0x564954544c455821)

#define VITTE_LEXER_DEAD_MAGIC \
    UINT64_C(0x444541444c455821)

#define VITTE_LEXER_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_LEXER_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_LEXER_DEFAULT_INITIAL_CAPACITY \
    ((size_t)128u)

#define VITTE_LEXER_DEFAULT_MAX_TOKENS \
    ((size_t)16777216u)

#define VITTE_LEXER_DEFAULT_MAX_SOURCE_BYTES \
    ((size_t)(1024u * 1024u * 1024u))

#define VITTE_LEXER_DEFAULT_MAX_COMMENT_DEPTH \
    ((size_t)4096u)

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_lexer_error {
    VITTE_LEXER_ERROR_NONE = 0,

    VITTE_LEXER_ERROR_INVALID_ARGUMENT,
    VITTE_LEXER_ERROR_INVALID_CONTEXT,
    VITTE_LEXER_ERROR_INVALID_STATE,

    VITTE_LEXER_ERROR_INVALID_UTF8,
    VITTE_LEXER_ERROR_INVALID_TOKEN,
    VITTE_LEXER_ERROR_INVALID_NUMBER,
    VITTE_LEXER_ERROR_INVALID_ESCAPE,
    VITTE_LEXER_ERROR_INVALID_CHARACTER,

    VITTE_LEXER_ERROR_UNTERMINATED_STRING,
    VITTE_LEXER_ERROR_UNTERMINATED_COMMENT,

    VITTE_LEXER_ERROR_COMMENT_DEPTH,

    VITTE_LEXER_ERROR_TOKEN_LIMIT,
    VITTE_LEXER_ERROR_SOURCE_TOO_LARGE,

    VITTE_LEXER_ERROR_OVERFLOW,
    VITTE_LEXER_ERROR_OUT_OF_MEMORY,

    VITTE_LEXER_ERROR_VALIDATION,
    VITTE_LEXER_ERROR_INTERNAL,

    VITTE_LEXER_ERROR_COUNT
} vitte_lexer_error_t;

/* ========================================================================= */
/* Lexer state                                                               */
/* ========================================================================= */

typedef enum vitte_lexer_state {
    VITTE_LEXER_STATE_INVALID = 0,

    /*
     * Initialized and ready for vitte_lexer_run().
     */
    VITTE_LEXER_STATE_READY,

    /*
     * Internal transient state while tokenization is running.
     */
    VITTE_LEXER_STATE_LEXING,

    /*
     * Complete token stream ending in EOF is available.
     */
    VITTE_LEXER_STATE_DONE,

    /*
     * Tokenization failed.
     */
    VITTE_LEXER_STATE_FAILED,

    /*
     * destroy() was called.
     */
    VITTE_LEXER_STATE_DESTROYED,

    VITTE_LEXER_STATE_COUNT
} vitte_lexer_state_t;

/* ========================================================================= */
/* Token kinds                                                               */
/* ========================================================================= */

typedef enum vitte_token_kind {
    VITTE_TOKEN_INVALID = 0,

    VITTE_TOKEN_EOF,

    /* --------------------------------------------------------------------- */
    /* Identifiers / literals                                                */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_IDENTIFIER,

    VITTE_TOKEN_INTEGER_LITERAL,
    VITTE_TOKEN_FLOAT_LITERAL,
    VITTE_TOKEN_STRING_LITERAL,
    VITTE_TOKEN_CHARACTER_LITERAL,

    /* --------------------------------------------------------------------- */
    /* Canonical Vitte keywords                                              */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_KW_SPACE,
    VITTE_TOKEN_KW_USE,

    VITTE_TOKEN_KW_PUB,

    VITTE_TOKEN_KW_CONST,
    VITTE_TOKEN_KW_STATIC,

    VITTE_TOKEN_KW_TYPE,
    VITTE_TOKEN_KW_OPAQUE,
    VITTE_TOKEN_KW_FORM,
    VITTE_TOKEN_KW_PICK,

    VITTE_TOKEN_KW_TRAIT,
    VITTE_TOKEN_KW_IMPL,
    VITTE_TOKEN_KW_DYN,
    VITTE_TOKEN_KW_WHERE,

    VITTE_TOKEN_KW_PROC,
    VITTE_TOKEN_KW_EXTERN,

    VITTE_TOKEN_KW_MACRO,
    VITTE_TOKEN_KW_COMPTIME,

    VITTE_TOKEN_KW_TEST,

    VITTE_TOKEN_KW_LET,
    VITTE_TOKEN_KW_MUT,
    VITTE_TOKEN_KW_SET,

    VITTE_TOKEN_KW_RETURN,
    VITTE_TOKEN_KW_GIVE,
    VITTE_TOKEN_KW_DEFER,
    VITTE_TOKEN_KW_REQUIRES,
    VITTE_TOKEN_KW_ENSURES,
    VITTE_TOKEN_KW_EXPORT,

    VITTE_TOKEN_KW_IF,
    VITTE_TOKEN_KW_ELSE,
    VITTE_TOKEN_KW_ELIF,

    VITTE_TOKEN_KW_WHILE,
    VITTE_TOKEN_KW_LOOP,
    VITTE_TOKEN_KW_FOR,
    VITTE_TOKEN_KW_IN,

    VITTE_TOKEN_KW_BREAK,
    VITTE_TOKEN_KW_CONTINUE,

    VITTE_TOKEN_KW_MATCH,
    VITTE_TOKEN_KW_AS,

    VITTE_TOKEN_KW_ASYNC,
    VITTE_TOKEN_KW_AWAIT,

    VITTE_TOKEN_KW_UNSAFE,
    VITTE_TOKEN_KW_ASM,

    VITTE_TOKEN_KW_MOVE,
    VITTE_TOKEN_KW_REF,
    VITTE_TOKEN_KW_SELF,

    VITTE_TOKEN_KW_AND,
    VITTE_TOKEN_KW_OR,
    VITTE_TOKEN_KW_NOT,

    VITTE_TOKEN_KW_TRUE,
    VITTE_TOKEN_KW_FALSE,
    VITTE_TOKEN_KW_NULL,

    VITTE_TOKEN_KW_ASSERT,

    VITTE_TOKEN_KW_MAP,

    VITTE_TOKEN_KW_SIZEOF,
    VITTE_TOKEN_KW_ALIGNOF,
    VITTE_TOKEN_KW_OFFSETOF,
    VITTE_TOKEN_KW_TYPEOF,

    /* --------------------------------------------------------------------- */
    /* Delimiters                                                            */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_LEFT_PAREN,
    VITTE_TOKEN_RIGHT_PAREN,

    VITTE_TOKEN_LEFT_BRACE,
    VITTE_TOKEN_RIGHT_BRACE,

    VITTE_TOKEN_LEFT_BRACKET,
    VITTE_TOKEN_RIGHT_BRACKET,

    VITTE_TOKEN_COMMA,
    VITTE_TOKEN_SEMICOLON,

    VITTE_TOKEN_COLON,
    VITTE_TOKEN_COLON_COLON,

    VITTE_TOKEN_DOT,
    VITTE_TOKEN_DOT_DOT,
    VITTE_TOKEN_DOT_DOT_EQUAL,

    /* --------------------------------------------------------------------- */
    /* Arrows                                                                */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_ARROW,
    VITTE_TOKEN_FAT_ARROW,

    /* --------------------------------------------------------------------- */
    /* Arithmetic                                                            */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_PLUS,
    VITTE_TOKEN_MINUS,
    VITTE_TOKEN_STAR,
    VITTE_TOKEN_SLASH,
    VITTE_TOKEN_PERCENT,

    /* --------------------------------------------------------------------- */
    /* Bitwise                                                               */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_AMP,
    VITTE_TOKEN_PIPE,
    VITTE_TOKEN_CARET,
    VITTE_TOKEN_TILDE,

    /* --------------------------------------------------------------------- */
    /* Logical / unary                                                       */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_BANG,

    /* --------------------------------------------------------------------- */
    /* Assignment / comparisons                                              */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_EQUAL,

    VITTE_TOKEN_EQUAL_EQUAL,
    VITTE_TOKEN_BANG_EQUAL,

    VITTE_TOKEN_LESS,
    VITTE_TOKEN_LESS_EQUAL,

    VITTE_TOKEN_GREATER,
    VITTE_TOKEN_GREATER_EQUAL,

    /* --------------------------------------------------------------------- */
    /* Shifts                                                                */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_SHIFT_LEFT,
    VITTE_TOKEN_SHIFT_RIGHT,

    /* --------------------------------------------------------------------- */
    /* Compound assignment                                                   */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_PLUS_EQUAL,
    VITTE_TOKEN_MINUS_EQUAL,
    VITTE_TOKEN_STAR_EQUAL,
    VITTE_TOKEN_SLASH_EQUAL,
    VITTE_TOKEN_PERCENT_EQUAL,

    VITTE_TOKEN_AMP_EQUAL,
    VITTE_TOKEN_PIPE_EQUAL,
    VITTE_TOKEN_CARET_EQUAL,

    VITTE_TOKEN_SHIFT_LEFT_EQUAL,
    VITTE_TOKEN_SHIFT_RIGHT_EQUAL,

    /* --------------------------------------------------------------------- */
    /* Symbolic logical operators                                            */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_AMP_AMP,
    VITTE_TOKEN_PIPE_PIPE,

    /* --------------------------------------------------------------------- */
    /* Optional / coalescing                                                 */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_QUESTION,
    VITTE_TOKEN_QUESTION_QUESTION,

    /* --------------------------------------------------------------------- */
    /* Miscellaneous punctuation                                             */
    /* --------------------------------------------------------------------- */

    VITTE_TOKEN_AT,
    VITTE_TOKEN_HASH,
    VITTE_TOKEN_DOLLAR,

    VITTE_TOKEN_COUNT
} vitte_token_kind_t;

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

typedef struct vitte_lexer_span {
    uint32_t file_id;

    size_t begin;
    size_t end;

    bool valid;
} vitte_lexer_span_t;

/* ========================================================================= */
/* Token                                                                     */
/* ========================================================================= */

typedef struct vitte_token {
    vitte_token_kind_t kind;

    vitte_lexer_span_t span;

    /*
     * 1-based location of the token's first byte.
     */
    size_t line;
    size_t column;

    /*
     * Borrowed pointer into vitte_lexer_t::source.
     *
     * Not necessarily NUL-terminated.
     */
    const char *lexeme;

    size_t length;
} vitte_token_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_lexer_stats {
    uint64_t runs;
    uint64_t failed_runs;

    uint64_t source_bytes;

    uint64_t tokens_emitted;

    uint64_t identifiers;
    uint64_t keywords;

    uint64_t integer_literals;
    uint64_t float_literals;
    uint64_t string_literals;
    uint64_t character_literals;

    uint64_t operators;

    uint64_t comments;
    uint64_t comment_bytes;
    uint64_t whitespace_bytes;

    uint64_t errors;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;
} vitte_lexer_stats_t;

/* ========================================================================= */
/* Lexer context                                                             */
/* ========================================================================= */

/*
 * API-v1 representation.
 *
 * The structure is intentionally visible so compiler stages may stack
 * allocate lexer contexts and inspect them during low-level debugging.
 *
 * Normal consumers should use the public functions below rather than
 * modifying fields directly.
 */
typedef struct vitte_lexer {
    uint64_t magic;

    vitte_lexer_state_t state;
    vitte_lexer_error_t last_error;

    /* --------------------------------------------------------------------- */
    /* Source                                                                */
    /* --------------------------------------------------------------------- */

    /*
     * Borrowed source buffer.
     */
    const char *source;

    size_t source_length;

    uint32_t file_id;

    /* --------------------------------------------------------------------- */
    /* Current cursor                                                        */
    /* --------------------------------------------------------------------- */

    size_t offset;

    /*
     * 1-based.
     */
    size_t line;
    size_t column;

    /* --------------------------------------------------------------------- */
    /* Tokens                                                                */
    /* --------------------------------------------------------------------- */

    vitte_token_t *tokens;

    size_t token_count;
    size_t token_capacity;

    /* --------------------------------------------------------------------- */
    /* Last error                                                            */
    /* --------------------------------------------------------------------- */

    vitte_lexer_span_t error_span;

    /* --------------------------------------------------------------------- */
    /* Resource limits                                                       */
    /* --------------------------------------------------------------------- */

    size_t max_tokens;
    size_t max_source_bytes;
    size_t max_comment_depth;

    /* --------------------------------------------------------------------- */
    /* Statistics                                                            */
    /* --------------------------------------------------------------------- */

    vitte_lexer_stats_t stats;

    /*
     * Increased by reset().
     *
     * Useful to detect references into an older token generation.
     */
    uint64_t generation;
} vitte_lexer_t;

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_token_kind_name(
    vitte_token_kind_t kind);

const char *
vitte_lexer_error_name(
    vitte_lexer_error_t error);

const char *
vitte_lexer_state_name(
    vitte_lexer_state_t state);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize a lexer over a borrowed source buffer.
 *
 * source may be NULL only when source_length == 0.
 */
bool
vitte_lexer_init(
    vitte_lexer_t *lexer,
    const char *source,
    size_t source_length,
    uint32_t file_id);

/*
 * Release token storage and invalidate the context.
 */
void
vitte_lexer_destroy(
    vitte_lexer_t *lexer);

/*
 * Reuse an initialized context with another source buffer.
 *
 * Existing token storage is discarded.
 * Configured limits are preserved.
 */
bool
vitte_lexer_reset(
    vitte_lexer_t *lexer,
    const char *source,
    size_t source_length,
    uint32_t file_id);

bool
vitte_lexer_is_valid(
    const vitte_lexer_t *lexer);

/* ========================================================================= */
/* Tokenization                                                              */
/* ========================================================================= */

/*
 * Tokenize the complete source.
 *
 * On success:
 *
 *     state == VITTE_LEXER_STATE_DONE
 *
 * and the token stream always ends with exactly one EOF token.
 *
 * On failure:
 *
 *     state == VITTE_LEXER_STATE_FAILED
 *
 * and last_error/error_span describe the lexical failure.
 */
bool
vitte_lexer_run(
    vitte_lexer_t *lexer);

/* ========================================================================= */
/* Token access                                                              */
/* ========================================================================= */

const vitte_token_t *
vitte_lexer_token_at(
    const vitte_lexer_t *lexer,
    size_t index);

size_t
vitte_lexer_token_count(
    const vitte_lexer_t *lexer);

/* ========================================================================= */
/* Token text                                                                */
/* ========================================================================= */

/*
 * Compare the exact token lexeme against an arbitrary byte string.
 */
bool
vitte_token_text_equal(
    const vitte_token_t *token,
    const char *text,
    size_t length);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate:
 *
 *   - context invariants;
 *   - token kinds;
 *   - token ordering;
 *   - source ranges;
 *   - token lengths;
 *   - token source pointers;
 *   - line/column presence;
 *   - EOF placement.
 */
bool
vitte_lexer_validate(
    vitte_lexer_t *lexer);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Deterministic FNV-1a-based fingerprint of the token stream.
 *
 * Includes:
 *
 *   - lexer API version;
 *   - file ID;
 *   - token count;
 *   - token kinds;
 *   - byte spans;
 *   - line/column locations;
 *   - exact lexeme bytes.
 *
 * This is intended for regression testing, cache bookkeeping and
 * reproducibility checks.
 *
 * It is not cryptographic.
 */
uint64_t
vitte_lexer_fingerprint(
    vitte_lexer_t *lexer);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_lexer_stats_t
vitte_lexer_stats(
    const vitte_lexer_t *lexer);

/* ========================================================================= */
/* Resource limits                                                           */
/* ========================================================================= */

/*
 * Limits may only be changed while the lexer is READY.
 */
bool
vitte_lexer_set_limits(
    vitte_lexer_t *lexer,
    size_t max_tokens,
    size_t max_source_bytes,
    size_t max_comment_depth);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_lexer_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_token_kind_is_valid(vitte_token_kind_t kind)
{
    return kind > VITTE_TOKEN_INVALID &&
           kind < VITTE_TOKEN_COUNT;
}

static inline bool
vitte_lexer_error_is_valid(vitte_lexer_error_t error)
{
    return error >= VITTE_LEXER_ERROR_NONE &&
           error < VITTE_LEXER_ERROR_COUNT;
}

static inline bool
vitte_lexer_state_is_valid(vitte_lexer_state_t state)
{
    return state > VITTE_LEXER_STATE_INVALID &&
           state < VITTE_LEXER_STATE_COUNT;
}

/* ========================================================================= */
/* Inline span helpers                                                       */
/* ========================================================================= */

static inline vitte_lexer_span_t
vitte_lexer_span_invalid(void)
{
    vitte_lexer_span_t span;

    span.file_id = 0u;
    span.begin = 0u;
    span.end = 0u;
    span.valid = false;

    return span;
}

static inline vitte_lexer_span_t
vitte_lexer_span_make(
    uint32_t file_id,
    size_t begin,
    size_t end)
{
    vitte_lexer_span_t span;

    span.file_id = file_id;
    span.begin = begin;
    span.end = end;
    span.valid = begin <= end;

    return span;
}

static inline size_t
vitte_lexer_span_length(
    const vitte_lexer_span_t *span)
{
    if (span == NULL ||
        !span->valid ||
        span->end < span->begin) {
        return 0u;
    }

    return span->end - span->begin;
}

static inline bool
vitte_lexer_span_contains(
    const vitte_lexer_span_t *span,
    size_t offset)
{
    return span != NULL &&
           span->valid &&
           span->begin <= offset &&
           offset < span->end;
}

/* ========================================================================= */
/* Inline context queries                                                    */
/* ========================================================================= */

static inline vitte_lexer_state_t
vitte_lexer_state(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return VITTE_LEXER_STATE_INVALID;
    }

    return lexer->state;
}

static inline vitte_lexer_error_t
vitte_lexer_last_error(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return VITTE_LEXER_ERROR_INVALID_CONTEXT;
    }

    return lexer->last_error;
}

static inline vitte_lexer_span_t
vitte_lexer_error_span(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return vitte_lexer_span_invalid();
    }

    return lexer->error_span;
}

static inline uint64_t
vitte_lexer_generation(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return UINT64_C(0);
    }

    return lexer->generation;
}

static inline bool
vitte_lexer_is_ready(
    const vitte_lexer_t *lexer)
{
    return lexer != NULL &&
           lexer->magic == VITTE_LEXER_MAGIC &&
           lexer->state == VITTE_LEXER_STATE_READY;
}

static inline bool
vitte_lexer_is_done(
    const vitte_lexer_t *lexer)
{
    return lexer != NULL &&
           lexer->magic == VITTE_LEXER_MAGIC &&
           lexer->state == VITTE_LEXER_STATE_DONE;
}

static inline bool
vitte_lexer_has_failed(
    const vitte_lexer_t *lexer)
{
    return lexer != NULL &&
           lexer->magic == VITTE_LEXER_MAGIC &&
           lexer->state == VITTE_LEXER_STATE_FAILED;
}

static inline size_t
vitte_lexer_source_length(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return 0u;
    }

    return lexer->source_length;
}

static inline uint32_t
vitte_lexer_file_id(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return 0u;
    }

    return lexer->file_id;
}

static inline size_t
vitte_lexer_offset(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return 0u;
    }

    return lexer->offset;
}

static inline size_t
vitte_lexer_line(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return 0u;
    }

    return lexer->line;
}

static inline size_t
vitte_lexer_column(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return 0u;
    }

    return lexer->column;
}

/* ========================================================================= */
/* Inline token queries                                                      */
/* ========================================================================= */

static inline bool
vitte_token_is(
    const vitte_token_t *token,
    vitte_token_kind_t kind)
{
    return token != NULL &&
           token->kind == kind;
}

static inline bool
vitte_token_is_eof(
    const vitte_token_t *token)
{
    return vitte_token_is(token, VITTE_TOKEN_EOF);
}

static inline bool
vitte_token_is_identifier(
    const vitte_token_t *token)
{
    return vitte_token_is(
        token,
        VITTE_TOKEN_IDENTIFIER);
}

static inline bool
vitte_token_is_literal_kind(
    vitte_token_kind_t kind)
{
    return kind == VITTE_TOKEN_INTEGER_LITERAL ||
           kind == VITTE_TOKEN_FLOAT_LITERAL ||
           kind == VITTE_TOKEN_STRING_LITERAL ||
           kind == VITTE_TOKEN_CHARACTER_LITERAL ||
           kind == VITTE_TOKEN_KW_TRUE ||
           kind == VITTE_TOKEN_KW_FALSE ||
           kind == VITTE_TOKEN_KW_NULL;
}

static inline bool
vitte_token_is_literal(
    const vitte_token_t *token)
{
    return token != NULL &&
           vitte_token_is_literal_kind(token->kind);
}

static inline bool
vitte_token_kind_is_keyword(
    vitte_token_kind_t kind)
{
    return kind >= VITTE_TOKEN_KW_SPACE &&
           kind <= VITTE_TOKEN_KW_TYPEOF;
}

static inline bool
vitte_token_is_keyword(
    const vitte_token_t *token)
{
    return token != NULL &&
           vitte_token_kind_is_keyword(token->kind);
}

static inline bool
vitte_token_kind_is_assignment(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_EQUAL:
        case VITTE_TOKEN_PLUS_EQUAL:
        case VITTE_TOKEN_MINUS_EQUAL:
        case VITTE_TOKEN_STAR_EQUAL:
        case VITTE_TOKEN_SLASH_EQUAL:
        case VITTE_TOKEN_PERCENT_EQUAL:
        case VITTE_TOKEN_AMP_EQUAL:
        case VITTE_TOKEN_PIPE_EQUAL:
        case VITTE_TOKEN_CARET_EQUAL:
        case VITTE_TOKEN_SHIFT_LEFT_EQUAL:
        case VITTE_TOKEN_SHIFT_RIGHT_EQUAL:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_token_kind_is_comparison(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_EQUAL_EQUAL:
        case VITTE_TOKEN_BANG_EQUAL:
        case VITTE_TOKEN_LESS:
        case VITTE_TOKEN_LESS_EQUAL:
        case VITTE_TOKEN_GREATER:
        case VITTE_TOKEN_GREATER_EQUAL:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_token_kind_is_delimiter(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_LEFT_PAREN:
        case VITTE_TOKEN_RIGHT_PAREN:
        case VITTE_TOKEN_LEFT_BRACE:
        case VITTE_TOKEN_RIGHT_BRACE:
        case VITTE_TOKEN_LEFT_BRACKET:
        case VITTE_TOKEN_RIGHT_BRACKET:
        case VITTE_TOKEN_COMMA:
        case VITTE_TOKEN_SEMICOLON:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_token_kind_is_operator(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_COLON:
        case VITTE_TOKEN_COLON_COLON:
        case VITTE_TOKEN_DOT:
        case VITTE_TOKEN_DOT_DOT:
        case VITTE_TOKEN_DOT_DOT_EQUAL:

        case VITTE_TOKEN_ARROW:
        case VITTE_TOKEN_FAT_ARROW:

        case VITTE_TOKEN_PLUS:
        case VITTE_TOKEN_MINUS:
        case VITTE_TOKEN_STAR:
        case VITTE_TOKEN_SLASH:
        case VITTE_TOKEN_PERCENT:

        case VITTE_TOKEN_AMP:
        case VITTE_TOKEN_PIPE:
        case VITTE_TOKEN_CARET:
        case VITTE_TOKEN_TILDE:
        case VITTE_TOKEN_BANG:

        case VITTE_TOKEN_EQUAL:
        case VITTE_TOKEN_EQUAL_EQUAL:
        case VITTE_TOKEN_BANG_EQUAL:

        case VITTE_TOKEN_LESS:
        case VITTE_TOKEN_LESS_EQUAL:
        case VITTE_TOKEN_GREATER:
        case VITTE_TOKEN_GREATER_EQUAL:

        case VITTE_TOKEN_SHIFT_LEFT:
        case VITTE_TOKEN_SHIFT_RIGHT:

        case VITTE_TOKEN_PLUS_EQUAL:
        case VITTE_TOKEN_MINUS_EQUAL:
        case VITTE_TOKEN_STAR_EQUAL:
        case VITTE_TOKEN_SLASH_EQUAL:
        case VITTE_TOKEN_PERCENT_EQUAL:

        case VITTE_TOKEN_AMP_EQUAL:
        case VITTE_TOKEN_PIPE_EQUAL:
        case VITTE_TOKEN_CARET_EQUAL:

        case VITTE_TOKEN_SHIFT_LEFT_EQUAL:
        case VITTE_TOKEN_SHIFT_RIGHT_EQUAL:

        case VITTE_TOKEN_AMP_AMP:
        case VITTE_TOKEN_PIPE_PIPE:

        case VITTE_TOKEN_QUESTION:
        case VITTE_TOKEN_QUESTION_QUESTION:

        case VITTE_TOKEN_AT:
        case VITTE_TOKEN_HASH:
        case VITTE_TOKEN_DOLLAR:
            return true;

        default:
            return false;
    }
}

static inline const char *
vitte_token_data(
    const vitte_token_t *token)
{
    return token != NULL
        ? token->lexeme
        : NULL;
}

static inline size_t
vitte_token_length(
    const vitte_token_t *token)
{
    return token != NULL
        ? token->length
        : 0u;
}

static inline vitte_lexer_span_t
vitte_token_span(
    const vitte_token_t *token)
{
    return token != NULL
        ? token->span
        : vitte_lexer_span_invalid();
}

/* ========================================================================= */
/* Indexed helpers                                                           */
/* ========================================================================= */

static inline const vitte_token_t *
vitte_lexer_first_token(
    const vitte_lexer_t *lexer)
{
    return vitte_lexer_token_at(lexer, 0u);
}

static inline const vitte_token_t *
vitte_lexer_last_token(
    const vitte_lexer_t *lexer)
{
    size_t count;

    count = vitte_lexer_token_count(lexer);

    if (count == 0u) {
        return NULL;
    }

    return vitte_lexer_token_at(
        lexer,
        count - 1u);
}

static inline const vitte_token_t *
vitte_lexer_eof_token(
    const vitte_lexer_t *lexer)
{
    const vitte_token_t *token;

    token = vitte_lexer_last_token(lexer);

    if (token == NULL ||
        token->kind != VITTE_TOKEN_EOF) {
        return NULL;
    }

    return token;
}

/* ========================================================================= */
/* Resource-limit queries                                                    */
/* ========================================================================= */

static inline size_t
vitte_lexer_max_tokens(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return 0u;
    }

    return lexer->max_tokens;
}

static inline size_t
vitte_lexer_max_source_bytes(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return 0u;
    }

    return lexer->max_source_bytes;
}

static inline size_t
vitte_lexer_max_comment_depth(
    const vitte_lexer_t *lexer)
{
    if (lexer == NULL ||
        lexer->magic != VITTE_LEXER_MAGIC) {
        return 0u;
    }

    return lexer->max_comment_depth;
}

/* ========================================================================= */
/* Compile-time contract                                                     */
/* ========================================================================= */

#if defined(__cplusplus)

#define VITTE_LEXER_STATIC_ASSERT(condition, message) \
    static_assert((condition), message)

#else

#define VITTE_LEXER_STATIC_ASSERT(condition, message) \
    _Static_assert((condition), message)

#endif

VITTE_LEXER_STATIC_ASSERT(
    VITTE_LEXER_DEFAULT_INITIAL_CAPACITY > 0u,
    "lexer initial capacity must be non-zero");

VITTE_LEXER_STATIC_ASSERT(
    VITTE_LEXER_DEFAULT_MAX_TOKENS > 0u,
    "lexer token limit must be non-zero");

VITTE_LEXER_STATIC_ASSERT(
    VITTE_LEXER_DEFAULT_MAX_SOURCE_BYTES > 0u,
    "lexer source limit must be non-zero");

VITTE_LEXER_STATIC_ASSERT(
    VITTE_LEXER_DEFAULT_MAX_COMMENT_DEPTH > 0u,
    "lexer comment depth must be non-zero");

VITTE_LEXER_STATIC_ASSERT(
    VITTE_LEXER_ERROR_COUNT >
        VITTE_LEXER_ERROR_INTERNAL,
    "lexer error enum invariant");

VITTE_LEXER_STATIC_ASSERT(
    VITTE_LEXER_STATE_COUNT >
        VITTE_LEXER_STATE_DESTROYED,
    "lexer state enum invariant");

VITTE_LEXER_STATIC_ASSERT(
    VITTE_TOKEN_COUNT >
        VITTE_TOKEN_DOLLAR,
    "lexer token enum invariant");

VITTE_LEXER_STATIC_ASSERT(
    VITTE_TOKEN_KW_SPACE <
        VITTE_TOKEN_KW_TYPEOF,
    "lexer keyword range invariant");

VITTE_LEXER_STATIC_ASSERT(
    sizeof(uint64_t) == 8u,
    "lexer requires 64-bit uint64_t");

#undef VITTE_LEXER_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif /* VITTE_LEXER_LEXER_H */
