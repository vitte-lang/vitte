// Vitte Compiler - Lexer Implementation
// Complete tokenization and lexical analysis

#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// Helper Functions
// ============================================================================

static char* lexer_strdup(const char *str) {
    if (!str) return NULL;
    char *copy = malloc(strlen(str) + 1);
    if (copy) strcpy(copy, str);
    return copy;
}

static bool is_at_end(lexer_t *lexer) {
    return lexer->current >= lexer->source_length;
}

static char current_char(lexer_t *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->source[lexer->current];
}

static char peek_char(lexer_t *lexer, int offset) {
    size_t pos = lexer->current + offset;
    if (pos >= lexer->source_length) return '\0';
    return lexer->source[pos];
}

static void advance(lexer_t *lexer) {
    if (!is_at_end(lexer)) {
        if (lexer->source[lexer->current] == '\n') {
            lexer->line++;
            lexer->column = 0;
        } else {
            lexer->column++;
        }
        lexer->current++;
    }
}

static void skip_whitespace(lexer_t *lexer) {
    while (!is_at_end(lexer) && isspace(current_char(lexer)) && current_char(lexer) != '\n') {
        advance(lexer);
    }
}

static void skip_comment(lexer_t *lexer) {
    // Single-line comment: //
    if (current_char(lexer) == '/' && peek_char(lexer, 1) == '/') {
        while (!is_at_end(lexer) && current_char(lexer) != '\n') {
            advance(lexer);
        }
    }
    // Multi-line comment: /* ... */
    else if (current_char(lexer) == '/' && peek_char(lexer, 1) == '*') {
        advance(lexer); // /
        advance(lexer); // *
        while (!is_at_end(lexer)) {
            if (current_char(lexer) == '*' && peek_char(lexer, 1) == '/') {
                advance(lexer); // *
                advance(lexer); // /
                break;
            }
            advance(lexer);
        }
    }
}

// ============================================================================
// Keyword Recognition
// ============================================================================

token_type_t lexer_keyword_type(const char *text, size_t length) {
    if (!text || length == 0) return TOKEN_IDENTIFIER;
    
    #define KEYWORD(name, token) \
        if (strlen(name) == length && strncmp(text, name, length) == 0) return token
    
    // Control flow
    KEYWORD("if", TOKEN_KW_IF);
    KEYWORD("else", TOKEN_KW_ELSE);
    KEYWORD("for", TOKEN_KW_FOR);
    KEYWORD("while", TOKEN_KW_WHILE);
    KEYWORD("return", TOKEN_KW_RETURN);
    KEYWORD("break", TOKEN_KW_BREAK);
    KEYWORD("continue", TOKEN_KW_CONTINUE);
    KEYWORD("match", TOKEN_KW_MATCH);
    
    // Definitions
    KEYWORD("fn", TOKEN_KW_FN);
    KEYWORD("let", TOKEN_KW_LET);
    KEYWORD("var", TOKEN_KW_VAR);
    KEYWORD("const", TOKEN_KW_CONST);
    KEYWORD("type", TOKEN_KW_TYPE);
    KEYWORD("struct", TOKEN_KW_STRUCT);
    KEYWORD("enum", TOKEN_KW_ENUM);
    KEYWORD("trait", TOKEN_KW_TRAIT);
    KEYWORD("impl", TOKEN_KW_IMPL);
    
    // Modifiers
    KEYWORD("pub", TOKEN_KW_PUB);
    KEYWORD("async", TOKEN_KW_ASYNC);
    KEYWORD("await", TOKEN_KW_AWAIT);
    KEYWORD("unsafe", TOKEN_KW_UNSAFE);
    KEYWORD("static", TOKEN_KW_STATIC);
    KEYWORD("inline", TOKEN_KW_INLINE);
    KEYWORD("extern", TOKEN_KW_EXTERN);
    KEYWORD("mut", TOKEN_KW_MUT);
    KEYWORD("ref", TOKEN_KW_REF);
    
    // Special
    KEYWORD("use", TOKEN_KW_USE);
    KEYWORD("as", TOKEN_KW_AS);
    KEYWORD("in", TOKEN_KW_IN);
    KEYWORD("is", TOKEN_KW_IS);
    KEYWORD("self", TOKEN_KW_SELF);
    KEYWORD("true", TOKEN_KW_TRUE);
    KEYWORD("false", TOKEN_KW_FALSE);
    KEYWORD("nil", TOKEN_NIL);
    
    #undef KEYWORD
    
    return TOKEN_IDENTIFIER;
}

