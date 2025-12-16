// Vitte Compiler - Lexer Header
// Tokenization and lexical analysis

#ifndef VITTE_LEXER_H
#define VITTE_LEXER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Token Types
// ============================================================================

typedef enum {
    // End of input
    TOKEN_EOF = 0,
    
    // Literals
    TOKEN_INTEGER = 100,
    TOKEN_FLOAT = 101,
    TOKEN_STRING = 102,
    TOKEN_CHAR = 103,
    TOKEN_BOOL = 104,
    TOKEN_NIL = 105,
    
    // Identifiers and keywords
    TOKEN_IDENTIFIER = 200,
    TOKEN_KEYWORD = 201,
    
    // Keywords (specific)
    TOKEN_KW_FN = 210,
    TOKEN_KW_LET = 211,
    TOKEN_KW_VAR = 212,
    TOKEN_KW_CONST = 213,
    TOKEN_KW_IF = 214,
    TOKEN_KW_ELSE = 215,
    TOKEN_KW_FOR = 216,
    TOKEN_KW_WHILE = 217,
    TOKEN_KW_RETURN = 218,
    TOKEN_KW_BREAK = 219,
    TOKEN_KW_CONTINUE = 220,
    TOKEN_KW_MATCH = 221,
    TOKEN_KW_TYPE = 222,
    TOKEN_KW_STRUCT = 223,
    TOKEN_KW_ENUM = 224,
    TOKEN_KW_TRAIT = 225,
    TOKEN_KW_IMPL = 226,
    TOKEN_KW_USE = 227,
    TOKEN_KW_PUB = 228,
    TOKEN_KW_ASYNC = 229,
    TOKEN_KW_AWAIT = 230,
    TOKEN_KW_UNSAFE = 231,
    TOKEN_KW_STATIC = 232,
    TOKEN_KW_INLINE = 233,
    TOKEN_KW_EXTERN = 234,
    TOKEN_KW_MUT = 235,
    TOKEN_KW_REF = 236,
    TOKEN_KW_SELF = 237,
    TOKEN_KW_TRUE = 238,
    TOKEN_KW_FALSE = 239,
    TOKEN_KW_AS = 240,
    TOKEN_KW_IN = 241,
    TOKEN_KW_IS = 242,
    
    // Operators
    TOKEN_PLUS = 300,
    TOKEN_MINUS = 301,
    TOKEN_STAR = 302,
    TOKEN_SLASH = 303,
    TOKEN_PERCENT = 304,
    TOKEN_POWER = 305,
    TOKEN_EQ = 306,
    TOKEN_NE = 307,
    TOKEN_LT = 308,
    TOKEN_LE = 309,
    TOKEN_GT = 310,
    TOKEN_GE = 311,
    TOKEN_AND = 312,
    TOKEN_OR = 313,
    TOKEN_NOT = 314,
    TOKEN_BIT_AND = 315,
    TOKEN_BIT_OR = 316,
    TOKEN_BIT_XOR = 317,
    TOKEN_BIT_NOT = 318,
    TOKEN_LSHIFT = 319,
    TOKEN_RSHIFT = 320,
    TOKEN_ASSIGN = 321,
    TOKEN_PLUS_ASSIGN = 322,
    TOKEN_MINUS_ASSIGN = 323,
    TOKEN_STAR_ASSIGN = 324,
    TOKEN_SLASH_ASSIGN = 325,
    TOKEN_PERCENT_ASSIGN = 326,
    TOKEN_AND_ASSIGN = 327,
    TOKEN_OR_ASSIGN = 328,
    TOKEN_XOR_ASSIGN = 329,
    TOKEN_LSHIFT_ASSIGN = 330,
    TOKEN_RSHIFT_ASSIGN = 331,
    TOKEN_ARROW = 332,
    TOKEN_FAT_ARROW = 333,
    TOKEN_RANGE = 334,
    TOKEN_INCLUSIVE_RANGE = 335,
    TOKEN_DOT = 336,
    TOKEN_DOUBLE_DOT = 337,
    TOKEN_TRIPLE_DOT = 338,
    TOKEN_DOUBLE_COLON = 339,
    TOKEN_QUESTION = 340,
    TOKEN_AT = 341,
    TOKEN_DOLLAR = 342,
    TOKEN_PIPE = 343,
    TOKEN_AMPERSAND = 344,
    TOKEN_CARET = 345,
    TOKEN_TILDE = 346,
    TOKEN_BACKTICK = 347,
    TOKEN_HASH = 348,
    
    // Delimiters
    TOKEN_LPAREN = 400,
    TOKEN_RPAREN = 401,
    TOKEN_LBRACE = 402,
    TOKEN_RBRACE = 403,
    TOKEN_LBRACKET = 404,
    TOKEN_RBRACKET = 405,
    TOKEN_SEMICOLON = 406,
    TOKEN_COMMA = 407,
    TOKEN_COLON = 408,
    TOKEN_NEWLINE = 409,
    
    // Special
    TOKEN_COMMENT = 500,
    TOKEN_DOC_COMMENT = 501,
    TOKEN_WHITESPACE = 502,
    TOKEN_ERROR = 503,
} token_type_t;

// ============================================================================
// Token Structure
// ============================================================================

typedef struct {
    token_type_t type;
    const char *lexeme;
    size_t length;
    int line;
    int column;
    int start_pos;
    int end_pos;
    union {
        int64_t int_value;
        double float_value;
        const char *string_value;
    } value;
} token_t;

// ============================================================================
// Lexer Structure
// ============================================================================

typedef struct {
    const char *source;
    size_t source_length;
    size_t current;
    size_t start;
    int line;
    int column;
    int start_column;
    token_t *tokens;
    size_t token_count;
    size_t token_capacity;
} lexer_t;

// ============================================================================
// Lexer API
// ============================================================================

// Create lexer from source string
lexer_t* lexer_create(const char *source);

// Create lexer from file
lexer_t* lexer_create_from_file(const char *filename);

// Tokenize entire source
int lexer_tokenize(lexer_t *lexer);

// Get next token
token_t lexer_next_token(lexer_t *lexer);

// Get token at index
token_t lexer_peek_token(lexer_t *lexer, size_t index);

// Reset lexer position
void lexer_reset(lexer_t *lexer);

// Free lexer
void lexer_free(lexer_t *lexer);

// ============================================================================
// Token Utilities
// ============================================================================

// Get token type name
const char* token_type_name(token_type_t type);

// Check if token is keyword
bool token_is_keyword(token_type_t type);

// Check if token is operator
bool token_is_operator(token_type_t type);

// Check if token is literal
bool token_is_literal(token_type_t type);

// ============================================================================
// Keyword Management
// ============================================================================

// Check if string is keyword
token_type_t lexer_keyword_type(const char *text, size_t length);

// Print token information
void token_print(const token_t *token);

// Get lexeme as string
const char* token_get_lexeme(const token_t *token);

#ifdef __cplusplus
}
#endif

#endif // VITTE_LEXER_H
