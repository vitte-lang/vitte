/*
 * Vitte Compiler
 * src/hir/hir.c
 *
 * High-Level Intermediate Representation (HIR)
 * ============================================
 *
 * This translation unit implements the core HIR data model used between
 * semantic analysis and lower-level IR generation.
 *
 * Pipeline:
 *
 *   source
 *     |
 *     v
 *   lexer
 *     |
 *     v
 *   parser / AST
 *     |
 *     v
 *   name resolution
 *     |
 *     v
 *   type checking
 *     |
 *     v
 *   contracts / constant folding
 *     |
 *     v
 *   +-----------------------------+
 *   |             HIR             |
 *   +-----------------------------+
 *     |
 *     v
 *   optimization / canonicalization
 *     |
 *     v
 *   lower IR
 *     |
 *     v
 *   backend / C17 / native
 *
 * Goals:
 *
 *   - explicit ownership;
 *   - deterministic IDs;
 *   - checked allocation arithmetic;
 *   - stable traversal order;
 *   - source provenance;
 *   - typed expressions;
 *   - declarations;
 *   - statements;
 *   - patterns;
 *   - control flow;
 *   - procedures;
 *   - modules/spaces;
 *   - validation;
 *   - hashing/fingerprinting;
 *   - statistics;
 *   - no backend-specific representation leakage.
 *
 * The HIR intentionally remains higher-level than the lower IR.
 *
 * Constructs such as:
 *
 *   - if
 *   - while
 *   - loop
 *   - for
 *   - match
 *   - defer
 *   - calls
 *   - typed binary expressions
 *   - aggregate construction
 *
 * may remain explicit here.
 *
 * The lower IR is responsible for converting these into backend-oriented
 * blocks, branches, temporaries and operations.
 *
 * ISO C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_HIR_MAGIC \
    UINT64_C(0x5649545445484952)

#define VITTE_HIR_DEAD_MAGIC \
    UINT64_C(0x4445414448495221)

#define VITTE_HIR_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_HIR_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_HIR_INVALID_ID \
    UINT64_C(0)

#define VITTE_HIR_DEFAULT_INITIAL_CAPACITY \
    ((size_t)16u)

#define VITTE_HIR_DEFAULT_MAX_NODES \
    ((size_t)16777216u)

#define VITTE_HIR_DEFAULT_MAX_SYMBOLS \
    ((size_t)4194304u)

#define VITTE_HIR_DEFAULT_MAX_TYPES \
    ((size_t)1048576u)

#define VITTE_HIR_DEFAULT_MAX_STRING_BYTES \
    ((size_t)(256u * 1024u * 1024u))

#define VITTE_HIR_MAX_RECURSION_DEPTH \
    ((size_t)4096u)

/* ========================================================================= */
/* IDs                                                                       */
/* ========================================================================= */

typedef uint64_t vitte_hir_id_t;
typedef uint64_t vitte_hir_node_id_t;
typedef uint64_t vitte_hir_type_id_t;
typedef uint64_t vitte_hir_symbol_id_t;
typedef uint64_t vitte_hir_scope_id_t;
typedef uint64_t vitte_hir_block_id_t;
typedef uint64_t vitte_hir_string_id_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_hir_error {
    VITTE_HIR_ERROR_NONE = 0,

    VITTE_HIR_ERROR_INVALID_ARGUMENT,
    VITTE_HIR_ERROR_INVALID_CONTEXT,
    VITTE_HIR_ERROR_INVALID_STATE,

    VITTE_HIR_ERROR_INVALID_NODE,
    VITTE_HIR_ERROR_INVALID_NODE_KIND,
    VITTE_HIR_ERROR_INVALID_TYPE,
    VITTE_HIR_ERROR_INVALID_SYMBOL,
    VITTE_HIR_ERROR_INVALID_SCOPE,
    VITTE_HIR_ERROR_INVALID_BLOCK,
    VITTE_HIR_ERROR_INVALID_STRING,

    VITTE_HIR_ERROR_TYPE_MISMATCH,
    VITTE_HIR_ERROR_DUPLICATE_SYMBOL,
    VITTE_HIR_ERROR_UNRESOLVED_SYMBOL,

    VITTE_HIR_ERROR_TOO_MANY_NODES,
    VITTE_HIR_ERROR_TOO_MANY_SYMBOLS,
    VITTE_HIR_ERROR_TOO_MANY_TYPES,
    VITTE_HIR_ERROR_STRING_LIMIT,
    VITTE_HIR_ERROR_RECURSION_LIMIT,

    VITTE_HIR_ERROR_OVERFLOW,
    VITTE_HIR_ERROR_OUT_OF_MEMORY,

    VITTE_HIR_ERROR_VALIDATION,
    VITTE_HIR_ERROR_CORRUPTION,
    VITTE_HIR_ERROR_UNSUPPORTED,
    VITTE_HIR_ERROR_INTERNAL,

    VITTE_HIR_ERROR_COUNT
} vitte_hir_error_t;

/* ========================================================================= */
/* Context state                                                             */
/* ========================================================================= */

typedef enum vitte_hir_state {
    VITTE_HIR_STATE_INVALID = 0,

    VITTE_HIR_STATE_BUILDING,
    VITTE_HIR_STATE_SEALED,
    VITTE_HIR_STATE_VALIDATED,
    VITTE_HIR_STATE_DESTROYED,

    VITTE_HIR_STATE_COUNT
} vitte_hir_state_t;

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

typedef struct vitte_hir_span {
    uint32_t file_id;

    size_t begin;
    size_t end;

    bool valid;
} vitte_hir_span_t;

/* ========================================================================= */
/* Visibility                                                                */
/* ========================================================================= */

typedef enum vitte_hir_visibility {
    VITTE_HIR_VISIBILITY_PRIVATE = 0,
    VITTE_HIR_VISIBILITY_PUBLIC,

    VITTE_HIR_VISIBILITY_COUNT
} vitte_hir_visibility_t;

/* ========================================================================= */
/* Mutability                                                                */
/* ========================================================================= */

typedef enum vitte_hir_mutability {
    VITTE_HIR_MUTABILITY_IMMUTABLE = 0,
    VITTE_HIR_MUTABILITY_MUTABLE,

    VITTE_HIR_MUTABILITY_COUNT
} vitte_hir_mutability_t;

/* ========================================================================= */
/* Type kinds                                                                */
/* ========================================================================= */

typedef enum vitte_hir_type_kind {
    VITTE_HIR_TYPE_INVALID = 0,

    VITTE_HIR_TYPE_UNIT,
    VITTE_HIR_TYPE_NEVER,

    VITTE_HIR_TYPE_BOOL,
    VITTE_HIR_TYPE_CHAR,

    VITTE_HIR_TYPE_I8,
    VITTE_HIR_TYPE_I16,
    VITTE_HIR_TYPE_I32,
    VITTE_HIR_TYPE_I64,

    VITTE_HIR_TYPE_U8,
    VITTE_HIR_TYPE_U16,
    VITTE_HIR_TYPE_U32,
    VITTE_HIR_TYPE_U64,

    VITTE_HIR_TYPE_F32,
    VITTE_HIR_TYPE_F64,

    VITTE_HIR_TYPE_STRING,

    VITTE_HIR_TYPE_POINTER,
    VITTE_HIR_TYPE_REFERENCE,

    VITTE_HIR_TYPE_ARRAY,
    VITTE_HIR_TYPE_SLICE,

    VITTE_HIR_TYPE_TUPLE,

    VITTE_HIR_TYPE_FUNCTION,

    VITTE_HIR_TYPE_NAMED,
    VITTE_HIR_TYPE_GENERIC_PARAMETER,

    VITTE_HIR_TYPE_DYN_TRAIT,

    VITTE_HIR_TYPE_COUNT
} vitte_hir_type_kind_t;

/* ========================================================================= */
/* Node kinds                                                                */
/* ========================================================================= */

typedef enum vitte_hir_node_kind {
    VITTE_HIR_NODE_INVALID = 0,

    /* --------------------------------------------------------------------- */
    /* Compilation structure                                                 */
    /* --------------------------------------------------------------------- */

    VITTE_HIR_NODE_MODULE,
    VITTE_HIR_NODE_USE,

    /* --------------------------------------------------------------------- */
    /* Declarations                                                          */
    /* --------------------------------------------------------------------- */

    VITTE_HIR_NODE_CONST_DECL,
    VITTE_HIR_NODE_STATIC_DECL,
    VITTE_HIR_NODE_TYPE_DECL,
    VITTE_HIR_NODE_FORM_DECL,
    VITTE_HIR_NODE_PICK_DECL,
    VITTE_HIR_NODE_TRAIT_DECL,
    VITTE_HIR_NODE_IMPL_DECL,
    VITTE_HIR_NODE_PROC_DECL,
    VITTE_HIR_NODE_EXTERN_PROC_DECL,
    VITTE_HIR_NODE_MACRO_DECL,
    VITTE_HIR_NODE_TEST_DECL,

    /* --------------------------------------------------------------------- */
    /* Statements                                                            */
    /* --------------------------------------------------------------------- */

    VITTE_HIR_NODE_BLOCK,
    VITTE_HIR_NODE_LET,
    VITTE_HIR_NODE_ASSIGN,
    VITTE_HIR_NODE_EXPR_STMT,

    VITTE_HIR_NODE_RETURN,
    VITTE_HIR_NODE_DEFER,

    VITTE_HIR_NODE_IF,
    VITTE_HIR_NODE_WHILE,
    VITTE_HIR_NODE_LOOP,
    VITTE_HIR_NODE_FOR,

    VITTE_HIR_NODE_BREAK,
    VITTE_HIR_NODE_CONTINUE,

    VITTE_HIR_NODE_MATCH,

    VITTE_HIR_NODE_ASSERT,

    VITTE_HIR_NODE_UNSAFE,
    VITTE_HIR_NODE_ASM,

    /* --------------------------------------------------------------------- */
    /* Expressions                                                           */
    /* --------------------------------------------------------------------- */

    VITTE_HIR_NODE_LITERAL,
    VITTE_HIR_NODE_SYMBOL,
    VITTE_HIR_NODE_UNARY,
    VITTE_HIR_NODE_BINARY,
    VITTE_HIR_NODE_CALL,

    VITTE_HIR_NODE_MEMBER,
    VITTE_HIR_NODE_INDEX,

    VITTE_HIR_NODE_CAST,

    VITTE_HIR_NODE_TUPLE,
    VITTE_HIR_NODE_ARRAY,

    VITTE_HIR_NODE_FORM_INIT,

    VITTE_HIR_NODE_REF,
    VITTE_HIR_NODE_DEREF,
    VITTE_HIR_NODE_MOVE,

    VITTE_HIR_NODE_AWAIT,

    VITTE_HIR_NODE_SIZEOF,
    VITTE_HIR_NODE_ALIGNOF,
    VITTE_HIR_NODE_OFFSETOF,
    VITTE_HIR_NODE_TYPEOF,

    /* --------------------------------------------------------------------- */
    /* Patterns                                                              */
    /* --------------------------------------------------------------------- */

    VITTE_HIR_NODE_PATTERN_WILDCARD,
    VITTE_HIR_NODE_PATTERN_LITERAL,
    VITTE_HIR_NODE_PATTERN_BINDING,
    VITTE_HIR_NODE_PATTERN_TUPLE,
    VITTE_HIR_NODE_PATTERN_PICK,

    VITTE_HIR_NODE_COUNT
} vitte_hir_node_kind_t;

/* ========================================================================= */
/* Unary operators                                                           */
/* ========================================================================= */

typedef enum vitte_hir_unary_operator {
    VITTE_HIR_UNARY_INVALID = 0,

    VITTE_HIR_UNARY_PLUS,
    VITTE_HIR_UNARY_NEGATE,
    VITTE_HIR_UNARY_NOT,
    VITTE_HIR_UNARY_BIT_NOT,

    VITTE_HIR_UNARY_COUNT
} vitte_hir_unary_operator_t;

/* ========================================================================= */
/* Binary operators                                                          */
/* ========================================================================= */

