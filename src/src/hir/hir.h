#ifndef VITTE_HIR_HIR_H
#define VITTE_HIR_HIR_H

/*
 * Vitte Compiler
 * src/hir/hir.h
 *
 * High-Level Intermediate Representation (HIR)
 * ============================================
 *
 * Public HIR API.
 *
 * The HIR is the compiler's typed, semantically resolved high-level
 * representation between the frontend semantic phases and lower IR.
 *
 * Typical pipeline:
 *
 *   source
 *      |
 *      v
 *   lexer
 *      |
 *      v
 *   parser / AST
 *      |
 *      v
 *   name resolution
 *      |
 *      v
 *   type checking
 *      |
 *      v
 *   constant folding / semantic contracts
 *      |
 *      v
 *   +-----------------------------+
 *   |             HIR             |
 *   +-----------------------------+
 *      |
 *      v
 *   canonicalization / optimization
 *      |
 *      v
 *   lower IR
 *      |
 *      v
 *   backend
 *
 * HIR preserves high-level constructs such as:
 *
 *   - declarations;
 *   - lexical scopes;
 *   - resolved symbols;
 *   - semantic types;
 *   - blocks;
 *   - if/while/loop/for;
 *   - match;
 *   - defer;
 *   - typed expressions;
 *   - calls;
 *   - aggregates;
 *   - patterns;
 *   - source provenance.
 *
 * Design principles
 * -----------------
 *
 *   - zero is the invalid ID for every HIR ID namespace;
 *   - IDs are stable for the lifetime of one HIR generation;
 *   - construction is deterministic;
 *   - all owned dynamic storage is released by vitte_hir_destroy();
 *   - public add functions copy their input structures;
 *   - dynamic lists embedded in added nodes/types are deep-copied;
 *   - mutation is allowed only while the HIR is BUILDING;
 *   - sealing performs structural validation;
 *   - validated/sealed HIR is read-only;
 *   - no backend-specific representation belongs in this API.
 *
 * Ownership
 * ---------
 *
 * vitte_hir_t owns:
 *
 *   - nodes;
 *   - types;
 *   - symbols;
 *   - scopes;
 *   - interned strings;
 *   - root lists;
 *   - deep-copied lists stored in nodes and types.
 *
 * The caller must eventually call:
 *
 *     vitte_hir_destroy(&hir);
 *
 * String interning copies source bytes.
 *
 * Structures passed to vitte_hir_add_node() and vitte_hir_add_type() remain
 * owned by the caller. The HIR creates its own copies.
 *
 * Threading
 * ---------
 *
 * A single vitte_hir_t is not internally synchronized.
 *
 * Independent HIR instances may be used concurrently when the caller's other
 * compiler state is also independent.
 *
 * C standard: ISO C17.
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

#define VITTE_HIR_API_VERSION_MAJOR 1u
#define VITTE_HIR_API_VERSION_MINOR 0u
#define VITTE_HIR_API_VERSION_PATCH 0u

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

/*
 * IDs are 1-based.
 *
 * Zero is always invalid/absent.
 *
 * The separate typedefs document semantic namespaces even though their
 * underlying representation is currently uint64_t.
 */

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
/* State                                                                     */
/* ========================================================================= */

typedef enum vitte_hir_state {
    VITTE_HIR_STATE_INVALID = 0,

    /*
     * Nodes/types/symbols/scopes/strings may be added.
     */
    VITTE_HIR_STATE_BUILDING,

    /*
     * Construction has ended.
     *
     * This state may exist transiently while seal validation is running.
     */
    VITTE_HIR_STATE_SEALED,

    /*
     * Structurally validated and read-only.
     */
    VITTE_HIR_STATE_VALIDATED,

    /*
     * Storage has been released.
     */
    VITTE_HIR_STATE_DESTROYED,

    VITTE_HIR_STATE_COUNT
} vitte_hir_state_t;

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

/*
 * Half-open byte interval:
 *
 *     [begin, end)
 *
 * file_id is owned/interpreted by the compiler source manager.
 */
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
/* Types                                                                     */
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

    /* Compilation structure. */
    VITTE_HIR_NODE_MODULE,
    VITTE_HIR_NODE_USE,

    /* Declarations. */
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

    /* Statements / control flow. */
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

    /* Expressions. */
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

    /* Patterns. */
    VITTE_HIR_NODE_PATTERN_WILDCARD,
    VITTE_HIR_NODE_PATTERN_LITERAL,
    VITTE_HIR_NODE_PATTERN_BINDING,
    VITTE_HIR_NODE_PATTERN_TUPLE,
    VITTE_HIR_NODE_PATTERN_PICK,

    VITTE_HIR_NODE_COUNT
} vitte_hir_node_kind_t;

/* ========================================================================= */
/* Operators                                                                 */
/* ========================================================================= */