// ============================================================================
// Token Creation
// ============================================================================

static void add_token(lexer_t *lexer, token_type_t type) {
    if (lexer->token_count >= lexer->token_capacity) {
        lexer->token_capacity *= 2;
        token_t *new_tokens = realloc(lexer->tokens, 
            sizeof(token_t) * lexer->token_capacity);
        if (!new_tokens) return;
        lexer->tokens = new_tokens;
    }
    
    token_t *token = &lexer->tokens[lexer->token_count++];
    token->type = type;
    token->length = lexer->current - lexer->start;
    token->lexeme = &lexer->source[lexer->start];
    token->line = lexer->line;
    token->column = lexer->start_column;
    token->start_pos = lexer->start;
    token->end_pos = lexer->current;
}

// ============================================================================
// Number Parsing
// ============================================================================

static token_type_t scan_number(lexer_t *lexer) {
    while (!is_at_end(lexer) && isdigit(current_char(lexer))) {
        advance(lexer);
    }
    
    // Float
    if (current_char(lexer) == '.' && isdigit(peek_char(lexer, 1))) {
        advance(lexer); // .
        while (!is_at_end(lexer) && isdigit(current_char(lexer))) {
            advance(lexer);
        }
        
        // Scientific notation
        if (current_char(lexer) == 'e' || current_char(lexer) == 'E') {
            advance(lexer);
            if (current_char(lexer) == '+' || current_char(lexer) == '-') {
                advance(lexer);
            }
            while (!is_at_end(lexer) && isdigit(current_char(lexer))) {
                advance(lexer);
            }
        }
        
        return TOKEN_FLOAT;
    }
    
    // Hex, octal, binary
    if (current_char(lexer) == 'x' || current_char(lexer) == 'X') {
        advance(lexer);
        while (!is_at_end(lexer) && isxdigit(current_char(lexer))) {
            advance(lexer);
        }
        return TOKEN_INTEGER;
    }
    
    return TOKEN_INTEGER;
}

// ============================================================================
// String Parsing
// ============================================================================

static token_type_t scan_string(lexer_t *lexer, char quote) {
    advance(lexer); // Opening quote
    
    while (!is_at_end(lexer) && current_char(lexer) != quote) {
        if (current_char(lexer) == '\\') {
            advance(lexer);
            if (!is_at_end(lexer)) advance(lexer);
        } else {
            advance(lexer);
        }
    }
    
    if (!is_at_end(lexer)) {
        advance(lexer); // Closing quote
    }
    
    return quote == '"' ? TOKEN_STRING : TOKEN_CHAR;
}

// ============================================================================
// Identifier and Keyword Scanning
// ============================================================================

static token_type_t scan_identifier(lexer_t *lexer) {
    while (!is_at_end(lexer) && (isalnum(current_char(lexer)) || 
           current_char(lexer) == '_')) {
        advance(lexer);
    }
    
    size_t length = lexer->current - lexer->start;
    return lexer_keyword_type(&lexer->source[lexer->start], length);
}

// ============================================================================
// Main Tokenization
// ============================================================================