typedef enum vitte_hir_binary_operator {
    VITTE_HIR_BINARY_INVALID = 0,

    VITTE_HIR_BINARY_ADD,
    VITTE_HIR_BINARY_SUBTRACT,
    VITTE_HIR_BINARY_MULTIPLY,
    VITTE_HIR_BINARY_DIVIDE,
    VITTE_HIR_BINARY_REMAINDER,

    VITTE_HIR_BINARY_BIT_AND,
    VITTE_HIR_BINARY_BIT_OR,
    VITTE_HIR_BINARY_BIT_XOR,

    VITTE_HIR_BINARY_SHIFT_LEFT,
    VITTE_HIR_BINARY_SHIFT_RIGHT,

    VITTE_HIR_BINARY_EQUAL,
    VITTE_HIR_BINARY_NOT_EQUAL,

    VITTE_HIR_BINARY_LESS,
    VITTE_HIR_BINARY_LESS_EQUAL,
    VITTE_HIR_BINARY_GREATER,
    VITTE_HIR_BINARY_GREATER_EQUAL,

    VITTE_HIR_BINARY_AND,
    VITTE_HIR_BINARY_OR,

    VITTE_HIR_BINARY_COUNT
} vitte_hir_binary_operator_t;

/* ========================================================================= */
/* Literal kinds                                                             */
/* ========================================================================= */

typedef enum vitte_hir_literal_kind {
    VITTE_HIR_LITERAL_INVALID = 0,

    VITTE_HIR_LITERAL_BOOL,

    VITTE_HIR_LITERAL_SIGNED_INTEGER,
    VITTE_HIR_LITERAL_UNSIGNED_INTEGER,

    VITTE_HIR_LITERAL_F32,
    VITTE_HIR_LITERAL_F64,

    VITTE_HIR_LITERAL_CHAR,
    VITTE_HIR_LITERAL_STRING,

    VITTE_HIR_LITERAL_NULL,

    VITTE_HIR_LITERAL_COUNT
} vitte_hir_literal_kind_t;

/* ========================================================================= */
/* Symbol kinds                                                              */
/* ========================================================================= */

typedef enum vitte_hir_symbol_kind {
    VITTE_HIR_SYMBOL_INVALID = 0,

    VITTE_HIR_SYMBOL_MODULE,

    VITTE_HIR_SYMBOL_CONST,
    VITTE_HIR_SYMBOL_STATIC,

    VITTE_HIR_SYMBOL_TYPE,
    VITTE_HIR_SYMBOL_FORM,
    VITTE_HIR_SYMBOL_PICK,
    VITTE_HIR_SYMBOL_TRAIT,

    VITTE_HIR_SYMBOL_PROC,
    VITTE_HIR_SYMBOL_PARAMETER,
    VITTE_HIR_SYMBOL_LOCAL,

    VITTE_HIR_SYMBOL_GENERIC_PARAMETER,

    VITTE_HIR_SYMBOL_COUNT
} vitte_hir_symbol_kind_t;

/* ========================================================================= */
/* Type                                                                      */
/* ========================================================================= */

typedef struct vitte_hir_type {
    vitte_hir_type_id_t id;
    vitte_hir_type_kind_t kind;

    vitte_hir_symbol_id_t symbol;

    vitte_hir_type_id_t element_type;

    size_t array_length;

    vitte_hir_type_id_t *parameters;
    size_t parameter_count;

    vitte_hir_type_id_t return_type;

    bool mutable_reference;
    bool variadic;
} vitte_hir_type_t;

/* ========================================================================= */
/* String                                                                    */
/* ========================================================================= */

typedef struct vitte_hir_string {
    vitte_hir_string_id_t id;

    char *data;
    size_t length;

    uint64_t hash;
} vitte_hir_string_t;

/* ========================================================================= */
/* Literal                                                                   */
/* ========================================================================= */

typedef struct vitte_hir_literal {
    vitte_hir_literal_kind_t kind;

    union {
        bool boolean;

        int64_t signed_integer;
        uint64_t unsigned_integer;

        float f32;
        double f64;

        uint32_t character;

        vitte_hir_string_id_t string;
    } as;
} vitte_hir_literal_t;

/* ========================================================================= */
/* Node list                                                                 */
/* ========================================================================= */

typedef struct vitte_hir_node_list {
    vitte_hir_node_id_t *items;
    size_t count;
    size_t capacity;
} vitte_hir_node_list_t;

/* ========================================================================= */
/* Type list                                                                 */
/* ========================================================================= */

typedef struct vitte_hir_type_list {
    vitte_hir_type_id_t *items;
    size_t count;
    size_t capacity;
} vitte_hir_type_list_t;

/* ========================================================================= */
/* Match arm                                                                 */
/* ========================================================================= */

typedef struct vitte_hir_match_arm {
    vitte_hir_node_id_t pattern;
    vitte_hir_node_id_t guard;
    vitte_hir_node_id_t body;

    vitte_hir_span_t span;
} vitte_hir_match_arm_t;

/* ========================================================================= */
/* Match arm list                                                            */
/* ========================================================================= */

typedef struct vitte_hir_match_arm_list {
    vitte_hir_match_arm_t *items;
    size_t count;
    size_t capacity;
} vitte_hir_match_arm_list_t;

/* ========================================================================= */
/* Node                                                                      */
/* ========================================================================= */

typedef struct vitte_hir_node {
    vitte_hir_node_id_t id;

    vitte_hir_node_kind_t kind;
    vitte_hir_type_id_t type;

    vitte_hir_span_t span;

    union {
        struct {
            vitte_hir_string_id_t name;
            vitte_hir_node_list_t items;
        } module;

        struct {
            vitte_hir_string_id_t path;
        } use_decl;

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_node_id_t value;
            vitte_hir_visibility_t visibility;
        } constant;

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_node_id_t value;
            vitte_hir_visibility_t visibility;
            vitte_hir_mutability_t mutability;
        } static_decl;

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_type_id_t aliased_type;
            vitte_hir_visibility_t visibility;
        } type_decl;

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_node_list_t fields;
            vitte_hir_visibility_t visibility;
        } aggregate_decl;

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_node_list_t members;
            vitte_hir_visibility_t visibility;
        } trait_decl;

        struct {
            vitte_hir_type_id_t target_type;
            vitte_hir_symbol_id_t trait_symbol;
            vitte_hir_node_list_t members;
        } impl_decl;

        struct {
            vitte_hir_symbol_id_t symbol;

            vitte_hir_node_list_t parameters;

            vitte_hir_type_id_t return_type;
            vitte_hir_node_id_t body;

            vitte_hir_visibility_t visibility;

            bool is_async;
            bool is_unsafe;
            bool is_const;
            bool is_inline;
            bool is_noinline;
            bool is_extern;
            bool is_variadic;
        } procedure;

        struct {
            vitte_hir_node_list_t statements;
            vitte_hir_node_id_t tail;
        } block;

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_type_id_t declared_type;
            vitte_hir_node_id_t initializer;
            vitte_hir_mutability_t mutability;
        } let_stmt;

        struct {
            vitte_hir_node_id_t target;
            vitte_hir_node_id_t value;
        } assignment;

        struct {
            vitte_hir_node_id_t expression;
        } expression_statement;

        struct {
            vitte_hir_node_id_t value;
        } return_stmt;

        struct {
            vitte_hir_node_id_t body;
        } defer_stmt;

        struct {
            vitte_hir_node_id_t condition;
            vitte_hir_node_id_t then_branch;
            vitte_hir_node_id_t else_branch;
        } if_expr;

        struct {
            vitte_hir_node_id_t condition;
            vitte_hir_node_id_t body;
        } while_stmt;

        struct {
            vitte_hir_node_id_t body;
        } loop_stmt;

        struct {
            vitte_hir_node_id_t pattern;
            vitte_hir_node_id_t iterator;
            vitte_hir_node_id_t body;
        } for_stmt;

        struct {
            vitte_hir_node_id_t value;
        } break_stmt;

        struct {
            vitte_hir_node_id_t value;
        } assert_stmt;

        struct {
            vitte_hir_node_id_t value;
            vitte_hir_match_arm_list_t arms;
        } match_expr;

        struct {
            vitte_hir_literal_t value;
        } literal;

        struct {
            vitte_hir_symbol_id_t symbol;
        } symbol_ref;

        struct {
            vitte_hir_unary_operator_t op;
            vitte_hir_node_id_t operand;
        } unary;

        struct {
            vitte_hir_binary_operator_t op;
            vitte_hir_node_id_t left;
            vitte_hir_node_id_t right;
        } binary;

        struct {
            vitte_hir_node_id_t callee;
            vitte_hir_node_list_t arguments;
        } call;

        struct {
            vitte_hir_node_id_t object;
            vitte_hir_symbol_id_t member;
        } member;

        struct {
            vitte_hir_node_id_t object;
            vitte_hir_node_id_t index;
        } index;

        struct {
            vitte_hir_node_id_t expression;
            vitte_hir_type_id_t destination_type;
        } cast;

        struct {
            vitte_hir_node_list_t elements;
        } aggregate;

        struct {
            vitte_hir_type_id_t form_type;
            vitte_hir_node_list_t fields;
        } form_init;

        struct {
            vitte_hir_node_id_t expression;
            bool mutable_reference;
        } reference;

        struct {
            vitte_hir_node_id_t expression;
        } unary_expression;

        struct {
            vitte_hir_type_id_t queried_type;
        } type_query;

        struct {
            vitte_hir_type_id_t queried_type;
            vitte_hir_symbol_id_t member;
        } offset_query;

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_mutability_t mutability;
        } binding_pattern;

        struct {
            vitte_hir_node_list_t elements;
        } tuple_pattern;

        struct {
            vitte_hir_symbol_id_t variant;
            vitte_hir_node_list_t fields;
        } pick_pattern;
    } as;
} vitte_hir_node_t;

/* ========================================================================= */
/* Symbol                                                                    */
/* ========================================================================= */

typedef struct vitte_hir_symbol {
    vitte_hir_symbol_id_t id;

    vitte_hir_symbol_kind_t kind;

    vitte_hir_string_id_t name;
    vitte_hir_type_id_t type;

    vitte_hir_scope_id_t scope;
    vitte_hir_node_id_t declaration;

    vitte_hir_visibility_t visibility;
    vitte_hir_mutability_t mutability;

    vitte_hir_span_t span;
} vitte_hir_symbol_t;

/* ========================================================================= */
/* Scope                                                                     */
/* ========================================================================= */

typedef struct vitte_hir_scope {
    vitte_hir_scope_id_t id;
    vitte_hir_scope_id_t parent;

    vitte_hir_symbol_id_t *symbols;
    size_t symbol_count;
    size_t symbol_capacity;
} vitte_hir_scope_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_hir_stats {
    uint64_t nodes_created;
    uint64_t types_created;
    uint64_t symbols_created;
    uint64_t scopes_created;
    uint64_t strings_interned;

    uint64_t string_bytes;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;
} vitte_hir_stats_t;

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

typedef struct vitte_hir {
    uint64_t magic;

    vitte_hir_state_t state;
    vitte_hir_error_t last_error;

    vitte_hir_node_t *nodes;
    size_t node_count;
    size_t node_capacity;

    vitte_hir_type_t *types;
    size_t type_count;
    size_t type_capacity;

    vitte_hir_symbol_t *symbols;
    size_t symbol_count;
    size_t symbol_capacity;

    vitte_hir_scope_t *scopes;
    size_t scope_count;
    size_t scope_capacity;

    vitte_hir_string_t *strings;
    size_t string_count;
    size_t string_capacity;
    size_t string_bytes;

    vitte_hir_node_list_t roots;

    vitte_hir_stats_t stats;

    size_t max_nodes;
    size_t max_symbols;
    size_t max_types;
    size_t max_string_bytes;

    uint64_t generation;
} vitte_hir_t;

/* ========================================================================= */
/* Public prototypes                                                         */
/* ========================================================================= */

const char *
vitte_hir_error_name(vitte_hir_error_t error);

const char *
vitte_hir_state_name(vitte_hir_state_t state);

const char *
vitte_hir_type_kind_name(vitte_hir_type_kind_t kind);

const char *
vitte_hir_node_kind_name(vitte_hir_node_kind_t kind);

const char *
vitte_hir_symbol_kind_name(vitte_hir_symbol_kind_t kind);