typedef enum vitte_hir_unary_operator {
    VITTE_HIR_UNARY_INVALID = 0,

    VITTE_HIR_UNARY_PLUS,
    VITTE_HIR_UNARY_NEGATE,
    VITTE_HIR_UNARY_NOT,
    VITTE_HIR_UNARY_BIT_NOT,

    VITTE_HIR_UNARY_COUNT
} vitte_hir_unary_operator_t;

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
/* Literals                                                                  */
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
/* Symbols                                                                   */
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
/* Type representation                                                       */
/* ========================================================================= */

/*
 * One structure represents all HIR type kinds.
 *
 * Fields that are not meaningful for a particular kind remain zero.
 *
 * Examples:
 *
 * pointer/reference:
 *     element_type
 *
 * array:
 *     element_type
 *     array_length
 *
 * slice:
 *     element_type
 *
 * tuple:
 *     parameters[]
 *
 * function:
 *     parameters[]
 *     return_type
 *     variadic
 *
 * named:
 *     symbol
 *
 * generic parameter:
 *     symbol
 */
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
/* Interned strings                                                          */
/* ========================================================================= */

typedef struct vitte_hir_string {
    vitte_hir_string_id_t id;

    /*
     * NUL-terminated owned storage.
     *
     * length excludes the terminator.
     *
     * HIR strings are length-aware; embedded NUL bytes are therefore
     * representable even though data[length] is always '\0'.
     */
    char *data;
    size_t length;

    uint64_t hash;
} vitte_hir_string_t;

/* ========================================================================= */
/* Lists                                                                     */
/* ========================================================================= */

typedef struct vitte_hir_node_list {
    vitte_hir_node_id_t *items;

    size_t count;
    size_t capacity;
} vitte_hir_node_list_t;

typedef struct vitte_hir_type_list {
    vitte_hir_type_id_t *items;

    size_t count;
    size_t capacity;
} vitte_hir_type_list_t;

/* ========================================================================= */
/* Match                                                                     */
/* ========================================================================= */

typedef struct vitte_hir_match_arm {
    vitte_hir_node_id_t pattern;

    /*
     * Zero means no guard.
     */
    vitte_hir_node_id_t guard;

    vitte_hir_node_id_t body;

    vitte_hir_span_t span;
} vitte_hir_match_arm_t;

typedef struct vitte_hir_match_arm_list {
    vitte_hir_match_arm_t *items;

    size_t count;
    size_t capacity;
} vitte_hir_match_arm_list_t;

/* ========================================================================= */
/* HIR node                                                                  */
/* ========================================================================= */