static token_type_t scan_token(lexer_t *lexer) {
    skip_whitespace(lexer);
    
    // Skip comments
    while (current_char(lexer) == '/' && 
           (peek_char(lexer, 1) == '/' || peek_char(lexer, 1) == '*')) {
        skip_comment(lexer);
        skip_whitespace(lexer);
    }
    
    lexer->start = lexer->current;
    lexer->start_column = lexer->column;
    
    if (is_at_end(lexer)) {
        return TOKEN_EOF;
    }
    
    char c = current_char(lexer);
    advance(lexer);
    
    // Numbers
    if (isdigit(c)) {
        lexer->current--;
        return scan_number(lexer);
    }
    
    // Strings
    if (c == '"' || c == '\'') {
        lexer->current--;
        return scan_string(lexer, c);
    }
    
    // Identifiers and keywords
    if (isalpha(c) || c == '_') {
        lexer->current--;
        return scan_identifier(lexer);
    }
    
    // Operators and delimiters
    switch (c) {
        case '(': return TOKEN_LPAREN;
        case ')': return TOKEN_RPAREN;
        case '{': return TOKEN_LBRACE;
        case '}': return TOKEN_RBRACE;
        case '[': return TOKEN_LBRACKET;
        case ']': return TOKEN_RBRACKET;
        case ';': return TOKEN_SEMICOLON;
        case ',': return TOKEN_COMMA;
        case '~': return TOKEN_BIT_NOT;
        case '?': return TOKEN_QUESTION;
        case '@': return TOKEN_AT;
        case '$': return TOKEN_DOLLAR;
        case '`': return TOKEN_BACKTICK;
        case '#': return TOKEN_HASH;
        
        case ':':
            if (current_char(lexer) == ':') {
                advance(lexer);
                return TOKEN_DOUBLE_COLON;
            }
            return TOKEN_COLON;
        
        case '.':
            if (current_char(lexer) == '.') {
                advance(lexer);
                if (current_char(lexer) == '.') {
                    advance(lexer);
                    return TOKEN_TRIPLE_DOT;
                } else if (current_char(lexer) == '=') {
                    advance(lexer);
                    return TOKEN_INCLUSIVE_RANGE;
                }
                return TOKEN_DOUBLE_DOT;
            }
            return TOKEN_DOT;
        
        case '+':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_PLUS_ASSIGN;
            }
            return TOKEN_PLUS;
        
        case '-':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_MINUS_ASSIGN;
            }
            if (current_char(lexer) == '>') {
                advance(lexer);
                return TOKEN_ARROW;
            }
            return TOKEN_MINUS;
        
        case '*':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_STAR_ASSIGN;
            }
            if (current_char(lexer) == '*') {
                advance(lexer);
                return TOKEN_POWER;
            }
            return TOKEN_STAR;
        
        case '/':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_SLASH_ASSIGN;
            }
            return TOKEN_SLASH;
        
        case '%':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_PERCENT_ASSIGN;
            }
            return TOKEN_PERCENT;
        
        case '=':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_EQ;
            }
            if (current_char(lexer) == '>') {
                advance(lexer);
                return TOKEN_FAT_ARROW;
            }
            return TOKEN_ASSIGN;
        
        case '!':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_NE;
            }
            return TOKEN_NOT;
        
        case '<':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_LE;
            }
            if (current_char(lexer) == '<') {
                advance(lexer);
                if (current_char(lexer) == '=') {
                    advance(lexer);
                    return TOKEN_LSHIFT_ASSIGN;
                }
                return TOKEN_LSHIFT;
            }
            return TOKEN_LT;
        
        case '>':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_GE;
            }
            if (current_char(lexer) == '>') {
                advance(lexer);
                if (current_char(lexer) == '=') {
                    advance(lexer);
                    return TOKEN_RSHIFT_ASSIGN;
                }
                return TOKEN_RSHIFT;
            }
            return TOKEN_GT;
        
        case '&':
            if (current_char(lexer) == '&') {
                advance(lexer);
                return TOKEN_AND;
            }
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_AND_ASSIGN;
            }
            return TOKEN_BIT_AND;
        
        case '|':
            if (current_char(lexer) == '|') {
                advance(lexer);
                return TOKEN_OR;
            }
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_OR_ASSIGN;
            }
            return TOKEN_PIPE;
        
        case '^':
            if (current_char(lexer) == '=') {
                advance(lexer);
                return TOKEN_XOR_ASSIGN;
            }
            return TOKEN_BIT_XOR;
        
        case '\n':
            return TOKEN_NEWLINE;
        
        default:
            return TOKEN_ERROR;
    }
}