const char *
vitte_hir_unary_operator_name(vitte_hir_unary_operator_t op);

const char *
vitte_hir_binary_operator_name(vitte_hir_binary_operator_t op);

bool
vitte_hir_init(vitte_hir_t *hir);

void
vitte_hir_destroy(vitte_hir_t *hir);

bool
vitte_hir_reset(vitte_hir_t *hir);

bool
vitte_hir_is_valid(const vitte_hir_t *hir);

bool
vitte_hir_seal(vitte_hir_t *hir);

bool
vitte_hir_validate(vitte_hir_t *hir);

vitte_hir_string_id_t
vitte_hir_intern_string(
    vitte_hir_t *hir,
    const char *data,
    size_t length);

const vitte_hir_string_t *
vitte_hir_get_string(
    const vitte_hir_t *hir,
    vitte_hir_string_id_t id);

vitte_hir_type_id_t
vitte_hir_add_type(
    vitte_hir_t *hir,
    const vitte_hir_type_t *type);

const vitte_hir_type_t *
vitte_hir_get_type(
    const vitte_hir_t *hir,
    vitte_hir_type_id_t id);

vitte_hir_symbol_id_t
vitte_hir_add_symbol(
    vitte_hir_t *hir,
    const vitte_hir_symbol_t *symbol);

const vitte_hir_symbol_t *
vitte_hir_get_symbol(
    const vitte_hir_t *hir,
    vitte_hir_symbol_id_t id);

vitte_hir_scope_id_t
vitte_hir_add_scope(
    vitte_hir_t *hir,
    vitte_hir_scope_id_t parent);

const vitte_hir_scope_t *
vitte_hir_get_scope(
    const vitte_hir_t *hir,
    vitte_hir_scope_id_t id);

bool
vitte_hir_scope_add_symbol(
    vitte_hir_t *hir,
    vitte_hir_scope_id_t scope,
    vitte_hir_symbol_id_t symbol);

vitte_hir_symbol_id_t
vitte_hir_scope_lookup(
    const vitte_hir_t *hir,
    vitte_hir_scope_id_t scope,
    vitte_hir_string_id_t name);

vitte_hir_node_id_t
vitte_hir_add_node(
    vitte_hir_t *hir,
    const vitte_hir_node_t *node);

const vitte_hir_node_t *
vitte_hir_get_node(
    const vitte_hir_t *hir,
    vitte_hir_node_id_t id);

vitte_hir_node_t *
vitte_hir_get_node_mut(
    vitte_hir_t *hir,
    vitte_hir_node_id_t id);

bool
vitte_hir_add_root(
    vitte_hir_t *hir,
    vitte_hir_node_id_t node);

uint64_t
vitte_hir_fingerprint(vitte_hir_t *hir);

vitte_hir_stats_t
vitte_hir_stats(const vitte_hir_t *hir);

/* ========================================================================= */
/* Checked arithmetic                                                        */
/* ========================================================================= */

static bool
vitte_hir_size_add(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left > SIZE_MAX - right) {
        return false;
    }

    *result = left + right;
    return true;
}

static bool
vitte_hir_size_mul(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left != 0u && right > SIZE_MAX / left) {
        return false;
    }

    *result = left * right;
    return true;
}

/* ========================================================================= */
/* Saturating statistics                                                     */
/* ========================================================================= */

static void
vitte_hir_u64_increment(uint64_t *value)
{
    if (value != NULL && *value != UINT64_MAX) {
        ++(*value);
    }
}

static void
vitte_hir_u64_add(
    uint64_t *destination,
    uint64_t value)
{
    if (destination == NULL) {
        return;
    }

    if (*destination > UINT64_MAX - value) {
        *destination = UINT64_MAX;
        return;
    }

    *destination += value;
}

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