typedef struct vitte_hir_node {
    vitte_hir_node_id_t id;

    vitte_hir_node_kind_t kind;

    /*
     * Semantic result type.
     *
     * Zero is accepted for nodes that do not produce a value or during
     * intermediate construction before semantic type assignment.
     */
    vitte_hir_type_id_t type;

    vitte_hir_span_t span;

    union {
        /* ----------------------------------------------------------------- */
        /* Module                                                            */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_string_id_t name;
            vitte_hir_node_list_t items;
        } module;

        /* ----------------------------------------------------------------- */
        /* use                                                               */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_string_id_t path;
        } use_decl;

        /* ----------------------------------------------------------------- */
        /* const                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_node_id_t value;

            vitte_hir_visibility_t visibility;
        } constant;

        /* ----------------------------------------------------------------- */
        /* static                                                            */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;

            /*
             * Zero may represent an absent initializer where semantics permit
             * it.
             */
            vitte_hir_node_id_t value;

            vitte_hir_visibility_t visibility;
            vitte_hir_mutability_t mutability;
        } static_decl;

        /* ----------------------------------------------------------------- */
        /* type                                                              */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;
            vitte_hir_type_id_t aliased_type;

            vitte_hir_visibility_t visibility;
        } type_decl;

        /* ----------------------------------------------------------------- */
        /* form / pick                                                       */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;

            vitte_hir_node_list_t fields;

            vitte_hir_visibility_t visibility;
        } aggregate_decl;

        /* ----------------------------------------------------------------- */
        /* trait                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;

            vitte_hir_node_list_t members;

            vitte_hir_visibility_t visibility;
        } trait_decl;

        /* ----------------------------------------------------------------- */
        /* impl                                                              */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_type_id_t target_type;

            /*
             * Zero supports inherent impl blocks.
             */
            vitte_hir_symbol_id_t trait_symbol;

            vitte_hir_node_list_t members;
        } impl_decl;

        /* ----------------------------------------------------------------- */
        /* proc / extern proc / test                                         */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;

            vitte_hir_node_list_t parameters;

            vitte_hir_type_id_t return_type;

            /*
             * Extern procedures may have no body.
             */
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

        /* ----------------------------------------------------------------- */
        /* block                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_list_t statements;

            /*
             * Optional tail expression.
             */
            vitte_hir_node_id_t tail;
        } block;

        /* ----------------------------------------------------------------- */
        /* let                                                               */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;

            /*
             * Zero means inferred type.
             */
            vitte_hir_type_id_t declared_type;

            /*
             * Optional initializer.
             */
            vitte_hir_node_id_t initializer;

            vitte_hir_mutability_t mutability;
        } let_stmt;

        /* ----------------------------------------------------------------- */
        /* assignment                                                        */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t target;
            vitte_hir_node_id_t value;
        } assignment;

        /* ----------------------------------------------------------------- */
        /* expression statement                                              */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t expression;
        } expression_statement;

        /* ----------------------------------------------------------------- */
        /* return                                                            */
        /* ----------------------------------------------------------------- */

        struct {
            /*
             * Zero means return without a value.
             */
            vitte_hir_node_id_t value;
        } return_stmt;

        /* ----------------------------------------------------------------- */
        /* defer                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t body;
        } defer_stmt;

        /* ----------------------------------------------------------------- */
        /* if                                                                */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t condition;
            vitte_hir_node_id_t then_branch;

            /*
             * Zero means no else branch.
             */
            vitte_hir_node_id_t else_branch;
        } if_expr;

        /* ----------------------------------------------------------------- */
        /* while                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t condition;
            vitte_hir_node_id_t body;
        } while_stmt;

        /* ----------------------------------------------------------------- */
        /* loop                                                              */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t body;
        } loop_stmt;

        /* ----------------------------------------------------------------- */
        /* for                                                               */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t pattern;
            vitte_hir_node_id_t iterator;
            vitte_hir_node_id_t body;
        } for_stmt;

        /* ----------------------------------------------------------------- */
        /* break                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            /*
             * Optional break value.
             */
            vitte_hir_node_id_t value;
        } break_stmt;

        /* ----------------------------------------------------------------- */
        /* assert                                                            */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t value;
        } assert_stmt;

        /* ----------------------------------------------------------------- */
        /* match                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t value;

            vitte_hir_match_arm_list_t arms;
        } match_expr;

        /* ----------------------------------------------------------------- */
        /* literal                                                           */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_literal_t value;
        } literal;

        /* ----------------------------------------------------------------- */
        /* resolved symbol expression                                        */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;
        } symbol_ref;

        /* ----------------------------------------------------------------- */
        /* unary                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_unary_operator_t op;

            vitte_hir_node_id_t operand;
        } unary;

        /* ----------------------------------------------------------------- */
        /* binary                                                            */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_binary_operator_t op;

            vitte_hir_node_id_t left;
            vitte_hir_node_id_t right;
        } binary;

        /* ----------------------------------------------------------------- */
        /* call                                                              */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t callee;

            vitte_hir_node_list_t arguments;
        } call;

        /* ----------------------------------------------------------------- */
        /* member                                                            */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t object;

            /*
             * Resolved member symbol.
             */
            vitte_hir_symbol_id_t member;
        } member;

        /* ----------------------------------------------------------------- */
        /* index                                                             */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t object;
            vitte_hir_node_id_t index;
        } index;

        /* ----------------------------------------------------------------- */
        /* cast                                                              */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t expression;

            vitte_hir_type_id_t destination_type;
        } cast;

        /* ----------------------------------------------------------------- */
        /* tuple / array                                                     */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_list_t elements;
        } aggregate;

        /* ----------------------------------------------------------------- */
        /* form construction                                                 */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_type_id_t form_type;

            vitte_hir_node_list_t fields;
        } form_init;

        /* ----------------------------------------------------------------- */
        /* ref                                                               */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t expression;

            bool mutable_reference;
        } reference;

        /* ----------------------------------------------------------------- */
        /* deref / move / await                                              */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_id_t expression;
        } unary_expression;

        /* ----------------------------------------------------------------- */
        /* sizeof / alignof / typeof                                         */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_type_id_t queried_type;
        } type_query;

        /* ----------------------------------------------------------------- */
        /* offsetof                                                          */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_type_id_t queried_type;
            vitte_hir_symbol_id_t member;
        } offset_query;

        /* ----------------------------------------------------------------- */
        /* binding pattern                                                   */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_symbol_id_t symbol;

            vitte_hir_mutability_t mutability;
        } binding_pattern;

        /* ----------------------------------------------------------------- */
        /* tuple pattern                                                     */
        /* ----------------------------------------------------------------- */

        struct {
            vitte_hir_node_list_t elements;
        } tuple_pattern;

        /* ----------------------------------------------------------------- */
        /* pick pattern                                                      */
        /* ----------------------------------------------------------------- */

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

    /*
     * Semantic type.
     *
     * Zero is accepted when a symbol does not have a value type or while the
     * HIR is being assembled.
     */
    vitte_hir_type_id_t type;

    /*
     * Lexical owner.
     *
     * Zero means no lexical scope / global root ownership.
     */
    vitte_hir_scope_id_t scope;

    /*
     * Node that introduced this symbol.
     *
     * Zero may be used transiently while constructing mutually-referential
     * structures.
     */
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

    /*
     * Zero means root scope.
     */
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
/* HIR context                                                               */
/* ========================================================================= */

