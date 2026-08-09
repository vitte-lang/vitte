#include "lexer.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>

static void vitte_lexer_set_error(
    vitte_lexer_t *lexer,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (lexer != NULL) {
        vitte_error_set_details(&lexer->last_error, status, code, message, details);
    }
}

void vitte_token_init(vitte_token_t *token) {
    if (token == NULL) {
        return;
    }
    memset(token, 0, sizeof(*token));
    token->kind = VITTE_TOKEN_ERROR;
}

void vitte_lexer_options_init(vitte_lexer_options_t *options) {
    if (options == NULL) {
        return;
    }
    memset(options, 0, sizeof(*options));
    options->keywords_enabled = true;
    options->max_source_bytes = VITTE_LEXER_DEFAULT_MAX_SOURCE_BYTES;
    options->tab_width = VITTE_LEXER_DEFAULT_TAB_WIDTH;
}

void vitte_lexer_result_init(vitte_lexer_result_t *result) {
    if (result == NULL) {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    vitte_token_init(&result->last_token);
}

void vitte_lexer_stats_init(vitte_lexer_stats_t *stats) {
    if (stats == NULL) {
        return;
    }
    memset(stats, 0, sizeof(*stats));
    stats->line_count = 1u;
}

vitte_status_t vitte_lexer_init(
    vitte_lexer_t *lexer,
    const char *source_name,
    const char *source,
    size_t length,
    const vitte_lexer_options_t *options
) {
    vitte_lexer_options_t defaults;

    if (lexer == NULL || source == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (length == 0u) {
        length = strlen(source);
    }

    memset(lexer, 0, sizeof(*lexer));
    vitte_error_init(&lexer->last_error);
    if (options == NULL) {
        vitte_lexer_options_init(&defaults);
        lexer->options = defaults;
    } else {
        lexer->options = *options;
    }
    if (lexer->options.max_source_bytes == 0u) {
        lexer->options.max_source_bytes = VITTE_LEXER_DEFAULT_MAX_SOURCE_BYTES;
    }
    if (lexer->options.tab_width == 0u) {
        lexer->options.tab_width = VITTE_LEXER_DEFAULT_TAB_WIDTH;
    }
    if (length > lexer->options.max_source_bytes) {
        vitte_lexer_set_error(lexer, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_LEXER_E_SOURCE", "source exceeds lexer size limit", source_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    lexer->initialized = true;
    lexer->source_name = source_name != NULL ? source_name : "<memory>";
    lexer->source = source;
    lexer->length = length;
    lexer->cursor.offset = 0u;
    lexer->cursor.line = 1u;
    lexer->cursor.column = 1u;
    vitte_lexer_stats_init(&lexer->stats);
    return VITTE_STATUS_OK;
}

void vitte_lexer_reset(vitte_lexer_t *lexer) {
    if (lexer == NULL) {
        return;
    }
    lexer->cursor.offset = 0u;
    lexer->cursor.line = 1u;
    lexer->cursor.column = 1u;
    lexer->has_peeked = false;
    vitte_lexer_stats_init(&lexer->stats);
    vitte_error_reset(&lexer->last_error);
}

bool vitte_lexer_is_initialized(const vitte_lexer_t *lexer) {
    return lexer != NULL && lexer->initialized && lexer->source != NULL;
}

const vitte_error_t *vitte_lexer_last_error(const vitte_lexer_t *lexer) {
    return lexer != NULL ? &lexer->last_error : vitte_error_last();
}

const vitte_lexer_stats_t *vitte_lexer_stats(const vitte_lexer_t *lexer) {
    return lexer != NULL ? &lexer->stats : NULL;
}

const char *vitte_token_kind_name(vitte_token_kind_t kind) {
    switch (kind) {
        case VITTE_TOKEN_ERROR:
            return "error";
        case VITTE_TOKEN_EOF:
            return "eof";
        case VITTE_TOKEN_WHITESPACE:
            return "whitespace";
        case VITTE_TOKEN_COMMENT:
            return "comment";
        case VITTE_TOKEN_IDENTIFIER:
            return "identifier";
        case VITTE_TOKEN_INTEGER:
            return "integer";
        case VITTE_TOKEN_STRING:
            return "string";
        case VITTE_TOKEN_KW_SPACE:
            return "space";
        case VITTE_TOKEN_KW_USE:
            return "use";
        case VITTE_TOKEN_KW_EXPORT:
            return "export";
        case VITTE_TOKEN_KW_AS:
            return "as";
        case VITTE_TOKEN_KW_PROC:
            return "proc";
        case VITTE_TOKEN_KW_CONST:
            return "const";
        case VITTE_TOKEN_KW_LET:
            return "let";
        case VITTE_TOKEN_KW_SET:
            return "set";
        case VITTE_TOKEN_KW_MUT:
            return "mut";
        case VITTE_TOKEN_KW_IF:
            return "if";
        case VITTE_TOKEN_KW_ELSE:
            return "else";
        case VITTE_TOKEN_KW_GIVE:
            return "give";
        case VITTE_TOKEN_KW_PICK:
            return "pick";
        case VITTE_TOKEN_KW_AND:
            return "and";
        case VITTE_TOKEN_KW_OR:
            return "or";
        case VITTE_TOKEN_KW_NOT:
            return "not";
        case VITTE_TOKEN_LPAREN:
            return "lparen";
        case VITTE_TOKEN_RPAREN:
            return "rparen";
        case VITTE_TOKEN_LBRACKET:
            return "lbracket";
        case VITTE_TOKEN_RBRACKET:
            return "rbracket";
        case VITTE_TOKEN_LBRACE:
            return "lbrace";
        case VITTE_TOKEN_RBRACE:
            return "rbrace";
        case VITTE_TOKEN_COMMA:
            return "comma";
        case VITTE_TOKEN_DOT:
            return "dot";
        case VITTE_TOKEN_COLON:
            return "colon";
        case VITTE_TOKEN_DOUBLE_COLON:
            return "double_colon";
        case VITTE_TOKEN_SEMICOLON:
            return "semicolon";
        case VITTE_TOKEN_EQUAL:
            return "equal";
        case VITTE_TOKEN_EQUAL_EQUAL:
            return "equal_equal";
        case VITTE_TOKEN_BANG:
            return "bang";
        case VITTE_TOKEN_BANG_EQUAL:
            return "bang_equal";
        case VITTE_TOKEN_PLUS:
            return "plus";
        case VITTE_TOKEN_MINUS:
            return "minus";
        case VITTE_TOKEN_STAR:
            return "star";
        case VITTE_TOKEN_SLASH:
            return "slash";
        case VITTE_TOKEN_PERCENT:
            return "percent";
        case VITTE_TOKEN_AMP:
            return "amp";
        case VITTE_TOKEN_AMP_AMP:
            return "amp_amp";
        case VITTE_TOKEN_PIPE:
            return "pipe";
        case VITTE_TOKEN_PIPE_PIPE:
            return "pipe_pipe";
        case VITTE_TOKEN_CARET:
            return "caret";
        case VITTE_TOKEN_LESS:
            return "less";
        case VITTE_TOKEN_LESS_EQUAL:
            return "less_equal";
        case VITTE_TOKEN_SHIFT_LEFT:
            return "shift_left";
        case VITTE_TOKEN_GREATER:
            return "greater";
        case VITTE_TOKEN_GREATER_EQUAL:
            return "greater_equal";
        case VITTE_TOKEN_SHIFT_RIGHT:
            return "shift_right";
        case VITTE_TOKEN_ARROW:
            return "arrow";
        case VITTE_TOKEN_KIND_COUNT:
        default:
            return "unknown";
    }
}

bool vitte_token_kind_is_keyword(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_KW_SPACE ||
        kind == VITTE_TOKEN_KW_USE ||
        kind == VITTE_TOKEN_KW_EXPORT ||
        kind == VITTE_TOKEN_KW_AS ||
        kind == VITTE_TOKEN_KW_PROC ||
        kind == VITTE_TOKEN_KW_CONST ||
        kind == VITTE_TOKEN_KW_LET ||
        kind == VITTE_TOKEN_KW_SET ||
        kind == VITTE_TOKEN_KW_MUT ||
        kind == VITTE_TOKEN_KW_IF ||
        kind == VITTE_TOKEN_KW_ELSE ||
        kind == VITTE_TOKEN_KW_GIVE ||
        kind == VITTE_TOKEN_KW_PICK ||
        kind == VITTE_TOKEN_KW_AND ||
        kind == VITTE_TOKEN_KW_OR ||
        kind == VITTE_TOKEN_KW_NOT;
}

static bool vitte_lexer_at_end(const vitte_lexer_t *lexer) {
    return lexer == NULL || lexer->cursor.offset >= lexer->length;
}

static char vitte_lexer_peek_char(const vitte_lexer_t *lexer) {
    return vitte_lexer_at_end(lexer) ? '\0' : lexer->source[lexer->cursor.offset];
}

static char vitte_lexer_peek_next_char(const vitte_lexer_t *lexer) {
    return lexer == NULL || lexer->cursor.offset + 1u >= lexer->length ? '\0' : lexer->source[lexer->cursor.offset + 1u];
}

static char vitte_lexer_advance_char(vitte_lexer_t *lexer) {
    char value;

    if (vitte_lexer_at_end(lexer)) {
        return '\0';
    }
    value = lexer->source[lexer->cursor.offset];
    lexer->cursor.offset++;
    if (value == '\n') {
        lexer->cursor.line++;
        lexer->cursor.column = 1u;
        lexer->stats.line_count = lexer->cursor.line;
    } else if (value == '\t') {
        lexer->cursor.column += (uint32_t)lexer->options.tab_width;
    } else {
        lexer->cursor.column++;
    }
    lexer->stats.bytes_consumed = lexer->cursor.offset;
    return value;
}

static bool vitte_lexer_match_char(vitte_lexer_t *lexer, char expected) {
    if (vitte_lexer_peek_char(lexer) != expected) {
        return false;
    }
    (void)vitte_lexer_advance_char(lexer);
    return true;
}

static void vitte_lexer_fill_token(
    vitte_lexer_t *lexer,
    vitte_token_t *token,
    vitte_token_kind_t kind,
    size_t start_offset,
    uint32_t start_line,
    uint32_t start_column,
    const char *message
) {
    vitte_token_init(token);
    token->kind = kind;
    token->source_name = lexer != NULL ? lexer->source_name : NULL;
    token->lexeme_start = lexer != NULL ? lexer->source + start_offset : NULL;
    token->lexeme_length = lexer != NULL ? lexer->cursor.offset - start_offset : 0u;
    token->start_offset = start_offset;
    token->end_offset = lexer != NULL ? lexer->cursor.offset : start_offset;
    token->start_line = start_line;
    token->start_column = start_column;
    token->end_line = lexer != NULL ? lexer->cursor.line : start_line;
    token->end_column = lexer != NULL ? lexer->cursor.column : start_column;
    token->message = message;
}

static void vitte_lexer_count_token(vitte_lexer_t *lexer, const vitte_token_t *token) {
    if (lexer == NULL || token == NULL) {
        return;
    }
    lexer->stats.token_count++;
    if (token->kind == VITTE_TOKEN_ERROR) {
        lexer->stats.error_count++;
    } else if (token->kind == VITTE_TOKEN_COMMENT) {
        lexer->stats.comment_count++;
    } else if (token->kind == VITTE_TOKEN_WHITESPACE) {
        lexer->stats.whitespace_count++;
    }
}

static vitte_token_kind_t vitte_lexer_keyword_kind(const char *start, size_t length) {
    if (length == 5u && memcmp(start, "space", 5u) == 0) {
        return VITTE_TOKEN_KW_SPACE;
    }
    if (length == 3u && memcmp(start, "use", 3u) == 0) {
        return VITTE_TOKEN_KW_USE;
    }
    if (length == 6u && memcmp(start, "export", 6u) == 0) {
        return VITTE_TOKEN_KW_EXPORT;
    }
    if (length == 2u && memcmp(start, "as", 2u) == 0) {
        return VITTE_TOKEN_KW_AS;
    }
    if (length == 4u && memcmp(start, "proc", 4u) == 0) {
        return VITTE_TOKEN_KW_PROC;
    }
    if (length == 5u && memcmp(start, "const", 5u) == 0) {
        return VITTE_TOKEN_KW_CONST;
    }
    if (length == 3u && memcmp(start, "let", 3u) == 0) {
        return VITTE_TOKEN_KW_LET;
    }
    if (length == 3u && memcmp(start, "set", 3u) == 0) {
        return VITTE_TOKEN_KW_SET;
    }
    if (length == 3u && memcmp(start, "mut", 3u) == 0) {
        return VITTE_TOKEN_KW_MUT;
    }
    if (length == 2u && memcmp(start, "if", 2u) == 0) {
        return VITTE_TOKEN_KW_IF;
    }
    if (length == 4u && memcmp(start, "else", 4u) == 0) {
        return VITTE_TOKEN_KW_ELSE;
    }
    if (length == 4u && memcmp(start, "give", 4u) == 0) {
        return VITTE_TOKEN_KW_GIVE;
    }
    if (length == 4u && memcmp(start, "pick", 4u) == 0) {
        return VITTE_TOKEN_KW_PICK;
    }
    if (length == 3u && memcmp(start, "and", 3u) == 0) {
        return VITTE_TOKEN_KW_AND;
    }
    if (length == 2u && memcmp(start, "or", 2u) == 0) {
        return VITTE_TOKEN_KW_OR;
    }
    if (length == 3u && memcmp(start, "not", 3u) == 0) {
        return VITTE_TOKEN_KW_NOT;
    }
    return VITTE_TOKEN_IDENTIFIER;
}

static bool vitte_lexer_is_identifier_start(char value) {
    return isalpha((unsigned char)value) != 0 || value == '_';
}

static bool vitte_lexer_is_identifier_continue(char value) {
    return isalnum((unsigned char)value) != 0 || value == '_';
}

static vitte_status_t vitte_lexer_scan_whitespace(vitte_lexer_t *lexer, vitte_token_t *token) {
    size_t start_offset = lexer->cursor.offset;
    uint32_t start_line = lexer->cursor.line;
    uint32_t start_column = lexer->cursor.column;

    while (!vitte_lexer_at_end(lexer)) {
        char value = vitte_lexer_peek_char(lexer);
        if (value != ' ' && value != '\t' && value != '\r' && value != '\n') {
            break;
        }
        (void)vitte_lexer_advance_char(lexer);
    }
    vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_WHITESPACE, start_offset, start_line, start_column, NULL);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_lexer_scan_line_comment(vitte_lexer_t *lexer, vitte_token_t *token) {
    size_t start_offset = lexer->cursor.offset;
    uint32_t start_line = lexer->cursor.line;
    uint32_t start_column = lexer->cursor.column;

    (void)vitte_lexer_advance_char(lexer);
    (void)vitte_lexer_advance_char(lexer);
    while (!vitte_lexer_at_end(lexer) && vitte_lexer_peek_char(lexer) != '\n') {
        (void)vitte_lexer_advance_char(lexer);
    }
    vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_COMMENT, start_offset, start_line, start_column, NULL);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_lexer_scan_block_comment(vitte_lexer_t *lexer, vitte_token_t *token) {
    size_t start_offset = lexer->cursor.offset;
    uint32_t start_line = lexer->cursor.line;
    uint32_t start_column = lexer->cursor.column;

    (void)vitte_lexer_advance_char(lexer);
    (void)vitte_lexer_advance_char(lexer);
    while (!vitte_lexer_at_end(lexer)) {
        if (vitte_lexer_peek_char(lexer) == '*' && vitte_lexer_peek_next_char(lexer) == '/') {
            (void)vitte_lexer_advance_char(lexer);
            (void)vitte_lexer_advance_char(lexer);
            vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_COMMENT, start_offset, start_line, start_column, NULL);
            return VITTE_STATUS_OK;
        }
        (void)vitte_lexer_advance_char(lexer);
    }
    vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_ERROR, start_offset, start_line, start_column, "unterminated block comment");
    vitte_lexer_set_error(lexer, VITTE_STATUS_ERROR_PARSE, "VITTE_LEXER_E_COMMENT", "unterminated block comment", lexer->source_name);
    return VITTE_STATUS_ERROR_PARSE;
}

static vitte_status_t vitte_lexer_scan_identifier(vitte_lexer_t *lexer, vitte_token_t *token) {
    size_t start_offset = lexer->cursor.offset;
    uint32_t start_line = lexer->cursor.line;
    uint32_t start_column = lexer->cursor.column;
    vitte_token_kind_t kind = VITTE_TOKEN_ERROR;

    (void)vitte_lexer_advance_char(lexer);
    while (vitte_lexer_is_identifier_continue(vitte_lexer_peek_char(lexer))) {
        (void)vitte_lexer_advance_char(lexer);
    }
    kind = VITTE_TOKEN_IDENTIFIER;
    if (lexer->options.keywords_enabled) {
        kind = vitte_lexer_keyword_kind(lexer->source + start_offset, lexer->cursor.offset - start_offset);
    }
    vitte_lexer_fill_token(lexer, token, kind, start_offset, start_line, start_column, NULL);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_lexer_scan_number(vitte_lexer_t *lexer, vitte_token_t *token) {
    size_t start_offset = lexer->cursor.offset;
    uint32_t start_line = lexer->cursor.line;
    uint32_t start_column = lexer->cursor.column;
    int64_t value = 0;
    bool overflow = false;

    while (isdigit((unsigned char)vitte_lexer_peek_char(lexer)) != 0) {
        int digit = vitte_lexer_advance_char(lexer) - '0';
        if (!overflow) {
            if (value > (INT64_MAX - digit) / 10) {
                overflow = true;
            } else {
                value = value * 10 + digit;
            }
        }
    }
    vitte_lexer_fill_token(lexer, token, overflow ? VITTE_TOKEN_ERROR : VITTE_TOKEN_INTEGER, start_offset, start_line, start_column, overflow ? "integer literal overflow" : NULL);
    if (overflow) {
        vitte_lexer_set_error(lexer, VITTE_STATUS_ERROR_PARSE, "VITTE_LEXER_E_INTEGER", "integer literal overflow", lexer->source_name);
        return VITTE_STATUS_ERROR_PARSE;
    }
    token->integer_value = value;
    token->has_integer_value = true;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_lexer_scan_string(vitte_lexer_t *lexer, vitte_token_t *token) {
    size_t start_offset = lexer->cursor.offset;
    uint32_t start_line = lexer->cursor.line;
    uint32_t start_column = lexer->cursor.column;
    bool has_escape = false;

    (void)vitte_lexer_advance_char(lexer);
    while (!vitte_lexer_at_end(lexer)) {
        char value = vitte_lexer_peek_char(lexer);
        if (value == '"') {
            (void)vitte_lexer_advance_char(lexer);
            vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_STRING, start_offset, start_line, start_column, NULL);
            token->has_escape = has_escape;
            return VITTE_STATUS_OK;
        }
        if (value == '\n' || value == '\r') {
            break;
        }
        if (value == '\\') {
            has_escape = true;
            (void)vitte_lexer_advance_char(lexer);
            if (vitte_lexer_at_end(lexer)) {
                break;
            }
            switch (vitte_lexer_peek_char(lexer)) {
                case '\\':
                case '"':
                case 'n':
                case 'r':
                case 't':
                    (void)vitte_lexer_advance_char(lexer);
                    break;
                default:
                    (void)vitte_lexer_advance_char(lexer);
                    vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_ERROR, start_offset, start_line, start_column, "invalid string escape");
                    token->has_escape = true;
                    vitte_lexer_set_error(lexer, VITTE_STATUS_ERROR_PARSE, "VITTE_LEXER_E_STRING", "invalid string escape", lexer->source_name);
                    return VITTE_STATUS_ERROR_PARSE;
            }
            continue;
        }
        (void)vitte_lexer_advance_char(lexer);
    }

    vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_ERROR, start_offset, start_line, start_column, "unterminated string literal");
    token->has_escape = has_escape;
    vitte_lexer_set_error(lexer, VITTE_STATUS_ERROR_PARSE, "VITTE_LEXER_E_STRING", "unterminated string literal", lexer->source_name);
    return VITTE_STATUS_ERROR_PARSE;
}

static vitte_status_t vitte_lexer_scan_punct(vitte_lexer_t *lexer, vitte_token_t *token) {
    size_t start_offset = lexer->cursor.offset;
    uint32_t start_line = lexer->cursor.line;
    uint32_t start_column = lexer->cursor.column;
    char value = vitte_lexer_advance_char(lexer);
    vitte_token_kind_t kind = VITTE_TOKEN_ERROR;

    switch (value) {
        case '(':
            kind = VITTE_TOKEN_LPAREN;
            break;
        case ')':
            kind = VITTE_TOKEN_RPAREN;
            break;
        case '[':
            kind = VITTE_TOKEN_LBRACKET;
            break;
        case ']':
            kind = VITTE_TOKEN_RBRACKET;
            break;
        case '{':
            kind = VITTE_TOKEN_LBRACE;
            break;
        case '}':
            kind = VITTE_TOKEN_RBRACE;
            break;
        case ',':
            kind = VITTE_TOKEN_COMMA;
            break;
        case '.':
            kind = VITTE_TOKEN_DOT;
            break;
        case ':':
            kind = vitte_lexer_match_char(lexer, ':') ? VITTE_TOKEN_DOUBLE_COLON : VITTE_TOKEN_COLON;
            break;
        case ';':
            kind = VITTE_TOKEN_SEMICOLON;
            break;
        case '+':
            kind = VITTE_TOKEN_PLUS;
            break;
        case '*':
            kind = VITTE_TOKEN_STAR;
            break;
        case '/':
            kind = VITTE_TOKEN_SLASH;
            break;
        case '%':
            kind = VITTE_TOKEN_PERCENT;
            break;
        case '&':
            kind = vitte_lexer_match_char(lexer, '&') ? VITTE_TOKEN_AMP_AMP : VITTE_TOKEN_AMP;
            break;
        case '|':
            kind = vitte_lexer_match_char(lexer, '|') ? VITTE_TOKEN_PIPE_PIPE : VITTE_TOKEN_PIPE;
            break;
        case '^':
            kind = VITTE_TOKEN_CARET;
            break;
        case '=':
            kind = vitte_lexer_match_char(lexer, '=') ? VITTE_TOKEN_EQUAL_EQUAL : VITTE_TOKEN_EQUAL;
            break;
        case '!':
            kind = vitte_lexer_match_char(lexer, '=') ? VITTE_TOKEN_BANG_EQUAL : VITTE_TOKEN_BANG;
            break;
        case '-':
            kind = vitte_lexer_match_char(lexer, '>') ? VITTE_TOKEN_ARROW : VITTE_TOKEN_MINUS;
            break;
        case '<':
            if (vitte_lexer_peek_char(lexer) == '<' && vitte_lexer_peek_next_char(lexer) == '<') {
                (void)vitte_lexer_advance_char(lexer);
                (void)vitte_lexer_advance_char(lexer);
                while (!vitte_lexer_at_end(lexer)) {
                    if (vitte_lexer_peek_char(lexer) == '>' &&
                        vitte_lexer_peek_next_char(lexer) == '>' &&
                        lexer->cursor.offset + 2u < lexer->length &&
                        lexer->source[lexer->cursor.offset + 2u] == '>') {
                        (void)vitte_lexer_advance_char(lexer);
                        (void)vitte_lexer_advance_char(lexer);
                        (void)vitte_lexer_advance_char(lexer);
                        kind = VITTE_TOKEN_COMMENT;
                        break;
                    }
                    (void)vitte_lexer_advance_char(lexer);
                }
                if (kind != VITTE_TOKEN_COMMENT) {
                    vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_ERROR, start_offset, start_line, start_column, "unterminated contract block");
                    vitte_lexer_set_error(lexer, VITTE_STATUS_ERROR_PARSE, "VITTE_LEXER_E_COMMENT", "unterminated contract block", lexer->source_name);
                    return VITTE_STATUS_ERROR_PARSE;
                }
                break;
            }
            if (vitte_lexer_match_char(lexer, '=')) {
                kind = VITTE_TOKEN_LESS_EQUAL;
            } else if (vitte_lexer_match_char(lexer, '<')) {
                kind = VITTE_TOKEN_SHIFT_LEFT;
            } else {
                kind = VITTE_TOKEN_LESS;
            }
            break;
        case '>':
            if (vitte_lexer_match_char(lexer, '=')) {
                kind = VITTE_TOKEN_GREATER_EQUAL;
            } else if (vitte_lexer_match_char(lexer, '>')) {
                kind = VITTE_TOKEN_SHIFT_RIGHT;
            } else {
                kind = VITTE_TOKEN_GREATER;
            }
            break;
        default:
            vitte_lexer_fill_token(lexer, token, VITTE_TOKEN_ERROR, start_offset, start_line, start_column, "unexpected character");
            vitte_lexer_set_error(lexer, VITTE_STATUS_ERROR_PARSE, "VITTE_LEXER_E_TOKEN", "unexpected character", NULL);
            return VITTE_STATUS_ERROR_PARSE;
    }

    vitte_lexer_fill_token(lexer, token, kind, start_offset, start_line, start_column, NULL);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_lexer_scan_one(vitte_lexer_t *lexer, vitte_token_t *token) {
    if (!vitte_lexer_is_initialized(lexer) || token == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    vitte_error_reset(&lexer->last_error);

    while (!vitte_lexer_at_end(lexer)) {
        char value = vitte_lexer_peek_char(lexer);
        if (value == ' ' || value == '\t' || value == '\r' || value == '\n') {
            vitte_status_t status = vitte_lexer_scan_whitespace(lexer, token);
            if (status != VITTE_STATUS_OK || lexer->options.emit_whitespace) {
                vitte_lexer_count_token(lexer, token);
                return status;
            }
            continue;
        }
        if (value == '/' && vitte_lexer_peek_next_char(lexer) == '/') {
            vitte_status_t status = vitte_lexer_scan_line_comment(lexer, token);
            if (status != VITTE_STATUS_OK || lexer->options.emit_comments) {
                vitte_lexer_count_token(lexer, token);
                return status;
            }
            continue;
        }
        if (value == '/' && vitte_lexer_peek_next_char(lexer) == '*') {
            vitte_status_t status = vitte_lexer_scan_block_comment(lexer, token);
            if (status != VITTE_STATUS_OK || lexer->options.emit_comments) {
                vitte_lexer_count_token(lexer, token);
                return status;
            }
            continue;
        }
        if (vitte_lexer_is_identifier_start(value)) {
            vitte_status_t status = vitte_lexer_scan_identifier(lexer, token);
            vitte_lexer_count_token(lexer, token);
            return status;
        }
        if (isdigit((unsigned char)value) != 0) {
            vitte_status_t status = vitte_lexer_scan_number(lexer, token);
            vitte_lexer_count_token(lexer, token);
            return status;
        }
        if (value == '"') {
            vitte_status_t status = vitte_lexer_scan_string(lexer, token);
            vitte_lexer_count_token(lexer, token);
            return status;
        }
        if (value == '<' && vitte_lexer_peek_next_char(lexer) == '<' &&
            lexer->cursor.offset + 2u < lexer->length &&
            lexer->source[lexer->cursor.offset + 2u] == '<') {
            vitte_status_t status = vitte_lexer_scan_punct(lexer, token);
            if (status != VITTE_STATUS_OK || token->kind != VITTE_TOKEN_COMMENT || lexer->options.emit_comments) {
                vitte_lexer_count_token(lexer, token);
                return status;
            }
            continue;
        }
        {
            vitte_status_t status = vitte_lexer_scan_punct(lexer, token);
            vitte_lexer_count_token(lexer, token);
            return status;
        }
    }

    vitte_lexer_fill_token(
        lexer,
        token,
        VITTE_TOKEN_EOF,
        lexer->cursor.offset,
        lexer->cursor.line,
        lexer->cursor.column,
        NULL
    );
    vitte_lexer_count_token(lexer, token);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_lexer_next(vitte_lexer_t *lexer, vitte_token_t *token) {
    if (!vitte_lexer_is_initialized(lexer) || token == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (lexer->has_peeked) {
        *token = lexer->peeked;
        lexer->has_peeked = false;
        return token->kind == VITTE_TOKEN_ERROR ? VITTE_STATUS_ERROR_PARSE : VITTE_STATUS_OK;
    }
    return vitte_lexer_scan_one(lexer, token);
}

vitte_status_t vitte_lexer_peek(vitte_lexer_t *lexer, vitte_token_t *token) {
    vitte_status_t status;

    if (!vitte_lexer_is_initialized(lexer) || token == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!lexer->has_peeked) {
        status = vitte_lexer_scan_one(lexer, &lexer->peeked);
        lexer->has_peeked = true;
        *token = lexer->peeked;
        return status;
    }
    *token = lexer->peeked;
    return token->kind == VITTE_TOKEN_ERROR ? VITTE_STATUS_ERROR_PARSE : VITTE_STATUS_OK;
}

vitte_status_t vitte_lexer_lex_all(
    vitte_lexer_t *lexer,
    vitte_token_t *storage,
    size_t capacity,
    vitte_lexer_result_t *result
) {
    size_t count = 0u;
    vitte_status_t status = VITTE_STATUS_OK;

    if (result != NULL) {
        vitte_lexer_result_init(result);
    }
    if (!vitte_lexer_is_initialized(lexer) || storage == NULL || capacity == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    while (count < capacity) {
        status = vitte_lexer_next(lexer, &storage[count]);
        if (result != NULL) {
            result->last_token = storage[count];
            result->token_count = count + 1u;
            if (storage[count].kind == VITTE_TOKEN_ERROR) {
                result->error_count++;
            }
        }
        count++;
        if (storage[count - 1u].kind == VITTE_TOKEN_EOF || storage[count - 1u].kind == VITTE_TOKEN_ERROR) {
            break;
        }
    }

    if (count == capacity && storage[count - 1u].kind != VITTE_TOKEN_EOF && storage[count - 1u].kind != VITTE_TOKEN_ERROR) {
        if (result != NULL) {
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }
        vitte_lexer_set_error(lexer, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_LEXER_E_CAPACITY", "token storage capacity is too small", lexer->source_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (result != NULL) {
        result->status = status;
    }
    return status;
}