static uint64_t
vitte_hir_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned index;

    for (index = 0u; index < 8u; ++index) {
        hash ^=
            (value >> (index * 8u)) &
            UINT64_C(0xff);

        hash *= VITTE_HIR_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_hir_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    if (data == NULL && length != 0u) {
        return hash;
    }

    bytes = (const unsigned char *)data;

    for (index = 0u; index < length; ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_HIR_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_hir_string_hash(
    const char *data,
    size_t length)
{
    uint64_t hash;

    hash = VITTE_HIR_FNV_OFFSET;
    hash = vitte_hir_hash_u64(hash, (uint64_t)length);

    return vitte_hir_hash_bytes(hash, data, length);
}

/* ========================================================================= */
/* Error handling                                                            */
/* ========================================================================= */

static bool
vitte_hir_fail(
    vitte_hir_t *hir,
    vitte_hir_error_t error)
{
    if (hir != NULL &&
        hir->magic == VITTE_HIR_MAGIC) {
        hir->last_error = error;
    }

    return false;
}

static vitte_hir_id_t
vitte_hir_fail_id(
    vitte_hir_t *hir,
    vitte_hir_error_t error)
{
    (void)vitte_hir_fail(hir, error);
    return VITTE_HIR_INVALID_ID;
}

/* ========================================================================= */
/* Enum validation                                                          */
/* ========================================================================= */

static bool
vitte_hir_type_kind_is_valid(
    vitte_hir_type_kind_t kind)
{
    return
        kind > VITTE_HIR_TYPE_INVALID &&
        kind < VITTE_HIR_TYPE_COUNT;
}

static bool
vitte_hir_node_kind_is_valid(
    vitte_hir_node_kind_t kind)
{
    return
        kind > VITTE_HIR_NODE_INVALID &&
        kind < VITTE_HIR_NODE_COUNT;
}

static bool
vitte_hir_symbol_kind_is_valid(
    vitte_hir_symbol_kind_t kind)
{
    return
        kind > VITTE_HIR_SYMBOL_INVALID &&
        kind < VITTE_HIR_SYMBOL_COUNT;
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_hir_error_name(vitte_hir_error_t error)
{
    switch (error) {
        case VITTE_HIR_ERROR_NONE:
            return "none";

        case VITTE_HIR_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_HIR_ERROR_INVALID_CONTEXT:
            return "invalid-context";

        case VITTE_HIR_ERROR_INVALID_STATE:
            return "invalid-state";

        case VITTE_HIR_ERROR_INVALID_NODE:
            return "invalid-node";

        case VITTE_HIR_ERROR_INVALID_NODE_KIND:
            return "invalid-node-kind";

        case VITTE_HIR_ERROR_INVALID_TYPE:
            return "invalid-type";

        case VITTE_HIR_ERROR_INVALID_SYMBOL:
            return "invalid-symbol";

        case VITTE_HIR_ERROR_INVALID_SCOPE:
            return "invalid-scope";

        case VITTE_HIR_ERROR_INVALID_BLOCK:
            return "invalid-block";

        case VITTE_HIR_ERROR_INVALID_STRING:
            return "invalid-string";

        case VITTE_HIR_ERROR_TYPE_MISMATCH:
            return "type-mismatch";

        case VITTE_HIR_ERROR_DUPLICATE_SYMBOL:
            return "duplicate-symbol";

        case VITTE_HIR_ERROR_UNRESOLVED_SYMBOL:
            return "unresolved-symbol";

        case VITTE_HIR_ERROR_TOO_MANY_NODES:
            return "too-many-nodes";

        case VITTE_HIR_ERROR_TOO_MANY_SYMBOLS:
            return "too-many-symbols";

        case VITTE_HIR_ERROR_TOO_MANY_TYPES:
            return "too-many-types";

        case VITTE_HIR_ERROR_STRING_LIMIT:
            return "string-limit";

        case VITTE_HIR_ERROR_RECURSION_LIMIT:
            return "recursion-limit";

        case VITTE_HIR_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_HIR_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_HIR_ERROR_VALIDATION:
            return "validation";

        case VITTE_HIR_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_HIR_ERROR_UNSUPPORTED:
            return "unsupported";

        case VITTE_HIR_ERROR_INTERNAL:
            return "internal";

        case VITTE_HIR_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_hir_state_name(vitte_hir_state_t state)
{
    switch (state) {
        case VITTE_HIR_STATE_INVALID:
            return "invalid";

        case VITTE_HIR_STATE_BUILDING:
            return "building";

        case VITTE_HIR_STATE_SEALED:
            return "sealed";

        case VITTE_HIR_STATE_VALIDATED:
            return "validated";

        case VITTE_HIR_STATE_DESTROYED:
            return "destroyed";

        case VITTE_HIR_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_hir_type_kind_name(vitte_hir_type_kind_t kind)
{
    switch (kind) {
        case VITTE_HIR_TYPE_INVALID:
            return "invalid";
        case VITTE_HIR_TYPE_UNIT:
            return "unit";
        case VITTE_HIR_TYPE_NEVER:
            return "never";
        case VITTE_HIR_TYPE_BOOL:
            return "bool";
        case VITTE_HIR_TYPE_CHAR:
            return "char";
        case VITTE_HIR_TYPE_I8:
            return "i8";
        case VITTE_HIR_TYPE_I16:
            return "i16";
        case VITTE_HIR_TYPE_I32:
            return "i32";
        case VITTE_HIR_TYPE_I64:
            return "i64";
        case VITTE_HIR_TYPE_U8:
            return "u8";
        case VITTE_HIR_TYPE_U16:
            return "u16";
        case VITTE_HIR_TYPE_U32:
            return "u32";
        case VITTE_HIR_TYPE_U64:
            return "u64";
        case VITTE_HIR_TYPE_F32:
            return "f32";
        case VITTE_HIR_TYPE_F64:
            return "f64";
        case VITTE_HIR_TYPE_STRING:
            return "string";
        case VITTE_HIR_TYPE_POINTER:
            return "pointer";
        case VITTE_HIR_TYPE_REFERENCE:
            return "reference";
        case VITTE_HIR_TYPE_ARRAY:
            return "array";
        case VITTE_HIR_TYPE_SLICE:
            return "slice";
        case VITTE_HIR_TYPE_TUPLE:
            return "tuple";
        case VITTE_HIR_TYPE_FUNCTION:
            return "function";
        case VITTE_HIR_TYPE_NAMED:
            return "named";
        case VITTE_HIR_TYPE_GENERIC_PARAMETER:
            return "generic-parameter";
        case VITTE_HIR_TYPE_DYN_TRAIT:
            return "dyn-trait";
        case VITTE_HIR_TYPE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_hir_symbol_kind_name(vitte_hir_symbol_kind_t kind)
{
    switch (kind) {
        case VITTE_HIR_SYMBOL_INVALID:
            return "invalid";
        case VITTE_HIR_SYMBOL_MODULE:
            return "module";
        case VITTE_HIR_SYMBOL_CONST:
            return "const";
        case VITTE_HIR_SYMBOL_STATIC:
            return "static";
        case VITTE_HIR_SYMBOL_TYPE:
            return "type";
        case VITTE_HIR_SYMBOL_FORM:
            return "form";
        case VITTE_HIR_SYMBOL_PICK:
            return "pick";
        case VITTE_HIR_SYMBOL_TRAIT:
            return "trait";
        case VITTE_HIR_SYMBOL_PROC:
            return "proc";
        case VITTE_HIR_SYMBOL_PARAMETER:
            return "parameter";
        case VITTE_HIR_SYMBOL_LOCAL:
            return "local";
        case VITTE_HIR_SYMBOL_GENERIC_PARAMETER:
            return "generic-parameter";
        case VITTE_HIR_SYMBOL_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_hir_unary_operator_name(vitte_hir_unary_operator_t op)
{
    switch (op) {
        case VITTE_HIR_UNARY_INVALID:
            return "invalid";
        case VITTE_HIR_UNARY_PLUS:
            return "plus";
        case VITTE_HIR_UNARY_NEGATE:
            return "negate";
        case VITTE_HIR_UNARY_NOT:
            return "not";
        case VITTE_HIR_UNARY_BIT_NOT:
            return "bit-not";
        case VITTE_HIR_UNARY_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_hir_binary_operator_name(vitte_hir_binary_operator_t op)
{
    switch (op) {
        case VITTE_HIR_BINARY_INVALID:
            return "invalid";
        case VITTE_HIR_BINARY_ADD:
            return "add";
        case VITTE_HIR_BINARY_SUBTRACT:
            return "subtract";
        case VITTE_HIR_BINARY_MULTIPLY:
            return "multiply";
        case VITTE_HIR_BINARY_DIVIDE:
            return "divide";
        case VITTE_HIR_BINARY_REMAINDER:
            return "remainder";
        case VITTE_HIR_BINARY_BIT_AND:
            return "bit-and";
        case VITTE_HIR_BINARY_BIT_OR:
            return "bit-or";
        case VITTE_HIR_BINARY_BIT_XOR:
            return "bit-xor";
        case VITTE_HIR_BINARY_SHIFT_LEFT:
            return "shift-left";
        case VITTE_HIR_BINARY_SHIFT_RIGHT:
            return "shift-right";
        case VITTE_HIR_BINARY_EQUAL:
            return "equal";
        case VITTE_HIR_BINARY_NOT_EQUAL:
            return "not-equal";
        case VITTE_HIR_BINARY_LESS:
            return "less";
        case VITTE_HIR_BINARY_LESS_EQUAL:
            return "less-equal";
        case VITTE_HIR_BINARY_GREATER:
            return "greater";
        case VITTE_HIR_BINARY_GREATER_EQUAL:
            return "greater-equal";
        case VITTE_HIR_BINARY_AND:
            return "and";
        case VITTE_HIR_BINARY_OR:
            return "or";
        case VITTE_HIR_BINARY_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_hir_node_kind_name(vitte_hir_node_kind_t kind)
{
    switch (kind) {
        case VITTE_HIR_NODE_INVALID:
            return "invalid";
        case VITTE_HIR_NODE_MODULE:
            return "module";
        case VITTE_HIR_NODE_USE:
            return "use";
        case VITTE_HIR_NODE_CONST_DECL:
            return "const-decl";
        case VITTE_HIR_NODE_STATIC_DECL:
            return "static-decl";
        case VITTE_HIR_NODE_TYPE_DECL:
            return "type-decl";
        case VITTE_HIR_NODE_FORM_DECL:
            return "form-decl";
        case VITTE_HIR_NODE_PICK_DECL:
            return "pick-decl";
        case VITTE_HIR_NODE_TRAIT_DECL:
            return "trait-decl";
        case VITTE_HIR_NODE_IMPL_DECL:
            return "impl-decl";
        case VITTE_HIR_NODE_PROC_DECL:
            return "proc-decl";
        case VITTE_HIR_NODE_EXTERN_PROC_DECL:
            return "extern-proc-decl";
        case VITTE_HIR_NODE_MACRO_DECL:
            return "macro-decl";
        case VITTE_HIR_NODE_TEST_DECL:
            return "test-decl";
        case VITTE_HIR_NODE_BLOCK:
            return "block";
        case VITTE_HIR_NODE_LET:
            return "let";
        case VITTE_HIR_NODE_ASSIGN:
            return "assign";
        case VITTE_HIR_NODE_EXPR_STMT:
            return "expr-stmt";
        case VITTE_HIR_NODE_RETURN:
            return "return";
        case VITTE_HIR_NODE_DEFER:
            return "defer";
        case VITTE_HIR_NODE_IF:
            return "if";
        case VITTE_HIR_NODE_WHILE:
            return "while";
        case VITTE_HIR_NODE_LOOP:
            return "loop";
        case VITTE_HIR_NODE_FOR:
            return "for";
        case VITTE_HIR_NODE_BREAK:
            return "break";
        case VITTE_HIR_NODE_CONTINUE:
            return "continue";
        case VITTE_HIR_NODE_MATCH:
            return "match";
        case VITTE_HIR_NODE_ASSERT:
            return "assert";
        case VITTE_HIR_NODE_UNSAFE:
            return "unsafe";
        case VITTE_HIR_NODE_ASM:
            return "asm";
        case VITTE_HIR_NODE_LITERAL:
            return "literal";
        case VITTE_HIR_NODE_SYMBOL:
            return "symbol";
        case VITTE_HIR_NODE_UNARY:
            return "unary";
        case VITTE_HIR_NODE_BINARY:
            return "binary";
        case VITTE_HIR_NODE_CALL:
            return "call";
        case VITTE_HIR_NODE_MEMBER:
            return "member";
        case VITTE_HIR_NODE_INDEX:
            return "index";
        case VITTE_HIR_NODE_CAST:
            return "cast";
        case VITTE_HIR_NODE_TUPLE:
            return "tuple";
        case VITTE_HIR_NODE_ARRAY:
            return "array";
        case VITTE_HIR_NODE_FORM_INIT:
            return "form-init";
        case VITTE_HIR_NODE_REF:
            return "ref";
        case VITTE_HIR_NODE_DEREF:
            return "deref";
        case VITTE_HIR_NODE_MOVE:
            return "move";
        case VITTE_HIR_NODE_AWAIT:
            return "await";
        case VITTE_HIR_NODE_SIZEOF:
            return "sizeof";
        case VITTE_HIR_NODE_ALIGNOF:
            return "alignof";
        case VITTE_HIR_NODE_OFFSETOF:
            return "offsetof";
        case VITTE_HIR_NODE_TYPEOF:
            return "typeof";
        case VITTE_HIR_NODE_PATTERN_WILDCARD:
            return "pattern-wildcard";
        case VITTE_HIR_NODE_PATTERN_LITERAL:
            return "pattern-literal";
        case VITTE_HIR_NODE_PATTERN_BINDING:
            return "pattern-binding";
        case VITTE_HIR_NODE_PATTERN_TUPLE:
            return "pattern-tuple";
        case VITTE_HIR_NODE_PATTERN_PICK:
            return "pattern-pick";
        case VITTE_HIR_NODE_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Generic reserve                                                           */
/* ========================================================================= */

static bool
vitte_hir_reserve(
    vitte_hir_t *hir,
    void **memory,
    size_t element_size,
    size_t *capacity,
    size_t required,
    size_t maximum)
{
    size_t new_capacity;
    size_t bytes;
    void *new_memory;

    if (hir == NULL ||
        memory == NULL ||
        capacity == NULL ||
        element_size == 0u) {
        return false;
    }

    if (required <= *capacity) {
        return true;
    }

    if (required > maximum) {
        return false;
    }

    new_capacity =
        *capacity == 0u
            ? VITTE_HIR_DEFAULT_INITIAL_CAPACITY
            : *capacity;

    while (new_capacity < required) {
        size_t next;

        if (!vitte_hir_size_add(
                new_capacity,
                new_capacity,
                &next)) {
            new_capacity = required;
            break;
        }

        if (next > maximum) {
            new_capacity = maximum;
            break;
        }

        new_capacity = next;
    }

    if (new_capacity < required) {
        return false;
    }

    if (!vitte_hir_size_mul(
            new_capacity,
            element_size,
            &bytes)) {
        return false;
    }

    new_memory = realloc(*memory, bytes);

    if (new_memory == NULL) {
        vitte_hir_u64_increment(
            &hir->stats.allocation_failures);

        return false;
    }

    if (*memory == NULL) {
        vitte_hir_u64_increment(
            &hir->stats.allocations);
    } else {
        vitte_hir_u64_increment(
            &hir->stats.reallocations);
    }

    *memory = new_memory;
    *capacity = new_capacity;

    return true;
}

/* ========================================================================= */
/* Node list                                                                 */
/* ========================================================================= */

static void
vitte_hir_node_list_destroy(
    vitte_hir_node_list_t *list)
{
    if (list == NULL) {
        return;
    }

    free(list->items);

    list->items = NULL;
    list->count = 0u;
    list->capacity = 0u;
}

static bool
vitte_hir_node_list_push(
    vitte_hir_t *hir,
    vitte_hir_node_list_t *list,
    vitte_hir_node_id_t node)
{
    size_t required;

    if (hir == NULL ||
        list == NULL ||
        node == VITTE_HIR_INVALID_ID) {
        return false;
    }

    if (!vitte_hir_size_add(
            list->count,
            1u,
            &required)) {
        return false;
    }

    if (!vitte_hir_reserve(
            hir,
            (void **)&list->items,
            sizeof(*list->items),
            &list->capacity,
            required,
            hir->max_nodes)) {
        return false;
    }

    list->items[list->count] = node;
    ++list->count;

    return true;
}

/* ========================================================================= */
/* Match list destruction                                                    */
/* ========================================================================= */

static void
vitte_hir_match_arm_list_destroy(
    vitte_hir_match_arm_list_t *list)
{
    if (list == NULL) {
        return;
    }

    free(list->items);

    list->items = NULL;
    list->count = 0u;
    list->capacity = 0u;
}

/* ========================================================================= */
/* Type destruction                                                          */
/* ========================================================================= */

static void
vitte_hir_type_destroy(vitte_hir_type_t *type)
{
    if (type == NULL) {
        return;
    }

    free(type->parameters);

    type->parameters = NULL;
    type->parameter_count = 0u;
}

/* ========================================================================= */
/* Node destruction                                                          */
/* ========================================================================= */

static void
vitte_hir_node_destroy(vitte_hir_node_t *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->kind) {
        case VITTE_HIR_NODE_MODULE:
            vitte_hir_node_list_destroy(
                &node->as.module.items);
            break;

        case VITTE_HIR_NODE_FORM_DECL:
        case VITTE_HIR_NODE_PICK_DECL:
            vitte_hir_node_list_destroy(
                &node->as.aggregate_decl.fields);
            break;

        case VITTE_HIR_NODE_TRAIT_DECL:
            vitte_hir_node_list_destroy(
                &node->as.trait_decl.members);
            break;

        case VITTE_HIR_NODE_IMPL_DECL:
            vitte_hir_node_list_destroy(
                &node->as.impl_decl.members);
            break;

        case VITTE_HIR_NODE_PROC_DECL:
        case VITTE_HIR_NODE_EXTERN_PROC_DECL:
        case VITTE_HIR_NODE_TEST_DECL:
            vitte_hir_node_list_destroy(
                &node->as.procedure.parameters);
            break;

        case VITTE_HIR_NODE_BLOCK:
            vitte_hir_node_list_destroy(
                &node->as.block.statements);
            break;

        case VITTE_HIR_NODE_MATCH:
            vitte_hir_match_arm_list_destroy(
                &node->as.match_expr.arms);
            break;

        case VITTE_HIR_NODE_CALL:
            vitte_hir_node_list_destroy(
                &node->as.call.arguments);
            break;

        case VITTE_HIR_NODE_TUPLE:
        case VITTE_HIR_NODE_ARRAY:
            vitte_hir_node_list_destroy(
                &node->as.aggregate.elements);
            break;

        case VITTE_HIR_NODE_FORM_INIT:
            vitte_hir_node_list_destroy(
                &node->as.form_init.fields);
            break;

        case VITTE_HIR_NODE_PATTERN_TUPLE:
            vitte_hir_node_list_destroy(
                &node->as.tuple_pattern.elements);
            break;

        case VITTE_HIR_NODE_PATTERN_PICK:
            vitte_hir_node_list_destroy(
                &node->as.pick_pattern.fields);
            break;

        case VITTE_HIR_NODE_INVALID:
        case VITTE_HIR_NODE_USE:
        case VITTE_HIR_NODE_CONST_DECL:
        case VITTE_HIR_NODE_STATIC_DECL:
        case VITTE_HIR_NODE_TYPE_DECL:
        case VITTE_HIR_NODE_MACRO_DECL:
        case VITTE_HIR_NODE_LET:
        case VITTE_HIR_NODE_ASSIGN:
        case VITTE_HIR_NODE_EXPR_STMT:
        case VITTE_HIR_NODE_RETURN:
        case VITTE_HIR_NODE_DEFER:
        case VITTE_HIR_NODE_IF:
        case VITTE_HIR_NODE_WHILE:
        case VITTE_HIR_NODE_LOOP:
        case VITTE_HIR_NODE_FOR:
        case VITTE_HIR_NODE_BREAK:
        case VITTE_HIR_NODE_CONTINUE:
        case VITTE_HIR_NODE_ASSERT:
        case VITTE_HIR_NODE_UNSAFE:
        case VITTE_HIR_NODE_ASM:
        case VITTE_HIR_NODE_LITERAL:
        case VITTE_HIR_NODE_SYMBOL:
        case VITTE_HIR_NODE_UNARY:
        case VITTE_HIR_NODE_BINARY:
        case VITTE_HIR_NODE_MEMBER:
        case VITTE_HIR_NODE_INDEX:
        case VITTE_HIR_NODE_CAST:
        case VITTE_HIR_NODE_REF:
        case VITTE_HIR_NODE_DEREF:
        case VITTE_HIR_NODE_MOVE:
        case VITTE_HIR_NODE_AWAIT:
        case VITTE_HIR_NODE_SIZEOF:
        case VITTE_HIR_NODE_ALIGNOF:
        case VITTE_HIR_NODE_OFFSETOF:
        case VITTE_HIR_NODE_TYPEOF:
        case VITTE_HIR_NODE_PATTERN_WILDCARD:
        case VITTE_HIR_NODE_PATTERN_LITERAL:
        case VITTE_HIR_NODE_PATTERN_BINDING:
        case VITTE_HIR_NODE_COUNT:
            break;
    }
}

/* ========================================================================= */
/* Context lifecycle                                                         */
/* ========================================================================= */

bool
vitte_hir_init(vitte_hir_t *hir)
{
    if (hir == NULL) {
        return false;
    }

    (void)memset(hir, 0, sizeof(*hir));

    hir->magic = VITTE_HIR_MAGIC;
    hir->state = VITTE_HIR_STATE_BUILDING;
    hir->last_error = VITTE_HIR_ERROR_NONE;

    hir->max_nodes =
        VITTE_HIR_DEFAULT_MAX_NODES;

    hir->max_symbols =
        VITTE_HIR_DEFAULT_MAX_SYMBOLS;

    hir->max_types =
        VITTE_HIR_DEFAULT_MAX_TYPES;

    hir->max_string_bytes =
        VITTE_HIR_DEFAULT_MAX_STRING_BYTES;

    hir->generation = UINT64_C(1);

    return true;
}

void
vitte_hir_destroy(vitte_hir_t *hir)
{
    size_t index;

    if (hir == NULL) {
        return;
    }

    if (hir->magic != VITTE_HIR_MAGIC) {
        (void)memset(hir, 0, sizeof(*hir));
        hir->magic = VITTE_HIR_DEAD_MAGIC;
        hir->state = VITTE_HIR_STATE_DESTROYED;
        return;
    }

    for (index = 0u; index < hir->node_count; ++index) {
        vitte_hir_node_destroy(&hir->nodes[index]);
    }

    for (index = 0u; index < hir->type_count; ++index) {
        vitte_hir_type_destroy(&hir->types[index]);
    }

    for (index = 0u; index < hir->scope_count; ++index) {
        free(hir->scopes[index].symbols);
    }

    for (index = 0u; index < hir->string_count; ++index) {
        free(hir->strings[index].data);
    }

    free(hir->nodes);
    free(hir->types);
    free(hir->symbols);
    free(hir->scopes);
    free(hir->strings);

    vitte_hir_node_list_destroy(&hir->roots);

    (void)memset(hir, 0, sizeof(*hir));

    hir->magic = VITTE_HIR_DEAD_MAGIC;
    hir->state = VITTE_HIR_STATE_DESTROYED;
}

bool
vitte_hir_reset(vitte_hir_t *hir)
{
    uint64_t generation;

    if (!vitte_hir_is_valid(hir)) {
        return false;
    }

    generation = hir->generation;

    vitte_hir_destroy(hir);

    if (!vitte_hir_init(hir)) {
        return false;
    }

    if (generation != UINT64_MAX) {
        hir->generation = generation + UINT64_C(1);
    } else {
        hir->generation = generation;
    }

    return true;
}

bool
vitte_hir_is_valid(const vitte_hir_t *hir)
{
    if (hir == NULL) {
        return false;
    }

    if (hir->magic != VITTE_HIR_MAGIC) {
        return false;
    }

    if (hir->state <= VITTE_HIR_STATE_INVALID ||
        hir->state >= VITTE_HIR_STATE_DESTROYED) {
        return false;
    }

    if (hir->last_error >= VITTE_HIR_ERROR_COUNT) {
        return false;
    }

    if (hir->node_count > hir->node_capacity ||
        hir->type_count > hir->type_capacity ||
        hir->symbol_count > hir->symbol_capacity ||
        hir->scope_count > hir->scope_capacity ||
        hir->string_count > hir->string_capacity) {
        return false;
    }

    if (hir->node_count > hir->max_nodes ||
        hir->symbol_count > hir->max_symbols ||
        hir->type_count > hir->max_types ||
        hir->string_bytes > hir->max_string_bytes) {
        return false;
    }

    if (hir->node_capacity != 0u &&
        hir->nodes == NULL) {
        return false;
    }

    if (hir->type_capacity != 0u &&
        hir->types == NULL) {
        return false;
    }

    if (hir->symbol_capacity != 0u &&
        hir->symbols == NULL) {
        return false;
    }

    if (hir->scope_capacity != 0u &&
        hir->scopes == NULL) {
        return false;
    }

    if (hir->string_capacity != 0u &&
        hir->strings == NULL) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* String interning                                                          */
/* ========================================================================= */

vitte_hir_string_id_t
vitte_hir_intern_string(
    vitte_hir_t *hir,
    const char *data,
    size_t length)
{
    uint64_t hash;
    size_t index;
    size_t required_count;
    size_t required_bytes;
    char *copy;
    vitte_hir_string_t *string;

    if (!vitte_hir_is_valid(hir) ||
        hir->state != VITTE_HIR_STATE_BUILDING) {
        return VITTE_HIR_INVALID_ID;
    }

    if (data == NULL && length != 0u) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_INVALID_ARGUMENT);
    }

    hash = vitte_hir_string_hash(data, length);

    for (index = 0u; index < hir->string_count; ++index) {
        const vitte_hir_string_t *existing;

        existing = &hir->strings[index];

        if (existing->hash != hash ||
            existing->length != length) {
            continue;
        }

        if (length == 0u ||
            memcmp(existing->data, data, length) == 0) {
            return existing->id;
        }
    }

    if (!vitte_hir_size_add(
            hir->string_count,
            1u,
            &required_count)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OVERFLOW);
    }

    if (!vitte_hir_size_add(
            hir->string_bytes,
            length,
            &required_bytes)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OVERFLOW);
    }

    if (required_bytes > hir->max_string_bytes) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_STRING_LIMIT);
    }

    if (!vitte_hir_reserve(
            hir,
            (void **)&hir->strings,
            sizeof(*hir->strings),
            &hir->string_capacity,
            required_count,
            hir->max_symbols)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    if (length == SIZE_MAX) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OVERFLOW);
    }

    copy = (char *)malloc(length + 1u);

    if (copy == NULL) {
        vitte_hir_u64_increment(
            &hir->stats.allocation_failures);

        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    vitte_hir_u64_increment(
        &hir->stats.allocations);

    if (length != 0u) {
        (void)memcpy(copy, data, length);
    }

    copy[length] = '\0';

    string = &hir->strings[hir->string_count];

    (void)memset(string, 0, sizeof(*string));

    string->id =
        (vitte_hir_string_id_t)(
            hir->string_count + 1u);

    string->data = copy;
    string->length = length;
    string->hash = hash;

    ++hir->string_count;
    hir->string_bytes = required_bytes;

    vitte_hir_u64_increment(
        &hir->stats.strings_interned);

    vitte_hir_u64_add(
        &hir->stats.string_bytes,
        (uint64_t)length);

    return string->id;
}

const vitte_hir_string_t *
vitte_hir_get_string(
    const vitte_hir_t *hir,
    vitte_hir_string_id_t id)
{
    size_t index;

    if (!vitte_hir_is_valid(hir) ||
        id == VITTE_HIR_INVALID_ID) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= hir->string_count) {
        return NULL;
    }

    return &hir->strings[index];
}

/* ========================================================================= */
/* Types                                                                     */
/* ========================================================================= */

static bool
vitte_hir_clone_type(
    vitte_hir_t *hir,
    vitte_hir_type_t *destination,
    const vitte_hir_type_t *source)
{
    size_t bytes;

    if (hir == NULL ||
        destination == NULL ||
        source == NULL) {
        return false;
    }

    *destination = *source;

    destination->parameters = NULL;

    if (source->parameter_count == 0u) {
        return true;
    }

    if (source->parameters == NULL) {
        return false;
    }

    if (!vitte_hir_size_mul(
            source->parameter_count,
            sizeof(*source->parameters),
            &bytes)) {
        return false;
    }

    destination->parameters =
        (vitte_hir_type_id_t *)malloc(bytes);

    if (destination->parameters == NULL) {
        vitte_hir_u64_increment(
            &hir->stats.allocation_failures);

        return false;
    }

    vitte_hir_u64_increment(
        &hir->stats.allocations);

    (void)memcpy(
        destination->parameters,
        source->parameters,
        bytes);

    return true;
}

vitte_hir_type_id_t
vitte_hir_add_type(
    vitte_hir_t *hir,
    const vitte_hir_type_t *type)
{
    size_t required;
    vitte_hir_type_t copy;

    if (!vitte_hir_is_valid(hir) ||
        hir->state != VITTE_HIR_STATE_BUILDING ||
        type == NULL) {
        return VITTE_HIR_INVALID_ID;
    }

    if (!vitte_hir_type_kind_is_valid(type->kind)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_INVALID_TYPE);
    }

    if (hir->type_count >= hir->max_types) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_TOO_MANY_TYPES);
    }

    if (!vitte_hir_size_add(
            hir->type_count,
            1u,
            &required)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OVERFLOW);
    }

    if (!vitte_hir_reserve(
            hir,
            (void **)&hir->types,
            sizeof(*hir->types),
            &hir->type_capacity,
            required,
            hir->max_types)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    (void)memset(&copy, 0, sizeof(copy));

    if (!vitte_hir_clone_type(
            hir,
            &copy,
            type)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    copy.id =
        (vitte_hir_type_id_t)(
            hir->type_count + 1u);

    hir->types[hir->type_count] = copy;
    ++hir->type_count;

    vitte_hir_u64_increment(
        &hir->stats.types_created);

    return copy.id;
}

const vitte_hir_type_t *
vitte_hir_get_type(
    const vitte_hir_t *hir,
    vitte_hir_type_id_t id)
{
    size_t index;

    if (!vitte_hir_is_valid(hir) ||
        id == VITTE_HIR_INVALID_ID) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= hir->type_count) {
        return NULL;
    }

    return &hir->types[index];
}

/* ========================================================================= */
/* Symbols                                                                   */
/* ========================================================================= */

vitte_hir_symbol_id_t
vitte_hir_add_symbol(
    vitte_hir_t *hir,
    const vitte_hir_symbol_t *symbol)
{
    size_t required;
    vitte_hir_symbol_t copy;

    if (!vitte_hir_is_valid(hir) ||
        hir->state != VITTE_HIR_STATE_BUILDING ||
        symbol == NULL) {
        return VITTE_HIR_INVALID_ID;
    }

    if (!vitte_hir_symbol_kind_is_valid(symbol->kind)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_INVALID_SYMBOL);
    }

    if (symbol->name == VITTE_HIR_INVALID_ID ||
        vitte_hir_get_string(hir, symbol->name) == NULL) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_INVALID_STRING);
    }

    if (hir->symbol_count >= hir->max_symbols) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_TOO_MANY_SYMBOLS);
    }

    if (!vitte_hir_size_add(
            hir->symbol_count,
            1u,
            &required)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OVERFLOW);
    }

    if (!vitte_hir_reserve(
            hir,
            (void **)&hir->symbols,
            sizeof(*hir->symbols),
            &hir->symbol_capacity,
            required,
            hir->max_symbols)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    copy = *symbol;

    copy.id =
        (vitte_hir_symbol_id_t)(
            hir->symbol_count + 1u);

    hir->symbols[hir->symbol_count] = copy;
    ++hir->symbol_count;

    vitte_hir_u64_increment(
        &hir->stats.symbols_created);

    return copy.id;
}

const vitte_hir_symbol_t *
vitte_hir_get_symbol(
    const vitte_hir_t *hir,
    vitte_hir_symbol_id_t id)
{
    size_t index;

    if (!vitte_hir_is_valid(hir) ||
        id == VITTE_HIR_INVALID_ID) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= hir->symbol_count) {
        return NULL;
    }

    return &hir->symbols[index];
}

/* ========================================================================= */
/* Scopes                                                                    */
/* ========================================================================= */

vitte_hir_scope_id_t
vitte_hir_add_scope(
    vitte_hir_t *hir,
    vitte_hir_scope_id_t parent)
{
    size_t required;
    vitte_hir_scope_t *scope;

    if (!vitte_hir_is_valid(hir) ||
        hir->state != VITTE_HIR_STATE_BUILDING) {
        return VITTE_HIR_INVALID_ID;
    }

    if (parent != VITTE_HIR_INVALID_ID &&
        vitte_hir_get_scope(hir, parent) == NULL) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_INVALID_SCOPE);
    }

    if (!vitte_hir_size_add(
            hir->scope_count,
            1u,
            &required)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OVERFLOW);
    }

    if (!vitte_hir_reserve(
            hir,
            (void **)&hir->scopes,
            sizeof(*hir->scopes),
            &hir->scope_capacity,
            required,
            hir->max_symbols)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    scope = &hir->scopes[hir->scope_count];

    (void)memset(scope, 0, sizeof(*scope));

    scope->id =
        (vitte_hir_scope_id_t)(
            hir->scope_count + 1u);

    scope->parent = parent;

    ++hir->scope_count;

    vitte_hir_u64_increment(
        &hir->stats.scopes_created);

    return scope->id;
}