/*
 * The context is intentionally public in API v1.
 *
 * This permits low-overhead compiler passes and straightforward debugging.
 *
 * Consumers should nevertheless prefer the accessor API for ordinary
 * operations so the representation can be made opaque in a future major API
 * version if required.
 */
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

    /*
     * Compilation-unit roots in deterministic source/insertion order.
     */
    vitte_hir_node_list_t roots;

    vitte_hir_stats_t stats;

    size_t max_nodes;
    size_t max_symbols;
    size_t max_types;
    size_t max_string_bytes;

    /*
     * Incremented by reset().
     *
     * Useful for detecting stale external caches associated with a previous
     * HIR generation.
     */
    uint64_t generation;
} vitte_hir_t;

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

/*
 * Returned strings have static storage duration.
 */

const char *
vitte_hir_error_name(
    vitte_hir_error_t error);

const char *
vitte_hir_state_name(
    vitte_hir_state_t state);

const char *
vitte_hir_type_kind_name(
    vitte_hir_type_kind_t kind);

const char *
vitte_hir_node_kind_name(
    vitte_hir_node_kind_t kind);

const char *
vitte_hir_symbol_kind_name(
    vitte_hir_symbol_kind_t kind);

const char *
vitte_hir_unary_operator_name(
    vitte_hir_unary_operator_t op);

const char *
vitte_hir_binary_operator_name(
    vitte_hir_binary_operator_t op);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize an empty BUILDING HIR.
 *
 * The object does not need to be zero-initialized beforehand.
 */
bool
vitte_hir_init(
    vitte_hir_t *hir);

/*
 * Release all memory owned by the HIR.
 *
 * After this operation:
 *
 *     state == VITTE_HIR_STATE_DESTROYED
 *
 * and the context must be initialized again before reuse.
 */
void
vitte_hir_destroy(
    vitte_hir_t *hir);

/*
 * Destroy all current contents and initialize a fresh BUILDING generation.
 *
 * Existing IDs become stale.
 */
bool
vitte_hir_reset(
    vitte_hir_t *hir);

/*
 * Fast context-envelope validation.
 *
 * This does not perform the complete graph validation performed by
 * vitte_hir_validate().
 */
bool
vitte_hir_is_valid(
    const vitte_hir_t *hir);

/* ========================================================================= */
/* State transitions                                                         */
/* ========================================================================= */

/*
 * End construction and validate the HIR.
 *
 * Successful transition:
 *
 *     BUILDING -> SEALED -> VALIDATED
 *
 * After success, mutation through the normal public mutation API is rejected.
 */
bool
vitte_hir_seal(
    vitte_hir_t *hir);

/*
 * Perform complete structural validation.
 *
 * Checks include:
 *
 *   - stable ID/index correspondence;
 *   - enum validity;
 *   - string integrity;
 *   - string hashes;
 *   - type references;
 *   - symbol references;
 *   - scope references;
 *   - node references;
 *   - list invariants;
 *   - source span ordering;
 *   - roots.
 *
 * When called on SEALED HIR and successful, state becomes VALIDATED.
 */
bool
vitte_hir_validate(
    vitte_hir_t *hir);

/* ========================================================================= */
/* String interning                                                          */
/* ========================================================================= */

/*
 * Intern exactly length bytes.
 *
 * data may be NULL only when length == 0.
 *
 * Equal byte sequences return the same string ID.
 *
 * The HIR owns a copied NUL-terminated representation.
 *
 * Returns VITTE_HIR_INVALID_ID on failure.
 */
vitte_hir_string_id_t
vitte_hir_intern_string(
    vitte_hir_t *hir,
    const char *data,
    size_t length);

const vitte_hir_string_t *
vitte_hir_get_string(
    const vitte_hir_t *hir,
    vitte_hir_string_id_t id);

/* ========================================================================= */
/* Types                                                                     */
/* ========================================================================= */

/*
 * Add a type.
 *
 * The structure is copied.
 *
 * If parameters are present, the parameter array is deep-copied.
 *
 * Returns VITTE_HIR_INVALID_ID on failure.
 */
vitte_hir_type_id_t
vitte_hir_add_type(
    vitte_hir_t *hir,
    const vitte_hir_type_t *type);

const vitte_hir_type_t *
vitte_hir_get_type(
    const vitte_hir_t *hir,
    vitte_hir_type_id_t id);

/* ========================================================================= */
/* Symbols                                                                   */
/* ========================================================================= */

/*
 * Add a resolved symbol.
 *
 * symbol->name must reference an interned HIR string.
 *
 * Returns VITTE_HIR_INVALID_ID on failure.
 */
vitte_hir_symbol_id_t
vitte_hir_add_symbol(
    vitte_hir_t *hir,
    const vitte_hir_symbol_t *symbol);

const vitte_hir_symbol_t *
vitte_hir_get_symbol(
    const vitte_hir_t *hir,
    vitte_hir_symbol_id_t id);