// ============================================================================
// Lexer API Implementation
// ============================================================================

lexer_t* lexer_create(const char *source) {
    if (!source) return NULL;
    
    lexer_t *lexer = malloc(sizeof(lexer_t));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->source_length = strlen(source);
    lexer->current = 0;
    lexer->start = 0;
    lexer->line = 1;
    lexer->column = 0;
    lexer->start_column = 0;
    lexer->token_count = 0;
    lexer->token_capacity = 128;
    
    lexer->tokens = malloc(sizeof(token_t) * lexer->token_capacity);
    if (!lexer->tokens) {
        free(lexer);
        return NULL;
    }
    
    return lexer;
}

lexer_t* lexer_create_from_file(const char *filename) {
    if (!filename) return NULL;
    
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *source = malloc(size + 1);
    if (!source) {
        fclose(file);
        return NULL;
    }
    
    fread(source, 1, size, file);
    source[size] = '\0';
    fclose(file);
    
    lexer_t *lexer = lexer_create(source);
    free(source);
    
    return lexer;
}

int lexer_tokenize(lexer_t *lexer) {
    if (!lexer) return -1;
    
    while (!is_at_end(lexer)) {
        token_type_t type = scan_token(lexer);
        if (type == TOKEN_EOF) break;
        add_token(lexer, type);
    }
    
    add_token(lexer, TOKEN_EOF);
    return 0;
}

token_t lexer_next_token(lexer_t *lexer) {
    if (!lexer) {
        return (token_t){.type = TOKEN_EOF};
    }
    
    token_type_t type = scan_token(lexer);
    add_token(lexer, type);
    
    return lexer->tokens[lexer->token_count - 1];
}

token_t lexer_peek_token(lexer_t *lexer, size_t index) {
    if (!lexer || index >= lexer->token_count) {
        return (token_t){.type = TOKEN_EOF};
    }
    return lexer->tokens[index];
}

void lexer_reset(lexer_t *lexer) {
    if (!lexer) return;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 0;
    lexer->token_count = 0;
}

void lexer_free(lexer_t *lexer) {
    if (!lexer) return;
    free(lexer->tokens);
    free(lexer);
}

// ============================================================================
// Token Utilities
// ============================================================================

const char* token_type_name(token_type_t type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_KW_FN: return "fn";
        case TOKEN_KW_LET: return "let";
        case TOKEN_KW_VAR: return "var";
        case TOKEN_KW_IF: return "if";
        case TOKEN_KW_ELSE: return "else";
        case TOKEN_KW_FOR: return "for";
        case TOKEN_KW_WHILE: return "while";
        case TOKEN_KW_RETURN: return "return";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_LBRACE: return "{";
        case TOKEN_RBRACE: return "}";
        case TOKEN_LBRACKET: return "[";
        case TOKEN_RBRACKET: return "]";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_COMMA: return ",";
        case TOKEN_COLON: return ":";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_ASSIGN: return "=";
        case TOKEN_EQ: return "==";
        case TOKEN_NE: return "!=";
        case TOKEN_LT: return "<";
        case TOKEN_LE: return "<=";
        case TOKEN_GT: return ">";
        case TOKEN_GE: return ">=";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool token_is_keyword(token_type_t type) {
    return type >= TOKEN_KW_FN && type <= TOKEN_KW_IN;
}

bool token_is_operator(token_type_t type) {
    return type >= TOKEN_PLUS && type <= TOKEN_TILDE;
}

bool token_is_literal(token_type_t type) {
    return type >= TOKEN_INTEGER && type <= TOKEN_NIL;
}

void token_print(const token_t *token) {
    if (!token) return;
    printf("Token: %s (", token_type_name(token->type));
    for (size_t i = 0; i < token->length; i++) {
        printf("%c", token->lexeme[i]);
    }
    printf(") at line %d, col %d\n", token->line, token->column);
}

const char* token_get_lexeme(const token_t *token) {
    if (!token) return "";
    return token->lexeme;
}