const vitte_hir_scope_t *
vitte_hir_get_scope(
    const vitte_hir_t *hir,
    vitte_hir_scope_id_t id)
{
    size_t index;

    if (!vitte_hir_is_valid(hir) ||
        id == VITTE_HIR_INVALID_ID) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= hir->scope_count) {
        return NULL;
    }

    return &hir->scopes[index];
}

bool
vitte_hir_scope_add_symbol(
    vitte_hir_t *hir,
    vitte_hir_scope_id_t scope_id,
    vitte_hir_symbol_id_t symbol_id)
{
    vitte_hir_scope_t *scope;
    const vitte_hir_symbol_t *symbol;
    size_t index;
    size_t required;

    if (!vitte_hir_is_valid(hir) ||
        hir->state != VITTE_HIR_STATE_BUILDING) {
        return false;
    }

    if (scope_id == VITTE_HIR_INVALID_ID ||
        symbol_id == VITTE_HIR_INVALID_ID) {
        return
            vitte_hir_fail(
                hir,
                VITTE_HIR_ERROR_INVALID_ARGUMENT);
    }

    if ((size_t)(scope_id - UINT64_C(1)) >=
        hir->scope_count) {
        return
            vitte_hir_fail(
                hir,
                VITTE_HIR_ERROR_INVALID_SCOPE);
    }

    scope =
        &hir->scopes[
            (size_t)(scope_id - UINT64_C(1))];

    symbol =
        vitte_hir_get_symbol(
            hir,
            symbol_id);

    if (symbol == NULL) {
        return
            vitte_hir_fail(
                hir,
                VITTE_HIR_ERROR_INVALID_SYMBOL);
    }

    for (index = 0u;
         index < scope->symbol_count;
         ++index) {
        const vitte_hir_symbol_t *existing;

        existing =
            vitte_hir_get_symbol(
                hir,
                scope->symbols[index]);

        if (existing != NULL &&
            existing->name == symbol->name) {
            return
                vitte_hir_fail(
                    hir,
                    VITTE_HIR_ERROR_DUPLICATE_SYMBOL);
        }
    }

    if (!vitte_hir_size_add(
            scope->symbol_count,
            1u,
            &required)) {
        return
            vitte_hir_fail(
                hir,
                VITTE_HIR_ERROR_OVERFLOW);
    }

    if (!vitte_hir_reserve(
            hir,
            (void **)&scope->symbols,
            sizeof(*scope->symbols),
            &scope->symbol_capacity,
            required,
            hir->max_symbols)) {
        return
            vitte_hir_fail(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    scope->symbols[scope->symbol_count] =
        symbol_id;

    ++scope->symbol_count;

    return true;
}

vitte_hir_symbol_id_t
vitte_hir_scope_lookup(
    const vitte_hir_t *hir,
    vitte_hir_scope_id_t scope_id,
    vitte_hir_string_id_t name)
{
    size_t depth;

    if (!vitte_hir_is_valid(hir) ||
        name == VITTE_HIR_INVALID_ID) {
        return VITTE_HIR_INVALID_ID;
    }

    depth = 0u;

    while (scope_id != VITTE_HIR_INVALID_ID) {
        const vitte_hir_scope_t *scope;
        size_t index;

        if (depth >= VITTE_HIR_MAX_RECURSION_DEPTH) {
            return VITTE_HIR_INVALID_ID;
        }

        scope =
            vitte_hir_get_scope(
                hir,
                scope_id);

        if (scope == NULL) {
            return VITTE_HIR_INVALID_ID;
        }

        for (index = scope->symbol_count;
             index != 0u;
             --index) {
            const vitte_hir_symbol_t *symbol;

            symbol =
                vitte_hir_get_symbol(
                    hir,
                    scope->symbols[index - 1u]);

            if (symbol != NULL &&
                symbol->name == name) {
                return symbol->id;
            }
        }

        scope_id = scope->parent;
        ++depth;
    }

    return VITTE_HIR_INVALID_ID;
}

/* ========================================================================= */
/* Node cloning                                                              */
/* ========================================================================= */

static bool
vitte_hir_clone_node_list(
    vitte_hir_t *hir,
    vitte_hir_node_list_t *destination,
    const vitte_hir_node_list_t *source)
{
    size_t bytes;

    if (hir == NULL ||
        destination == NULL ||
        source == NULL) {
        return false;
    }

    destination->items = NULL;
    destination->count = 0u;
    destination->capacity = 0u;

    if (source->count == 0u) {
        return true;
    }

    if (source->items == NULL) {
        return false;
    }

    if (!vitte_hir_size_mul(
            source->count,
            sizeof(*source->items),
            &bytes)) {
        return false;
    }

    destination->items =
        (vitte_hir_node_id_t *)malloc(bytes);

    if (destination->items == NULL) {
        vitte_hir_u64_increment(
            &hir->stats.allocation_failures);

        return false;
    }

    vitte_hir_u64_increment(
        &hir->stats.allocations);

    (void)memcpy(
        destination->items,
        source->items,
        bytes);

    destination->count = source->count;
    destination->capacity = source->count;

    return true;
}

static bool
vitte_hir_clone_match_arms(
    vitte_hir_t *hir,
    vitte_hir_match_arm_list_t *destination,
    const vitte_hir_match_arm_list_t *source)
{
    size_t bytes;

    if (hir == NULL ||
        destination == NULL ||
        source == NULL) {
        return false;
    }

    destination->items = NULL;
    destination->count = 0u;
    destination->capacity = 0u;

    if (source->count == 0u) {
        return true;
    }

    if (source->items == NULL) {
        return false;
    }

    if (!vitte_hir_size_mul(
            source->count,
            sizeof(*source->items),
            &bytes)) {
        return false;
    }

    destination->items =
        (vitte_hir_match_arm_t *)malloc(bytes);

    if (destination->items == NULL) {
        vitte_hir_u64_increment(
            &hir->stats.allocation_failures);

        return false;
    }

    vitte_hir_u64_increment(
        &hir->stats.allocations);

    (void)memcpy(
        destination->items,
        source->items,
        bytes);

    destination->count = source->count;
    destination->capacity = source->count;

    return true;
}

static bool
vitte_hir_clone_node(
    vitte_hir_t *hir,
    vitte_hir_node_t *destination,
    const vitte_hir_node_t *source)
{
    if (hir == NULL ||
        destination == NULL ||
        source == NULL) {
        return false;
    }

    *destination = *source;

    switch (source->kind) {
        case VITTE_HIR_NODE_MODULE:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.module.items,
                    &source->as.module.items);

        case VITTE_HIR_NODE_FORM_DECL:
        case VITTE_HIR_NODE_PICK_DECL:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.aggregate_decl.fields,
                    &source->as.aggregate_decl.fields);

        case VITTE_HIR_NODE_TRAIT_DECL:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.trait_decl.members,
                    &source->as.trait_decl.members);

        case VITTE_HIR_NODE_IMPL_DECL:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.impl_decl.members,
                    &source->as.impl_decl.members);

        case VITTE_HIR_NODE_PROC_DECL:
        case VITTE_HIR_NODE_EXTERN_PROC_DECL:
        case VITTE_HIR_NODE_TEST_DECL:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.procedure.parameters,
                    &source->as.procedure.parameters);

        case VITTE_HIR_NODE_BLOCK:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.block.statements,
                    &source->as.block.statements);

        case VITTE_HIR_NODE_MATCH:
            return
                vitte_hir_clone_match_arms(
                    hir,
                    &destination->as.match_expr.arms,
                    &source->as.match_expr.arms);

        case VITTE_HIR_NODE_CALL:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.call.arguments,
                    &source->as.call.arguments);

        case VITTE_HIR_NODE_TUPLE:
        case VITTE_HIR_NODE_ARRAY:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.aggregate.elements,
                    &source->as.aggregate.elements);

        case VITTE_HIR_NODE_FORM_INIT:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.form_init.fields,
                    &source->as.form_init.fields);

        case VITTE_HIR_NODE_PATTERN_TUPLE:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.tuple_pattern.elements,
                    &source->as.tuple_pattern.elements);

        case VITTE_HIR_NODE_PATTERN_PICK:
            return
                vitte_hir_clone_node_list(
                    hir,
                    &destination->as.pick_pattern.fields,
                    &source->as.pick_pattern.fields);

        case VITTE_HIR_NODE_INVALID:
        case VITTE_HIR_NODE_USE:
        case VITTE_HIR_NODE_CONST_DECL:
        case VITTE_HIR_NODE_STATIC_DECL:
        case VITTE_HIR_NODE_TYPE_DECL:
        case VITTE_HIR_NODE_MACRO_DECL:
        case VITTE_HIR_NODE_LET:
        case VITTE_HIR_NODE_ASSIGN:
        case VITTE_HIR_NODE_EXPR_STMT:
        case VITTE_HIR_NODE_RETURN:
        case VITTE_HIR_NODE_DEFER:
        case VITTE_HIR_NODE_IF:
        case VITTE_HIR_NODE_WHILE:
        case VITTE_HIR_NODE_LOOP:
        case VITTE_HIR_NODE_FOR:
        case VITTE_HIR_NODE_BREAK:
        case VITTE_HIR_NODE_CONTINUE:
        case VITTE_HIR_NODE_ASSERT:
        case VITTE_HIR_NODE_UNSAFE:
        case VITTE_HIR_NODE_ASM:
        case VITTE_HIR_NODE_LITERAL:
        case VITTE_HIR_NODE_SYMBOL:
        case VITTE_HIR_NODE_UNARY:
        case VITTE_HIR_NODE_BINARY:
        case VITTE_HIR_NODE_MEMBER:
        case VITTE_HIR_NODE_INDEX:
        case VITTE_HIR_NODE_CAST:
        case VITTE_HIR_NODE_REF:
        case VITTE_HIR_NODE_DEREF:
        case VITTE_HIR_NODE_MOVE:
        case VITTE_HIR_NODE_AWAIT:
        case VITTE_HIR_NODE_SIZEOF:
        case VITTE_HIR_NODE_ALIGNOF:
        case VITTE_HIR_NODE_OFFSETOF:
        case VITTE_HIR_NODE_TYPEOF:
        case VITTE_HIR_NODE_PATTERN_WILDCARD:
        case VITTE_HIR_NODE_PATTERN_LITERAL:
        case VITTE_HIR_NODE_PATTERN_BINDING:
        case VITTE_HIR_NODE_COUNT:
            return true;
    }

    return false;
}