/* ========================================================================= */
/* Scopes                                                                    */
/* ========================================================================= */

/*
 * Add a lexical scope.
 *
 * parent == VITTE_HIR_INVALID_ID creates a root scope.
 */
vitte_hir_scope_id_t
vitte_hir_add_scope(
    vitte_hir_t *hir,
    vitte_hir_scope_id_t parent);

const vitte_hir_scope_t *
vitte_hir_get_scope(
    const vitte_hir_t *hir,
    vitte_hir_scope_id_t id);

/*
 * Add an existing symbol to a scope.
 *
 * Duplicate names in the same scope are rejected.
 */
bool
vitte_hir_scope_add_symbol(
    vitte_hir_t *hir,
    vitte_hir_scope_id_t scope,
    vitte_hir_symbol_id_t symbol);

/*
 * Resolve an interned name starting at scope and walking through parents.
 *
 * Search order:
 *
 *     current scope
 *          |
 *          v
 *       parent
 *          |
 *          v
 *       parent
 *          |
 *          v
 *         ...
 *
 * Within one scope, the most recently inserted matching symbol is selected.
 *
 * Returns VITTE_HIR_INVALID_ID when unresolved.
 */
vitte_hir_symbol_id_t
vitte_hir_scope_lookup(
    const vitte_hir_t *hir,
    vitte_hir_scope_id_t scope,
    vitte_hir_string_id_t name);

/* ========================================================================= */
/* Nodes                                                                     */
/* ========================================================================= */

/*
 * Add one HIR node.
 *
 * Embedded owned node lists and match-arm arrays are deep-copied.
 *
 * Returns VITTE_HIR_INVALID_ID on failure.
 */
vitte_hir_node_id_t
vitte_hir_add_node(
    vitte_hir_t *hir,
    const vitte_hir_node_t *node);

const vitte_hir_node_t *
vitte_hir_get_node(
    const vitte_hir_t *hir,
    vitte_hir_node_id_t id);

/*
 * Mutable node access is available only while BUILDING.
 */
vitte_hir_node_t *
vitte_hir_get_node_mut(
    vitte_hir_t *hir,
    vitte_hir_node_id_t id);

/*
 * Add an existing node to the compilation-unit root list.
 */
bool
vitte_hir_add_root(
    vitte_hir_t *hir,
    vitte_hir_node_id_t node);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Produce a deterministic structural fingerprint.
 *
 * The fingerprint is intended for:
 *
 *   - debugging;
 *   - incremental-build bookkeeping;
 *   - regression tests;
 *   - deterministic-build diagnostics.
 *
 * It is NOT a cryptographic hash.
 *
 * IMPORTANT:
 *
 * The corresponding hir.c implementation must iterate arrays with bounds
 * checked before dereferencing:
 *
 *     for (index = 0u; index < hir->string_count; ++index)
 *
 * and:
 *
 *     for (index = 0u; index < hir->symbol_count; ++index)
 *
 * rather than dereferencing the array in the loop condition.
 */
uint64_t
vitte_hir_fingerprint(
    vitte_hir_t *hir);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_hir_stats_t
vitte_hir_stats(
    const vitte_hir_t *hir);

/* ========================================================================= */
/* Inline ID helpers                                                         */
/* ========================================================================= */

static inline bool
vitte_hir_id_is_valid(
    vitte_hir_id_t id)
{
    return id != VITTE_HIR_INVALID_ID;
}

static inline bool
vitte_hir_node_id_is_valid(
    vitte_hir_node_id_t id)
{
    return id != VITTE_HIR_INVALID_ID;
}

static inline bool
vitte_hir_type_id_is_valid(
    vitte_hir_type_id_t id)
{
    return id != VITTE_HIR_INVALID_ID;
}

static inline bool
vitte_hir_symbol_id_is_valid(
    vitte_hir_symbol_id_t id)
{
    return id != VITTE_HIR_INVALID_ID;
}

static inline bool
vitte_hir_scope_id_is_valid(
    vitte_hir_scope_id_t id)
{
    return id != VITTE_HIR_INVALID_ID;
}

static inline bool
vitte_hir_string_id_is_valid(
    vitte_hir_string_id_t id)
{
    return id != VITTE_HIR_INVALID_ID;
}

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_hir_error_is_valid(
    vitte_hir_error_t error)
{
    return
        error >= VITTE_HIR_ERROR_NONE &&
        error < VITTE_HIR_ERROR_COUNT;
}

static inline bool
vitte_hir_state_is_valid(
    vitte_hir_state_t state)
{
    return
        state > VITTE_HIR_STATE_INVALID &&
        state < VITTE_HIR_STATE_COUNT;
}

static inline bool
vitte_hir_type_kind_is_valid_public(
    vitte_hir_type_kind_t kind)
{
    return
        kind > VITTE_HIR_TYPE_INVALID &&
        kind < VITTE_HIR_TYPE_COUNT;
}

static inline bool
vitte_hir_node_kind_is_valid_public(
    vitte_hir_node_kind_t kind)
{
    return
        kind > VITTE_HIR_NODE_INVALID &&
        kind < VITTE_HIR_NODE_COUNT;
}

static inline bool
vitte_hir_symbol_kind_is_valid_public(
    vitte_hir_symbol_kind_t kind)
{
    return
        kind > VITTE_HIR_SYMBOL_INVALID &&
        kind < VITTE_HIR_SYMBOL_COUNT;
}

static inline bool
vitte_hir_unary_operator_is_valid(
    vitte_hir_unary_operator_t op)
{
    return
        op > VITTE_HIR_UNARY_INVALID &&
        op < VITTE_HIR_UNARY_COUNT;
}

static inline bool
vitte_hir_binary_operator_is_valid(
    vitte_hir_binary_operator_t op)
{
    return
        op > VITTE_HIR_BINARY_INVALID &&
        op < VITTE_HIR_BINARY_COUNT;
}

static inline bool
vitte_hir_literal_kind_is_valid(
    vitte_hir_literal_kind_t kind)
{
    return
        kind > VITTE_HIR_LITERAL_INVALID &&
        kind < VITTE_HIR_LITERAL_COUNT;
}

/* ========================================================================= */
/* Inline state helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_hir_is_building(
    const vitte_hir_t *hir)
{
    return
        hir != NULL &&
        hir->magic == VITTE_HIR_MAGIC &&
        hir->state == VITTE_HIR_STATE_BUILDING;
}

static inline bool
vitte_hir_is_sealed(
    const vitte_hir_t *hir)
{
    return
        hir != NULL &&
        hir->magic == VITTE_HIR_MAGIC &&
        (
            hir->state == VITTE_HIR_STATE_SEALED ||
            hir->state == VITTE_HIR_STATE_VALIDATED
        );
}

static inline bool
vitte_hir_is_validated(
    const vitte_hir_t *hir)
{
    return
        hir != NULL &&
        hir->magic == VITTE_HIR_MAGIC &&
        hir->state == VITTE_HIR_STATE_VALIDATED;
}

/* ========================================================================= */
/* Inline source span helpers                                                */
/* ========================================================================= */

static inline vitte_hir_span_t
vitte_hir_span_invalid(void)
{
    vitte_hir_span_t span;

    span.file_id = 0u;
    span.begin = 0u;
    span.end = 0u;
    span.valid = false;

    return span;
}

static inline vitte_hir_span_t
vitte_hir_span_make(
    uint32_t file_id,
    size_t begin,
    size_t end)
{
    vitte_hir_span_t span;

    span.file_id = file_id;
    span.begin = begin;
    span.end = end;
    span.valid = begin <= end;

    return span;
}

static inline size_t
vitte_hir_span_length(
    const vitte_hir_span_t *span)
{
    if (span == NULL ||
        !span->valid ||
        span->end < span->begin) {
        return 0u;
    }

    return span->end - span->begin;
}

/* ========================================================================= */
/* Inline type classification                                                */
/* ========================================================================= */

static inline bool
vitte_hir_type_kind_is_signed_integer(
    vitte_hir_type_kind_t kind)
{
    return
        kind == VITTE_HIR_TYPE_I8 ||
        kind == VITTE_HIR_TYPE_I16 ||
        kind == VITTE_HIR_TYPE_I32 ||
        kind == VITTE_HIR_TYPE_I64;
}

static inline bool
vitte_hir_type_kind_is_unsigned_integer(
    vitte_hir_type_kind_t kind)
{
    return
        kind == VITTE_HIR_TYPE_U8 ||
        kind == VITTE_HIR_TYPE_U16 ||
        kind == VITTE_HIR_TYPE_U32 ||
        kind == VITTE_HIR_TYPE_U64;
}

static inline bool
vitte_hir_type_kind_is_integer(
    vitte_hir_type_kind_t kind)
{
    return
        vitte_hir_type_kind_is_signed_integer(kind) ||
        vitte_hir_type_kind_is_unsigned_integer(kind);
}

static inline bool
vitte_hir_type_kind_is_float(
    vitte_hir_type_kind_t kind)
{
    return
        kind == VITTE_HIR_TYPE_F32 ||
        kind == VITTE_HIR_TYPE_F64;
}

static inline bool
vitte_hir_type_kind_is_numeric(
    vitte_hir_type_kind_t kind)
{
    return
        vitte_hir_type_kind_is_integer(kind) ||
        vitte_hir_type_kind_is_float(kind);
}

static inline bool
vitte_hir_type_kind_is_scalar(
    vitte_hir_type_kind_t kind)
{
    return
        kind == VITTE_HIR_TYPE_BOOL ||
        kind == VITTE_HIR_TYPE_CHAR ||
        vitte_hir_type_kind_is_numeric(kind) ||
        kind == VITTE_HIR_TYPE_POINTER ||
        kind == VITTE_HIR_TYPE_REFERENCE;
}