/* ========================================================================= */
/* Nodes                                                                     */
/* ========================================================================= */

vitte_hir_node_id_t
vitte_hir_add_node(
    vitte_hir_t *hir,
    const vitte_hir_node_t *node)
{
    size_t required;
    vitte_hir_node_t copy;

    if (!vitte_hir_is_valid(hir) ||
        hir->state != VITTE_HIR_STATE_BUILDING ||
        node == NULL) {
        return VITTE_HIR_INVALID_ID;
    }

    if (!vitte_hir_node_kind_is_valid(node->kind)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_INVALID_NODE_KIND);
    }

    if (hir->node_count >= hir->max_nodes) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_TOO_MANY_NODES);
    }

    if (!vitte_hir_size_add(
            hir->node_count,
            1u,
            &required)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OVERFLOW);
    }

    if (!vitte_hir_reserve(
            hir,
            (void **)&hir->nodes,
            sizeof(*hir->nodes),
            &hir->node_capacity,
            required,
            hir->max_nodes)) {
        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    (void)memset(&copy, 0, sizeof(copy));

    if (!vitte_hir_clone_node(
            hir,
            &copy,
            node)) {
        vitte_hir_node_destroy(&copy);

        return
            vitte_hir_fail_id(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    copy.id =
        (vitte_hir_node_id_t)(
            hir->node_count + 1u);

    hir->nodes[hir->node_count] = copy;
    ++hir->node_count;

    vitte_hir_u64_increment(
        &hir->stats.nodes_created);

    return copy.id;
}

const vitte_hir_node_t *
vitte_hir_get_node(
    const vitte_hir_t *hir,
    vitte_hir_node_id_t id)
{
    size_t index;

    if (!vitte_hir_is_valid(hir) ||
        id == VITTE_HIR_INVALID_ID) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= hir->node_count) {
        return NULL;
    }

    return &hir->nodes[index];
}

vitte_hir_node_t *
vitte_hir_get_node_mut(
    vitte_hir_t *hir,
    vitte_hir_node_id_t id)
{
    size_t index;

    if (!vitte_hir_is_valid(hir) ||
        hir->state != VITTE_HIR_STATE_BUILDING ||
        id == VITTE_HIR_INVALID_ID) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= hir->node_count) {
        return NULL;
    }

    return &hir->nodes[index];
}