static inline bool
vitte_hir_type_kind_is_sequence(
    vitte_hir_type_kind_t kind)
{
    return
        kind == VITTE_HIR_TYPE_ARRAY ||
        kind == VITTE_HIR_TYPE_SLICE ||
        kind == VITTE_HIR_TYPE_STRING;
}

/* ========================================================================= */
/* Inline node classification                                                */
/* ========================================================================= */

static inline bool
vitte_hir_node_kind_is_declaration(
    vitte_hir_node_kind_t kind)
{
    return
        kind == VITTE_HIR_NODE_CONST_DECL ||
        kind == VITTE_HIR_NODE_STATIC_DECL ||
        kind == VITTE_HIR_NODE_TYPE_DECL ||
        kind == VITTE_HIR_NODE_FORM_DECL ||
        kind == VITTE_HIR_NODE_PICK_DECL ||
        kind == VITTE_HIR_NODE_TRAIT_DECL ||
        kind == VITTE_HIR_NODE_IMPL_DECL ||
        kind == VITTE_HIR_NODE_PROC_DECL ||
        kind == VITTE_HIR_NODE_EXTERN_PROC_DECL ||
        kind == VITTE_HIR_NODE_MACRO_DECL ||
        kind == VITTE_HIR_NODE_TEST_DECL;
}

static inline bool
vitte_hir_node_kind_is_pattern(
    vitte_hir_node_kind_t kind)
{
    return
        kind == VITTE_HIR_NODE_PATTERN_WILDCARD ||
        kind == VITTE_HIR_NODE_PATTERN_LITERAL ||
        kind == VITTE_HIR_NODE_PATTERN_BINDING ||
        kind == VITTE_HIR_NODE_PATTERN_TUPLE ||
        kind == VITTE_HIR_NODE_PATTERN_PICK;
}

static inline bool
vitte_hir_node_kind_is_control_flow(
    vitte_hir_node_kind_t kind)
{
    return
        kind == VITTE_HIR_NODE_RETURN ||
        kind == VITTE_HIR_NODE_IF ||
        kind == VITTE_HIR_NODE_WHILE ||
        kind == VITTE_HIR_NODE_LOOP ||
        kind == VITTE_HIR_NODE_FOR ||
        kind == VITTE_HIR_NODE_BREAK ||
        kind == VITTE_HIR_NODE_CONTINUE ||
        kind == VITTE_HIR_NODE_MATCH;
}

/* ========================================================================= */
/* Inline list helpers                                                       */
/* ========================================================================= */

static inline vitte_hir_node_list_t
vitte_hir_node_list_empty(void)
{
    vitte_hir_node_list_t list;

    list.items = NULL;
    list.count = 0u;
    list.capacity = 0u;

    return list;
}

static inline vitte_hir_type_list_t
vitte_hir_type_list_empty(void)
{
    vitte_hir_type_list_t list;

    list.items = NULL;
    list.count = 0u;
    list.capacity = 0u;

    return list;
}

static inline vitte_hir_match_arm_list_t
vitte_hir_match_arm_list_empty(void)
{
    vitte_hir_match_arm_list_t list;

    list.items = NULL;
    list.count = 0u;
    list.capacity = 0u;

    return list;
}

static inline bool
vitte_hir_node_list_is_empty(
    const vitte_hir_node_list_t *list)
{
    return
        list == NULL ||
        list->count == 0u;
}

static inline const vitte_hir_node_id_t *
vitte_hir_node_list_at(
    const vitte_hir_node_list_t *list,
    size_t index)
{
    if (list == NULL ||
        index >= list->count ||
        list->items == NULL) {
        return NULL;
    }

    return &list->items[index];
}

static inline const vitte_hir_match_arm_t *
vitte_hir_match_arm_list_at(
    const vitte_hir_match_arm_list_t *list,
    size_t index)
{
    if (list == NULL ||
        index >= list->count ||
        list->items == NULL) {
        return NULL;
    }

    return &list->items[index];
}

/* ========================================================================= */
/* Inline literal constructors                                               */
/* ========================================================================= */

static inline vitte_hir_literal_t
vitte_hir_literal_make_bool(
    bool value)
{
    vitte_hir_literal_t literal = {0};

    literal.kind = VITTE_HIR_LITERAL_BOOL;
    literal.as.boolean = value;

    return literal;
}

static inline vitte_hir_literal_t
vitte_hir_literal_make_i64(
    int64_t value)
{
    vitte_hir_literal_t literal = {0};

    literal.kind =
        VITTE_HIR_LITERAL_SIGNED_INTEGER;

    literal.as.signed_integer = value;

    return literal;
}

static inline vitte_hir_literal_t
vitte_hir_literal_make_u64(
    uint64_t value)
{
    vitte_hir_literal_t literal = {0};

    literal.kind =
        VITTE_HIR_LITERAL_UNSIGNED_INTEGER;

    literal.as.unsigned_integer = value;

    return literal;
}

static inline vitte_hir_literal_t
vitte_hir_literal_make_f32(
    float value)
{
    vitte_hir_literal_t literal = {0};

    literal.kind = VITTE_HIR_LITERAL_F32;
    literal.as.f32 = value;

    return literal;
}

static inline vitte_hir_literal_t
vitte_hir_literal_make_f64(
    double value)
{
    vitte_hir_literal_t literal = {0};

    literal.kind = VITTE_HIR_LITERAL_F64;
    literal.as.f64 = value;

    return literal;
}

static inline vitte_hir_literal_t
vitte_hir_literal_make_char(
    uint32_t value)
{
    vitte_hir_literal_t literal = {0};

    literal.kind = VITTE_HIR_LITERAL_CHAR;
    literal.as.character = value;

    return literal;
}

static inline vitte_hir_literal_t
vitte_hir_literal_make_string(
    vitte_hir_string_id_t value)
{
    vitte_hir_literal_t literal = {0};

    literal.kind = VITTE_HIR_LITERAL_STRING;
    literal.as.string = value;

    return literal;
}

static inline vitte_hir_literal_t
vitte_hir_literal_make_null(void)
{
    vitte_hir_literal_t literal = {0};

    literal.kind = VITTE_HIR_LITERAL_NULL;

    return literal;
}

/* ========================================================================= */
/* Inline empty structures                                                   */
/* ========================================================================= */

static inline vitte_hir_type_t
vitte_hir_type_empty(void)
{
    vitte_hir_type_t type = {0};

    type.kind = VITTE_HIR_TYPE_INVALID;

    return type;
}

static inline vitte_hir_node_t
vitte_hir_node_empty(void)
{
    vitte_hir_node_t node = {0};

    node.kind = VITTE_HIR_NODE_INVALID;
    node.span = vitte_hir_span_invalid();

    return node;
}

static inline vitte_hir_symbol_t
vitte_hir_symbol_empty(void)
{
    vitte_hir_symbol_t symbol = {0};

    symbol.kind = VITTE_HIR_SYMBOL_INVALID;
    symbol.visibility = VITTE_HIR_VISIBILITY_PRIVATE;
    symbol.mutability = VITTE_HIR_MUTABILITY_IMMUTABLE;
    symbol.span = vitte_hir_span_invalid();

    return symbol;
}

/* ========================================================================= */
/* Inline context access                                                     */
/* ========================================================================= */

static inline vitte_hir_error_t
vitte_hir_last_error(
    const vitte_hir_t *hir)
{
    if (hir == NULL ||
        hir->magic != VITTE_HIR_MAGIC) {
        return VITTE_HIR_ERROR_INVALID_CONTEXT;
    }

    return hir->last_error;
}

static inline size_t
vitte_hir_node_count(
    const vitte_hir_t *hir)
{
    if (hir == NULL ||
        hir->magic != VITTE_HIR_MAGIC) {
        return 0u;
    }

    return hir->node_count;
}

static inline size_t
vitte_hir_type_count(
    const vitte_hir_t *hir)
{
    if (hir == NULL ||
        hir->magic != VITTE_HIR_MAGIC) {
        return 0u;
    }

    return hir->type_count;
}

static inline size_t
vitte_hir_symbol_count(
    const vitte_hir_t *hir)
{
    if (hir == NULL ||
        hir->magic != VITTE_HIR_MAGIC) {
        return 0u;
    }

    return hir->symbol_count;
}

static inline size_t
vitte_hir_scope_count(
    const vitte_hir_t *hir)
{
    if (hir == NULL ||
        hir->magic != VITTE_HIR_MAGIC) {
        return 0u;
    }

    return hir->scope_count;
}

static inline size_t
vitte_hir_string_count(
    const vitte_hir_t *hir)
{
    if (hir == NULL ||
        hir->magic != VITTE_HIR_MAGIC) {
        return 0u;
    }

    return hir->string_count;
}

static inline size_t
vitte_hir_root_count(
    const vitte_hir_t *hir)
{
    if (hir == NULL ||
        hir->magic != VITTE_HIR_MAGIC) {
        return 0u;
    }

    return hir->roots.count;
}

static inline uint64_t
vitte_hir_generation(
    const vitte_hir_t *hir)
{
    if (hir == NULL ||
        hir->magic != VITTE_HIR_MAGIC) {
        return UINT64_C(0);
    }

    return hir->generation;
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
    "HIR string byte limit must be non-zero");

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

_Static_assert(
    VITTE_HIR_UNARY_COUNT >
        VITTE_HIR_UNARY_BIT_NOT,
    "HIR unary operator enum invariant");

_Static_assert(
    VITTE_HIR_BINARY_COUNT >
        VITTE_HIR_BINARY_OR,
    "HIR binary operator enum invariant");

_Static_assert(
    VITTE_HIR_LITERAL_COUNT >
        VITTE_HIR_LITERAL_NULL,
    "HIR literal enum invariant");

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_HIR_HIR_H */