bool
vitte_hir_add_root(
    vitte_hir_t *hir,
    vitte_hir_node_id_t node)
{
    if (!vitte_hir_is_valid(hir) ||
        hir->state != VITTE_HIR_STATE_BUILDING) {
        return false;
    }

    if (vitte_hir_get_node(hir, node) == NULL) {
        return
            vitte_hir_fail(
                hir,
                VITTE_HIR_ERROR_INVALID_NODE);
    }

    if (!vitte_hir_node_list_push(
            hir,
            &hir->roots,
            node)) {
        return
            vitte_hir_fail(
                hir,
                VITTE_HIR_ERROR_OUT_OF_MEMORY);
    }

    return true;
}

/* ========================================================================= */
/* Validation helpers                                                        */
/* ========================================================================= */

static bool
vitte_hir_validate_span(
    const vitte_hir_span_t *span)
{
    if (span == NULL) {
        return false;
    }

    if (!span->valid) {
        return true;
    }

    return span->begin <= span->end;
}

static bool
vitte_hir_validate_node_id(
    const vitte_hir_t *hir,
    vitte_hir_node_id_t id,
    bool optional)
{
    if (id == VITTE_HIR_INVALID_ID) {
        return optional;
    }

    return vitte_hir_get_node(hir, id) != NULL;
}

static bool
vitte_hir_validate_type_id(
    const vitte_hir_t *hir,
    vitte_hir_type_id_t id,
    bool optional)
{
    if (id == VITTE_HIR_INVALID_ID) {
        return optional;
    }

    return vitte_hir_get_type(hir, id) != NULL;
}

static bool
vitte_hir_validate_symbol_id(
    const vitte_hir_t *hir,
    vitte_hir_symbol_id_t id,
    bool optional)
{
    if (id == VITTE_HIR_INVALID_ID) {
        return optional;
    }

    return vitte_hir_get_symbol(hir, id) != NULL;
}

static bool
vitte_hir_validate_node_list(
    const vitte_hir_t *hir,
    const vitte_hir_node_list_t *list)
{
    size_t index;

    if (hir == NULL || list == NULL) {
        return false;
    }

    if (list->count > list->capacity) {
        return false;
    }

    if (list->count != 0u &&
        list->items == NULL) {
        return false;
    }

    for (index = 0u; index < list->count; ++index) {
        if (!vitte_hir_validate_node_id(
                hir,
                list->items[index],
                false)) {
            return false;
        }
    }

    return true;
}

static bool
vitte_hir_validate_type(
    const vitte_hir_t *hir,
    const vitte_hir_type_t *type)
{
    size_t index;

    if (hir == NULL || type == NULL) {
        return false;
    }

    if (!vitte_hir_type_kind_is_valid(type->kind)) {
        return false;
    }

    if (!vitte_hir_validate_symbol_id(
            hir,
            type->symbol,
            true)) {
        return false;
    }

    if (!vitte_hir_validate_type_id(
            hir,
            type->element_type,
            true)) {
        return false;
    }

    if (!vitte_hir_validate_type_id(
            hir,
            type->return_type,
            true)) {
        return false;
    }

    if (type->parameter_count != 0u &&
        type->parameters == NULL) {
        return false;
    }

    for (index = 0u;
         index < type->parameter_count;
         ++index) {
        if (!vitte_hir_validate_type_id(
                hir,
                type->parameters[index],
                false)) {
            return false;
        }
    }

    return true;
}

static bool
vitte_hir_validate_symbol(
    const vitte_hir_t *hir,
    const vitte_hir_symbol_t *symbol)
{
    if (hir == NULL || symbol == NULL) {
        return false;
    }

    if (!vitte_hir_symbol_kind_is_valid(
            symbol->kind)) {
        return false;
    }

    if (vitte_hir_get_string(
            hir,
            symbol->name) == NULL) {
        return false;
    }

    if (!vitte_hir_validate_type_id(
            hir,
            symbol->type,
            true)) {
        return false;
    }

    if (symbol->scope != VITTE_HIR_INVALID_ID &&
        vitte_hir_get_scope(
            hir,
            symbol->scope) == NULL) {
        return false;
    }

    if (!vitte_hir_validate_node_id(
            hir,
            symbol->declaration,
            true)) {
        return false;
    }

    return
        vitte_hir_validate_span(
            &symbol->span);
}

static bool
vitte_hir_validate_scope(
    const vitte_hir_t *hir,
    const vitte_hir_scope_t *scope)
{
    size_t index;

    if (hir == NULL || scope == NULL) {
        return false;
    }

    if (scope->parent != VITTE_HIR_INVALID_ID &&
        vitte_hir_get_scope(
            hir,
            scope->parent) == NULL) {
        return false;
    }

    if (scope->symbol_count >
        scope->symbol_capacity) {
        return false;
    }

    if (scope->symbol_count != 0u &&
        scope->symbols == NULL) {
        return false;
    }

    for (index = 0u;
         index < scope->symbol_count;
         ++index) {
        if (!vitte_hir_validate_symbol_id(
                hir,
                scope->symbols[index],
                false)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Node validation                                                           */
/* ========================================================================= */

static bool
vitte_hir_validate_node(
    const vitte_hir_t *hir,
    const vitte_hir_node_t *node)
{
    if (hir == NULL || node == NULL) {
        return false;
    }

    if (!vitte_hir_node_kind_is_valid(
            node->kind)) {
        return false;
    }

    if (!vitte_hir_validate_span(
            &node->span)) {
        return false;
    }

    if (!vitte_hir_validate_type_id(
            hir,
            node->type,
            true)) {
        return false;
    }

    switch (node->kind) {
        case VITTE_HIR_NODE_MODULE:
            return
                vitte_hir_get_string(
                    hir,
                    node->as.module.name) != NULL &&
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.module.items);

        case VITTE_HIR_NODE_USE:
            return
                vitte_hir_get_string(
                    hir,
                    node->as.use_decl.path) != NULL;

        case VITTE_HIR_NODE_CONST_DECL:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.constant.symbol,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.constant.value,
                    false);

        case VITTE_HIR_NODE_STATIC_DECL:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.static_decl.symbol,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.static_decl.value,
                    true);

        case VITTE_HIR_NODE_TYPE_DECL:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.type_decl.symbol,
                    false) &&
                vitte_hir_validate_type_id(
                    hir,
                    node->as.type_decl.aliased_type,
                    false);

        case VITTE_HIR_NODE_FORM_DECL:
        case VITTE_HIR_NODE_PICK_DECL:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.aggregate_decl.symbol,
                    false) &&
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.aggregate_decl.fields);

        case VITTE_HIR_NODE_TRAIT_DECL:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.trait_decl.symbol,
                    false) &&
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.trait_decl.members);

        case VITTE_HIR_NODE_IMPL_DECL:
            return
                vitte_hir_validate_type_id(
                    hir,
                    node->as.impl_decl.target_type,
                    false) &&
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.impl_decl.trait_symbol,
                    true) &&
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.impl_decl.members);

        case VITTE_HIR_NODE_PROC_DECL:
        case VITTE_HIR_NODE_EXTERN_PROC_DECL:
        case VITTE_HIR_NODE_TEST_DECL:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.procedure.symbol,
                    false) &&
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.procedure.parameters) &&
                vitte_hir_validate_type_id(
                    hir,
                    node->as.procedure.return_type,
                    true) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.procedure.body,
                    node->kind ==
                        VITTE_HIR_NODE_EXTERN_PROC_DECL);

        case VITTE_HIR_NODE_BLOCK:
            return
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.block.statements) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.block.tail,
                    true);

        case VITTE_HIR_NODE_LET:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.let_stmt.symbol,
                    false) &&
                vitte_hir_validate_type_id(
                    hir,
                    node->as.let_stmt.declared_type,
                    true) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.let_stmt.initializer,
                    true);

        case VITTE_HIR_NODE_ASSIGN:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.assignment.target,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.assignment.value,
                    false);

        case VITTE_HIR_NODE_EXPR_STMT:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.expression_statement.expression,
                    false);

        case VITTE_HIR_NODE_RETURN:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.return_stmt.value,
                    true);

        case VITTE_HIR_NODE_DEFER:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.defer_stmt.body,
                    false);

        case VITTE_HIR_NODE_IF:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.if_expr.condition,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.if_expr.then_branch,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.if_expr.else_branch,
                    true);

        case VITTE_HIR_NODE_WHILE:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.while_stmt.condition,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.while_stmt.body,
                    false);

        case VITTE_HIR_NODE_LOOP:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.loop_stmt.body,
                    false);

        case VITTE_HIR_NODE_FOR:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.for_stmt.pattern,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.for_stmt.iterator,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.for_stmt.body,
                    false);

        case VITTE_HIR_NODE_BREAK:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.break_stmt.value,
                    true);

        case VITTE_HIR_NODE_CONTINUE:
            return true;

        case VITTE_HIR_NODE_MATCH:
        {
            size_t index;

            if (!vitte_hir_validate_node_id(
                    hir,
                    node->as.match_expr.value,
                    false)) {
                return false;
            }

            if (node->as.match_expr.arms.count >
                node->as.match_expr.arms.capacity) {
                return false;
            }

            if (node->as.match_expr.arms.count != 0u &&
                node->as.match_expr.arms.items == NULL) {
                return false;
            }

            for (index = 0u;
                 index < node->as.match_expr.arms.count;
                 ++index) {
                const vitte_hir_match_arm_t *arm;

                arm =
                    &node->as.match_expr.arms.items[index];

                if (!vitte_hir_validate_node_id(
                        hir,
                        arm->pattern,
                        false) ||
                    !vitte_hir_validate_node_id(
                        hir,
                        arm->guard,
                        true) ||
                    !vitte_hir_validate_node_id(
                        hir,
                        arm->body,
                        false) ||
                    !vitte_hir_validate_span(
                        &arm->span)) {
                    return false;
                }
            }

            return true;
        }

        case VITTE_HIR_NODE_ASSERT:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.assert_stmt.value,
                    false);

        case VITTE_HIR_NODE_LITERAL:
            return
                node->as.literal.value.kind >
                    VITTE_HIR_LITERAL_INVALID &&
                node->as.literal.value.kind <
                    VITTE_HIR_LITERAL_COUNT;

        case VITTE_HIR_NODE_SYMBOL:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.symbol_ref.symbol,
                    false);

        case VITTE_HIR_NODE_UNARY:
            return
                node->as.unary.op >
                    VITTE_HIR_UNARY_INVALID &&
                node->as.unary.op <
                    VITTE_HIR_UNARY_COUNT &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.unary.operand,
                    false);

        case VITTE_HIR_NODE_BINARY:
            return
                node->as.binary.op >
                    VITTE_HIR_BINARY_INVALID &&
                node->as.binary.op <
                    VITTE_HIR_BINARY_COUNT &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.binary.left,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.binary.right,
                    false);

        case VITTE_HIR_NODE_CALL:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.call.callee,
                    false) &&
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.call.arguments);

        case VITTE_HIR_NODE_MEMBER:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.member.object,
                    false) &&
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.member.member,
                    false);

        case VITTE_HIR_NODE_INDEX:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.index.object,
                    false) &&
                vitte_hir_validate_node_id(
                    hir,
                    node->as.index.index,
                    false);

        case VITTE_HIR_NODE_CAST:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.cast.expression,
                    false) &&
                vitte_hir_validate_type_id(
                    hir,
                    node->as.cast.destination_type,
                    false);

        case VITTE_HIR_NODE_TUPLE:
        case VITTE_HIR_NODE_ARRAY:
            return
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.aggregate.elements);

        case VITTE_HIR_NODE_FORM_INIT:
            return
                vitte_hir_validate_type_id(
                    hir,
                    node->as.form_init.form_type,
                    false) &&
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.form_init.fields);

        case VITTE_HIR_NODE_REF:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.reference.expression,
                    false);

        case VITTE_HIR_NODE_DEREF:
        case VITTE_HIR_NODE_MOVE:
        case VITTE_HIR_NODE_AWAIT:
            return
                vitte_hir_validate_node_id(
                    hir,
                    node->as.unary_expression.expression,
                    false);

        case VITTE_HIR_NODE_SIZEOF:
        case VITTE_HIR_NODE_ALIGNOF:
        case VITTE_HIR_NODE_TYPEOF:
            return
                vitte_hir_validate_type_id(
                    hir,
                    node->as.type_query.queried_type,
                    false);

        case VITTE_HIR_NODE_OFFSETOF:
            return
                vitte_hir_validate_type_id(
                    hir,
                    node->as.offset_query.queried_type,
                    false) &&
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.offset_query.member,
                    false);

        case VITTE_HIR_NODE_PATTERN_WILDCARD:
            return true;

        case VITTE_HIR_NODE_PATTERN_LITERAL:
            return
                node->as.literal.value.kind >
                    VITTE_HIR_LITERAL_INVALID &&
                node->as.literal.value.kind <
                    VITTE_HIR_LITERAL_COUNT;

        case VITTE_HIR_NODE_PATTERN_BINDING:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.binding_pattern.symbol,
                    false);

        case VITTE_HIR_NODE_PATTERN_TUPLE:
            return
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.tuple_pattern.elements);

        case VITTE_HIR_NODE_PATTERN_PICK:
            return
                vitte_hir_validate_symbol_id(
                    hir,
                    node->as.pick_pattern.variant,
                    false) &&
                vitte_hir_validate_node_list(
                    hir,
                    &node->as.pick_pattern.fields);

        case VITTE_HIR_NODE_UNSAFE:
        case VITTE_HIR_NODE_ASM:
        case VITTE_HIR_NODE_MACRO_DECL:
            /*
             * Representation is intentionally subsystem-defined for now.
             * The core validates their common node envelope.
             */
            return true;

        case VITTE_HIR_NODE_INVALID:
        case VITTE_HIR_NODE_COUNT:
            return false;
    }

    return false;
}

/* ========================================================================= */
/* Full validation                                                           */
/* ========================================================================= */

bool
vitte_hir_validate(vitte_hir_t *hir)
{
    size_t index;

    if (!vitte_hir_is_valid(hir)) {
        return false;
    }

    vitte_hir_u64_increment(
        &hir->stats.validation_runs);

    for (index = 0u; index < hir->string_count; ++index) {
        const vitte_hir_string_t *string;

        string = &hir->strings[index];

        if (string->id !=
                (vitte_hir_string_id_t)(index + 1u) ||
            string->data == NULL ||
            string->hash !=
                vitte_hir_string_hash(
                    string->data,
                    string->length)) {
            goto failure;
        }
    }

    for (index = 0u; index < hir->type_count; ++index) {
        if (hir->types[index].id !=
                (vitte_hir_type_id_t)(index + 1u) ||
            !vitte_hir_validate_type(
                hir,
                &hir->types[index])) {
            goto failure;
        }
    }

    for (index = 0u; index < hir->symbol_count; ++index) {
        if (hir->symbols[index].id !=
                (vitte_hir_symbol_id_t)(index + 1u) ||
            !vitte_hir_validate_symbol(
                hir,
                &hir->symbols[index])) {
            goto failure;
        }
    }

    for (index = 0u; index < hir->scope_count; ++index) {
        if (hir->scopes[index].id !=
                (vitte_hir_scope_id_t)(index + 1u) ||
            !vitte_hir_validate_scope(
                hir,
                &hir->scopes[index])) {
            goto failure;
        }
    }

    for (index = 0u; index < hir->node_count; ++index) {
        if (hir->nodes[index].id !=
                (vitte_hir_node_id_t)(index + 1u) ||
            !vitte_hir_validate_node(
                hir,
                &hir->nodes[index])) {
            goto failure;
        }
    }

    if (!vitte_hir_validate_node_list(
            hir,
            &hir->roots)) {
        goto failure;
    }

    hir->last_error = VITTE_HIR_ERROR_NONE;

    if (hir->state == VITTE_HIR_STATE_SEALED) {
        hir->state = VITTE_HIR_STATE_VALIDATED;
    }

    return true;

failure:

    vitte_hir_u64_increment(
        &hir->stats.validation_failures);

    hir->last_error =
        VITTE_HIR_ERROR_VALIDATION;

    return false;
}

/* ========================================================================= */
/* Seal                                                                      */
/* ========================================================================= */

bool
vitte_hir_seal(vitte_hir_t *hir)
{
    if (!vitte_hir_is_valid(hir)) {
        return false;
    }

    if (hir->state != VITTE_HIR_STATE_BUILDING) {
        return
            vitte_hir_fail(
                hir,
                VITTE_HIR_ERROR_INVALID_STATE);
    }

    hir->state = VITTE_HIR_STATE_SEALED;

    if (!vitte_hir_validate(hir)) {
        hir->state = VITTE_HIR_STATE_SEALED;
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_hir_fingerprint(vitte_hir_t *hir)
{
    uint64_t hash;
    size_t index;

    if (!vitte_hir_is_valid(hir)) {
        return UINT64_C(0);
    }

    vitte_hir_u64_increment(
        &hir->stats.hash_runs);

    hash = VITTE_HIR_FNV_OFFSET;

    hash =
        vitte_hir_hash_u64(
            hash,
            (uint64_t)hir->node_count);

    hash =
        vitte_hir_hash_u64(
            hash,
            (uint64_t)hir->type_count);

    hash =
        vitte_hir_hash_u64(
            hash,
            (uint64_t)hir->symbol_count);

    hash =
        vitte_hir_hash_u64(
            hash,
            (uint64_t)hir->scope_count);

    hash =
        vitte_hir_hash_u64(
            hash,
            (uint64_t)hir->string_count);

    for (index = 0u; index < hir->strings[index].id &&
                     index < hir->string_count; ++index) {
        const vitte_hir_string_t *string;

        string = &hir->strings[index];

        hash =
            vitte_hir_hash_u64(
                hash,
                string->id);

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)string->length);

        hash =
            vitte_hir_hash_bytes(
                hash,
                string->data,
                string->length);
    }

    for (index = 0u; index < hir->type_count; ++index) {
        const vitte_hir_type_t *type;
        size_t parameter_index;

        type = &hir->types[index];

        hash =
            vitte_hir_hash_u64(
                hash,
                type->id);

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)type->kind);

        hash =
            vitte_hir_hash_u64(
                hash,
                type->symbol);

        hash =
            vitte_hir_hash_u64(
                hash,
                type->element_type);

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)type->array_length);

        hash =
            vitte_hir_hash_u64(
                hash,
                type->return_type);

        hash =
            vitte_hir_hash_u64(
                hash,
                type->mutable_reference
                    ? UINT64_C(1)
                    : UINT64_C(0));

        hash =
            vitte_hir_hash_u64(
                hash,
                type->variadic
                    ? UINT64_C(1)
                    : UINT64_C(0));

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)type->parameter_count);

        for (parameter_index = 0u;
             parameter_index < type->parameter_count;
             ++parameter_index) {
            hash =
                vitte_hir_hash_u64(
                    hash,
                    type->parameters[parameter_index]);
        }
    }

    for (index = 0u; index < hir->symbols[index].id &&
                     index < hir->symbol_count; ++index) {
        const vitte_hir_symbol_t *symbol;

        symbol = &hir->symbols[index];

        hash =
            vitte_hir_hash_u64(
                hash,
                symbol->id);

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)symbol->kind);

        hash =
            vitte_hir_hash_u64(
                hash,
                symbol->name);

        hash =
            vitte_hir_hash_u64(
                hash,
                symbol->type);

        hash =
            vitte_hir_hash_u64(
                hash,
                symbol->scope);

        hash =
            vitte_hir_hash_u64(
                hash,
                symbol->declaration);
    }

    for (index = 0u; index < hir->node_count; ++index) {
        const vitte_hir_node_t *node;

        node = &hir->nodes[index];

        hash =
            vitte_hir_hash_u64(
                hash,
                node->id);

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)node->kind);

        hash =
            vitte_hir_hash_u64(
                hash,
                node->type);

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)node->span.file_id);

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)node->span.begin);

        hash =
            vitte_hir_hash_u64(
                hash,
                (uint64_t)node->span.end);
    }

    hash =
        vitte_hir_hash_u64(
            hash,
            (uint64_t)hir->roots.count);

    for (index = 0u;
         index < hir->roots.count;
         ++index) {
        hash =
            vitte_hir_hash_u64(
                hash,
                hir->roots.items[index]);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_hir_stats_t
vitte_hir_stats(const vitte_hir_t *hir)
{
    vitte_hir_stats_t stats;

    (void)memset(&stats, 0, sizeof(stats));

    if (!vitte_hir_is_valid(hir)) {
        return stats;
    }

    return hir->stats;
}

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    VITTE_HIR_INVALID_ID == UINT64_C(0),
    "HIR zero ID must remain invalid");

_Static_assert(
    VITTE_HIR_DEFAULT_INITIAL_CAPACITY > 0u,
    "HIR initial capacity must be non-zero");

_Static_assert(
    VITTE_HIR_DEFAULT_MAX_NODES > 0u,
    "HIR node limit must be non-zero");

_Static_assert(
    VITTE_HIR_DEFAULT_MAX_SYMBOLS > 0u,
    "HIR symbol limit must be non-zero");

_Static_assert(
    VITTE_HIR_DEFAULT_MAX_TYPES > 0u,
    "HIR type limit must be non-zero");

_Static_assert(
    VITTE_HIR_DEFAULT_MAX_STRING_BYTES > 0u,
    "HIR string limit must be non-zero");

_Static_assert(
    VITTE_HIR_MAX_RECURSION_DEPTH > 0u,
    "HIR recursion limit must be non-zero");

_Static_assert(
    VITTE_HIR_ERROR_COUNT >
        VITTE_HIR_ERROR_INTERNAL,
    "HIR error enum invariant");

_Static_assert(
    VITTE_HIR_STATE_COUNT >
        VITTE_HIR_STATE_DESTROYED,
    "HIR state enum invariant");

_Static_assert(
    VITTE_HIR_TYPE_COUNT >
        VITTE_HIR_TYPE_DYN_TRAIT,
    "HIR type enum invariant");

_Static_assert(
    VITTE_HIR_NODE_COUNT >
        VITTE_HIR_NODE_PATTERN_PICK,
    "HIR node enum invariant");

_Static_assert(
    VITTE_HIR_SYMBOL_COUNT >
        VITTE_HIR_SYMBOL_GENERIC_PARAMETER,
    "HIR symbol enum invariant");
