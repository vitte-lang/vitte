#ifndef VITTE_SRC_AST_AST_H
#define VITTE_SRC_AST_AST_H

/*
 * Vitte Compiler
 * src/ast/ast.h
 *
 * Core Abstract Syntax Tree.
 *
 * Design:
 *
 *     - arena-backed lifetime
 *     - stable monotonically increasing node IDs
 *     - explicit source spans
 *     - parent/child topology
 *     - declaration / statement / expression / type / pattern families
 *     - semantic attachments
 *     - extensible attributes
 *     - deterministic traversal
 *     - deep validation
 *     - no individual node destruction
 *
 * Ownership:
 *
 *     Every node belongs to exactly one vitte_ast_t.
 *
 *     Node memory is owned by the arena associated with the AST.
 *
 *     Individual AST nodes are never free()'d.
 *
 *     Resetting/destroying the underlying arena invalidates node storage.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_AST_INITIAL_CHILD_CAPACITY
#define VITTE_AST_INITIAL_CHILD_CAPACITY ((size_t)4u)
#endif

#ifndef VITTE_AST_INITIAL_ATTRIBUTE_CAPACITY
#define VITTE_AST_INITIAL_ATTRIBUTE_CAPACITY ((size_t)2u)
#endif

#ifndef VITTE_AST_MAX_DEPTH
#define VITTE_AST_MAX_DEPTH ((size_t)65536u)
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_arena_context
    vitte_arena_context_t;

typedef struct vitte_ast
    vitte_ast_t;

typedef struct vitte_ast_node
    vitte_ast_node_t;

typedef struct vitte_ast_attribute
    vitte_ast_attribute_t;

typedef struct vitte_ast_stats
    vitte_ast_stats_t;

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

/*
 * Half-open byte interval:
 *
 *     [begin, end)
 *
 * file_id is intentionally independent from any source-manager pointer.
 */
typedef struct vitte_ast_span {
    uint32_t file_id;

    size_t begin;
    size_t end;

    /* Rich diagnostic coordinates; optional for AST-only clients. */
    uint64_t source_id;
    const char *source_name;
    size_t start_offset;
    size_t end_offset;
    unsigned start_line;
    unsigned start_column;
    unsigned end_line;
    unsigned end_column;
    bool valid;
} vitte_ast_span_t;

/* ========================================================================= */
/* Node kinds                                                                */
/* ========================================================================= */

typedef enum vitte_ast_kind {
    VITTE_AST_INVALID = 0,

    /* --------------------------------------------------------------------- */
    /* Root                                                                  */
    /* --------------------------------------------------------------------- */

    VITTE_AST_MODULE,

    /* --------------------------------------------------------------------- */
    /* Declarations                                                          */
    /* --------------------------------------------------------------------- */

    VITTE_AST_SPACE_DECL,
    VITTE_AST_USE_DECL,

    VITTE_AST_CONST_DECL,
    VITTE_AST_STATIC_DECL,

    VITTE_AST_TYPE_DECL,
    VITTE_AST_OPAQUE_DECL,
    VITTE_AST_FORM_DECL,
    VITTE_AST_PICK_DECL,

    VITTE_AST_TRAIT_DECL,
    VITTE_AST_IMPL_DECL,

    VITTE_AST_PROC_DECL,
    VITTE_AST_EXTERN_DECL,

    VITTE_AST_MACRO_DECL,
    VITTE_AST_TEST_DECL,

    /* --------------------------------------------------------------------- */
    /* Declaration components                                                */
    /* --------------------------------------------------------------------- */

    VITTE_AST_GENERIC_PARAM,
    VITTE_AST_WHERE_CLAUSE,
    VITTE_AST_PARAMETER,
    VITTE_AST_FIELD,
    VITTE_AST_VARIANT,
    VITTE_AST_ATTRIBUTE,

    /* --------------------------------------------------------------------- */
    /* Statements                                                            */
    /* --------------------------------------------------------------------- */

    VITTE_AST_BLOCK_STMT,

    VITTE_AST_LET_STMT,
    VITTE_AST_EXPR_STMT,

    VITTE_AST_RETURN_STMT,
    VITTE_AST_DEFER_STMT,

    VITTE_AST_IF_STMT,
    VITTE_AST_WHILE_STMT,
    VITTE_AST_LOOP_STMT,
    VITTE_AST_FOR_STMT,

    VITTE_AST_BREAK_STMT,
    VITTE_AST_CONTINUE_STMT,

    VITTE_AST_MATCH_STMT,

    VITTE_AST_UNSAFE_STMT,
    VITTE_AST_ASM_STMT,

    VITTE_AST_ASSERT_STMT,

    /* --------------------------------------------------------------------- */
    /* Expressions                                                           */
    /* --------------------------------------------------------------------- */

    VITTE_AST_NAME_EXPR,

    VITTE_AST_INTEGER_EXPR,
    VITTE_AST_FLOAT_EXPR,
    VITTE_AST_STRING_EXPR,
    VITTE_AST_CHAR_EXPR,
    VITTE_AST_BOOL_EXPR,
    VITTE_AST_NULL_EXPR,

    VITTE_AST_ARRAY_EXPR,
    VITTE_AST_TUPLE_EXPR,
    VITTE_AST_MAP_EXPR,

    VITTE_AST_UNARY_EXPR,
    VITTE_AST_BINARY_EXPR,
    VITTE_AST_ASSIGN_EXPR,

    VITTE_AST_CALL_EXPR,
    VITTE_AST_INDEX_EXPR,
    VITTE_AST_MEMBER_EXPR,

    VITTE_AST_CAST_EXPR,
    VITTE_AST_RANGE_EXPR,

    VITTE_AST_MATCH_EXPR,

    VITTE_AST_AWAIT_EXPR,

    VITTE_AST_MOVE_EXPR,
    VITTE_AST_REF_EXPR,

    VITTE_AST_SIZEOF_EXPR,
    VITTE_AST_ALIGNOF_EXPR,
    VITTE_AST_OFFSETOF_EXPR,
    VITTE_AST_TYPEOF_EXPR,

    /* --------------------------------------------------------------------- */
    /* Types                                                                 */
    /* --------------------------------------------------------------------- */

    VITTE_AST_NAMED_TYPE,

    VITTE_AST_POINTER_TYPE,
    VITTE_AST_REFERENCE_TYPE,

    VITTE_AST_ARRAY_TYPE,
    VITTE_AST_SLICE_TYPE,

    VITTE_AST_TUPLE_TYPE,
    VITTE_AST_FUNCTION_TYPE,

    VITTE_AST_DYN_TYPE,
    VITTE_AST_INFER_TYPE,

    /* --------------------------------------------------------------------- */
    /* Patterns                                                              */
    /* --------------------------------------------------------------------- */

    VITTE_AST_WILDCARD_PATTERN,
    VITTE_AST_BINDING_PATTERN,
    VITTE_AST_LITERAL_PATTERN,

    VITTE_AST_TUPLE_PATTERN,

    VITTE_AST_FORM_PATTERN,
    VITTE_AST_PICK_PATTERN,

    VITTE_AST_RANGE_PATTERN,
    VITTE_AST_OR_PATTERN,

    /* --------------------------------------------------------------------- */
    /* Match                                                                 */
    /* --------------------------------------------------------------------- */

    VITTE_AST_MATCH_ARM,

    /* --------------------------------------------------------------------- */
    /* Sentinel                                                              */
    /* --------------------------------------------------------------------- */

    VITTE_AST_KIND_COUNT
} vitte_ast_kind_t;

/* ========================================================================= */
/* Unary operators                                                           */
/* ========================================================================= */

typedef enum vitte_ast_unary_operator {
    VITTE_AST_UNARY_INVALID = 0,

    VITTE_AST_UNARY_PLUS,
    VITTE_AST_UNARY_MINUS,

    VITTE_AST_UNARY_NOT,

    VITTE_AST_UNARY_BIT_NOT,

    VITTE_AST_UNARY_DEREF,
    VITTE_AST_UNARY_REF,

    VITTE_AST_UNARY_COUNT
} vitte_ast_unary_operator_t;

/* ========================================================================= */
/* Binary operators                                                          */
/* ========================================================================= */

typedef enum vitte_ast_binary_operator {
    VITTE_AST_BINARY_INVALID = 0,

    /* Arithmetic */
    VITTE_AST_BINARY_ADD,
    VITTE_AST_BINARY_SUB,
    VITTE_AST_BINARY_MUL,
    VITTE_AST_BINARY_DIV,
    VITTE_AST_BINARY_MOD,

    /* Bitwise */
    VITTE_AST_BINARY_BIT_AND,
    VITTE_AST_BINARY_BIT_OR,
    VITTE_AST_BINARY_BIT_XOR,
    VITTE_AST_BINARY_SHL,
    VITTE_AST_BINARY_SHR,

    /* Comparison */
    VITTE_AST_BINARY_EQ,
    VITTE_AST_BINARY_NE,

    VITTE_AST_BINARY_LT,
    VITTE_AST_BINARY_LE,
    VITTE_AST_BINARY_GT,
    VITTE_AST_BINARY_GE,

    /* Logical */
    VITTE_AST_BINARY_AND,
    VITTE_AST_BINARY_OR,

    VITTE_AST_BINARY_COUNT
} vitte_ast_binary_operator_t;

/* ========================================================================= */
/* Flags                                                                     */
/* ========================================================================= */

typedef uint64_t vitte_ast_flags_t;

#define VITTE_AST_FLAG_NONE \
    UINT64_C(0)

#define VITTE_AST_FLAG_INVALID \
    (UINT64_C(1) << 0)

#define VITTE_AST_FLAG_PUBLIC \
    (UINT64_C(1) << 1)

#define VITTE_AST_FLAG_MUTABLE \
    (UINT64_C(1) << 2)

#define VITTE_AST_FLAG_ASYNC \
    (UINT64_C(1) << 3)

#define VITTE_AST_FLAG_UNSAFE \
    (UINT64_C(1) << 4)

#define VITTE_AST_FLAG_CONST \
    (UINT64_C(1) << 5)

#define VITTE_AST_FLAG_EXTERN \
    (UINT64_C(1) << 6)

#define VITTE_AST_FLAG_INLINE \
    (UINT64_C(1) << 7)

#define VITTE_AST_FLAG_NOINLINE \
    (UINT64_C(1) << 8)

#define VITTE_AST_FLAG_VARIADIC \
    (UINT64_C(1) << 9)

#define VITTE_AST_FLAG_STATIC \
    (UINT64_C(1) << 10)

#define VITTE_AST_FLAG_MOVE \
    (UINT64_C(1) << 11)

#define VITTE_AST_FLAG_REFERENCE \
    (UINT64_C(1) << 12)

#define VITTE_AST_FLAG_SYNTHETIC \
    (UINT64_C(1) << 13)

#define VITTE_AST_FLAG_IMPLICIT \
    (UINT64_C(1) << 14)

#define VITTE_AST_FLAG_GENERATED \
    (UINT64_C(1) << 15)

#define VITTE_AST_FLAG_RESOLVED \
    (UINT64_C(1) << 16)

#define VITTE_AST_FLAG_TYPED \
    (UINT64_C(1) << 17)

#define VITTE_AST_FLAG_LOWERED \
    (UINT64_C(1) << 18)

/* ========================================================================= */
/* Attribute                                                                 */
/* ========================================================================= */

struct vitte_ast_attribute {
    const char *name;
    size_t name_length;

    const char *value;
    size_t value_length;

    vitte_ast_span_t span;
};

/* ========================================================================= */
/* Node payloads                                                             */
/* ========================================================================= */

typedef struct vitte_ast_integer_data {
    uint64_t value;
    bool negative;
} vitte_ast_integer_data_t;

typedef struct vitte_ast_float_data {
    double value;
} vitte_ast_float_data_t;

typedef struct vitte_ast_string_data {
    const char *value;
    size_t length;
} vitte_ast_string_data_t;

typedef struct vitte_ast_boolean_data {
    bool value;
} vitte_ast_boolean_data_t;

typedef struct vitte_ast_unary_data {
    vitte_ast_unary_operator_t op;
} vitte_ast_unary_data_t;

typedef struct vitte_ast_binary_data {
    vitte_ast_binary_operator_t op;
} vitte_ast_binary_data_t;

/*
 * Generic payload union.
 *
 * Child topology is stored separately from payload data.
 *
 * This intentionally keeps semantic/type-system structures outside the AST
 * core. semantic points to phase-owned semantic metadata.
 */
typedef union vitte_ast_node_data {
    vitte_ast_integer_data_t integer;
    vitte_ast_float_data_t floating;
    vitte_ast_string_data_t string;
    vitte_ast_boolean_data_t boolean;

    vitte_ast_unary_data_t unary;
    vitte_ast_binary_data_t binary;

    uint64_t raw_u64[4];
    void *raw_pointer[4];
} vitte_ast_node_data_t;

/* ========================================================================= */
/* AST node                                                                  */
/* ========================================================================= */

struct vitte_ast_node {
    /*
     * Runtime integrity cookie.
     */
    uint64_t magic;

    /*
     * Owning AST.
     */
    vitte_ast_t *owner;

    /*
     * Stable AST-local node identifier.
     *
     * Zero is invalid.
     */
    uint64_t id;

    /*
     * AST generation in which this node was created.
     */
    uint64_t generation;

    /*
     * Syntactic category.
     */
    vitte_ast_kind_t kind;

    /*
     * Original source range.
     */
    vitte_ast_span_t span;

    /*
     * Tree topology.
     */
    vitte_ast_node_t *parent;

    vitte_ast_node_t **children;

    size_t child_count;
    size_t child_capacity;

    /*
     * Source attributes.
     */
    vitte_ast_attribute_t *attributes;

    size_t attribute_count;
    size_t attribute_capacity;

    /*
     * Generic node properties.
     */
    vitte_ast_flags_t flags;

    /*
     * Optional node name.
     *
     * Stored as arena-owned NUL-terminated memory.
     *
     * name_length excludes the terminator.
     */
    const char *name;
    size_t name_length;

    /*
     * Syntactic payload.
     */
    vitte_ast_node_data_t data;

    /*
     * Optional semantic-phase attachment.
     *
     * The AST does not own this object.
     */
    void *semantic;
};

/* ========================================================================= */
/* AST                                                                       */
/* ========================================================================= */

struct vitte_ast {
    uint64_t magic;

    /*
     * Non-owning arena context.
     *
     * AST allocations are made through this context.
     */
    vitte_arena_context_t *arena;

    /*
     * Root node.
     *
     * Usually VITTE_AST_MODULE.
     */
    vitte_ast_node_t *root;

    /*
     * Next stable node identifier.
     */
    uint64_t next_node_id;

    /*
     * Current number of nodes allocated in this AST generation.
     */
    size_t node_count;

    /*
     * Lifetime high-water node count.
     */
    size_t peak_node_count;

    /*
     * Logical AST generation.
     *
     * Zero is reserved.
     */
    uint64_t generation;

    /*
     * Parser/AST error accounting.
     */
    size_t error_count;
};

/* ========================================================================= */
/* Traversal                                                                 */
/* ========================================================================= */

typedef enum vitte_ast_visit_phase {
    VITTE_AST_VISIT_ENTER = 0,
    VITTE_AST_VISIT_LEAVE = 1
} vitte_ast_visit_phase_t;

/*
 * Return false to stop traversal.
 */
typedef bool
(*vitte_ast_visit_fn)(
    vitte_ast_node_t *node,
    vitte_ast_visit_phase_t phase,
    size_t depth,
    void *user_data);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_ast_stats {
    /*
     * AST accounting.
     */
    size_t node_count;
    size_t peak_node_count;

    /*
     * Nodes reachable from root.
     */
    size_t reachable_nodes;

    /*
     * Maximum root-relative depth.
     *
     * Root depth is zero.
     */
    size_t maximum_depth;

    /*
     * Number of parent -> child edges.
     */
    size_t child_edges;

    /*
     * Number of source attributes.
     */
    size_t attribute_count;

    /*
     * Node-family counts.
     */
    size_t declaration_count;
    size_t statement_count;
    size_t expression_count;
    size_t type_count;
    size_t pattern_count;

    /*
     * Nodes carrying VITTE_AST_FLAG_INVALID.
     */
    size_t invalid_node_count;

    uint64_t generation;

    size_t error_count;
};

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_ast_init(
    vitte_ast_t *ast,
    vitte_arena_context_t *arena);

/*
 * Destroy AST metadata.
 *
 * Does not destroy the external arena.
 */
void
vitte_ast_destroy(
    vitte_ast_t *ast);

/*
 * Start a new logical AST generation.
 *
 * Important:
 *
 * This function does not reset the external arena itself.
 *
 * The owner of the arena controls its physical reset policy.
 */
void
vitte_ast_reset(
    vitte_ast_t *ast);

/* ========================================================================= */
/* Validity                                                                  */
/* ========================================================================= */

bool
vitte_ast_is_valid(
    const vitte_ast_t *ast);

bool
vitte_ast_node_is_valid(
    const vitte_ast_node_t *node);

/* ========================================================================= */
/* Kind classification                                                       */
/* ========================================================================= */

bool
vitte_ast_kind_is_declaration(
    vitte_ast_kind_t kind);

bool
vitte_ast_kind_is_statement(
    vitte_ast_kind_t kind);

bool
vitte_ast_kind_is_expression(
    vitte_ast_kind_t kind);

bool
vitte_ast_kind_is_type(
    vitte_ast_kind_t kind);

bool
vitte_ast_kind_is_pattern(
    vitte_ast_kind_t kind);

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_ast_kind_name(
    vitte_ast_kind_t kind);

const char *
vitte_ast_unary_operator_name(
    vitte_ast_unary_operator_t op);

const char *
vitte_ast_binary_operator_name(
    vitte_ast_binary_operator_t op);

/* ========================================================================= */
/* Source spans                                                              */
/* ========================================================================= */

vitte_ast_span_t
vitte_ast_span_make(
    uint32_t file_id,
    size_t begin,
    size_t end);

bool
vitte_ast_span_is_valid(
    vitte_ast_span_t span);

size_t
vitte_ast_span_length(
    vitte_ast_span_t span);

vitte_ast_span_t
vitte_ast_span_join(
    vitte_ast_span_t left,
    vitte_ast_span_t right);

/* ========================================================================= */
/* Node creation                                                             */
/* ========================================================================= */

vitte_ast_node_t *
vitte_ast_node_create(
    vitte_ast_t *ast,
    vitte_ast_kind_t kind,
    vitte_ast_span_t span);

vitte_ast_node_t *
vitte_ast_node_create_named(
    vitte_ast_t *ast,
    vitte_ast_kind_t kind,
    vitte_ast_span_t span,
    const char *name,
    size_t name_length);

/* ========================================================================= */
/* Tree construction                                                         */
/* ========================================================================= */

bool
vitte_ast_node_add_child(
    vitte_ast_node_t *parent,
    vitte_ast_node_t *child);

bool
vitte_ast_node_insert_child(
    vitte_ast_node_t *parent,
    size_t index,
    vitte_ast_node_t *child);

bool
vitte_ast_node_replace_child(
    vitte_ast_node_t *parent,
    size_t index,
    vitte_ast_node_t *replacement);

vitte_ast_node_t *
vitte_ast_node_detach_child(
    vitte_ast_node_t *parent,
    size_t index);

/* ========================================================================= */
/* Child access                                                              */
/* ========================================================================= */

vitte_ast_node_t *
vitte_ast_node_child(
    vitte_ast_node_t *node,
    size_t index);

const vitte_ast_node_t *
vitte_ast_node_child_const(
    const vitte_ast_node_t *node,
    size_t index);

size_t
vitte_ast_node_child_count(
    const vitte_ast_node_t *node);

/* ========================================================================= */
/* Root                                                                      */
/* ========================================================================= */

bool
vitte_ast_set_root(
    vitte_ast_t *ast,
    vitte_ast_node_t *root);

vitte_ast_node_t *
vitte_ast_root(
    vitte_ast_t *ast);

const vitte_ast_node_t *
vitte_ast_root_const(
    const vitte_ast_t *ast);

/* ========================================================================= */
/* Attributes                                                                */
/* ========================================================================= */

bool
vitte_ast_node_add_attribute(
    vitte_ast_node_t *node,
    const char *name,
    size_t name_length,
    const char *value,
    size_t value_length,
    vitte_ast_span_t span);

const vitte_ast_attribute_t *
vitte_ast_node_attribute(
    const vitte_ast_node_t *node,
    size_t index);

const vitte_ast_attribute_t *
vitte_ast_node_find_attribute(
    const vitte_ast_node_t *node,
    const char *name,
    size_t name_length);

/* ========================================================================= */
/* Flags                                                                     */
/* ========================================================================= */

void
vitte_ast_node_set_flag(
    vitte_ast_node_t *node,
    vitte_ast_flags_t flag);

void
vitte_ast_node_clear_flag(
    vitte_ast_node_t *node,
    vitte_ast_flags_t flag);

bool
vitte_ast_node_has_flag(
    const vitte_ast_node_t *node,
    vitte_ast_flags_t flag);

/* ========================================================================= */
/* Basic node accessors                                                      */
/* ========================================================================= */

uint64_t
vitte_ast_node_id(
    const vitte_ast_node_t *node);

vitte_ast_kind_t
vitte_ast_node_kind(
    const vitte_ast_node_t *node);

vitte_ast_span_t
vitte_ast_node_span(
    const vitte_ast_node_t *node);

vitte_ast_node_t *
vitte_ast_node_parent(
    vitte_ast_node_t *node);

const vitte_ast_node_t *
vitte_ast_node_parent_const(
    const vitte_ast_node_t *node);

const char *
vitte_ast_node_name(
    const vitte_ast_node_t *node);

size_t
vitte_ast_node_name_length(
    const vitte_ast_node_t *node);

/* ========================================================================= */
/* Semantic attachment                                                       */
/* ========================================================================= */

void
vitte_ast_node_set_semantic(
    vitte_ast_node_t *node,
    void *semantic);

void *
vitte_ast_node_semantic(
    vitte_ast_node_t *node);

const void *
vitte_ast_node_semantic_const(
    const vitte_ast_node_t *node);

/* ========================================================================= */
/* Literal payload                                                           */
/* ========================================================================= */

void
vitte_ast_node_set_integer(
    vitte_ast_node_t *node,
    uint64_t value,
    bool negative);

void
vitte_ast_node_set_float(
    vitte_ast_node_t *node,
    double value);

bool
vitte_ast_node_set_string(
    vitte_ast_node_t *node,
    const char *value,
    size_t length);

void
vitte_ast_node_set_bool(
    vitte_ast_node_t *node,
    bool value);

/* ========================================================================= */
/* Operator payload                                                          */
/* ========================================================================= */

void
vitte_ast_node_set_unary_operator(
    vitte_ast_node_t *node,
    vitte_ast_unary_operator_t op);

void
vitte_ast_node_set_binary_operator(
    vitte_ast_node_t *node,
    vitte_ast_binary_operator_t op);

/* ========================================================================= */
/* Search                                                                    */
/* ========================================================================= */

vitte_ast_node_t *
vitte_ast_find_id(
    vitte_ast_t *ast,
    uint64_t id);

/* ========================================================================= */
/* Traversal                                                                 */
/* ========================================================================= */

/*
 * Deterministic depth-first traversal.
 *
 * Each node receives:
 *
 *     ENTER
 *     recursively visit children
 *     LEAVE
 *
 * Returning false from the visitor aborts traversal.
 */
bool
vitte_ast_visit(
    vitte_ast_t *ast,
    vitte_ast_visit_fn visitor,
    void *user_data);

/* ========================================================================= */
/* Ancestor utilities                                                        */
/* ========================================================================= */

bool
vitte_ast_node_is_ancestor_of(
    const vitte_ast_node_t *ancestor,
    const vitte_ast_node_t *node);

size_t
vitte_ast_node_depth(
    const vitte_ast_node_t *node);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_ast_stats_t
vitte_ast_stats(
    const vitte_ast_t *ast);

/* ========================================================================= */
/* Error accounting                                                          */
/* ========================================================================= */

void
vitte_ast_record_error(
    vitte_ast_t *ast);

size_t
vitte_ast_error_count(
    const vitte_ast_t *ast);

/* ========================================================================= */
/* AST accessors                                                             */
/* ========================================================================= */

size_t
vitte_ast_node_count(
    const vitte_ast_t *ast);

size_t
vitte_ast_peak_node_count(
    const vitte_ast_t *ast);

uint64_t
vitte_ast_generation(
    const vitte_ast_t *ast);

vitte_arena_context_t *
vitte_ast_arena(
    vitte_ast_t *ast);

const vitte_arena_context_t *
vitte_ast_arena_const(
    const vitte_ast_t *ast);

/* ========================================================================= */
/* Deep validation                                                          */
/* ========================================================================= */

/*
 * Validate complete rooted AST.
 *
 * Checks include:
 *
 *     AST magic
 *     arena validity
 *     generation
 *     root validity
 *     root parent == NULL
 *     node ownership
 *     node generation
 *     source spans
 *     node IDs
 *     child count/capacity
 *     attribute count/capacity
 *     parent relationships
 *     duplicate children
 *     name consistency
 *     attribute consistency
 *     maximum traversal depth
 *     reachable node count == AST node count
 *
 * Detached construction nodes therefore make validation fail until attached.
 */
bool
vitte_ast_validate(
    const vitte_ast_t *ast);

/* ========================================================================= */
/* Inline kind helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_ast_node_is_declaration(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        vitte_ast_kind_is_declaration(
            node->kind);
}

static inline bool
vitte_ast_node_is_statement(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        vitte_ast_kind_is_statement(
            node->kind);
}

static inline bool
vitte_ast_node_is_expression(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        vitte_ast_kind_is_expression(
            node->kind);
}

static inline bool
vitte_ast_node_is_type(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        vitte_ast_kind_is_type(
            node->kind);
}

static inline bool
vitte_ast_node_is_pattern(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        vitte_ast_kind_is_pattern(
            node->kind);
}

/* ========================================================================= */
/* Inline topology helpers                                                   */
/* ========================================================================= */

static inline bool
vitte_ast_node_is_root(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->parent == NULL &&
        node->owner->root == node;
}

static inline bool
vitte_ast_node_is_leaf(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->child_count == 0u;
}

static inline bool
vitte_ast_node_has_children(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->child_count != 0u;
}

static inline bool
vitte_ast_node_has_parent(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->parent != NULL;
}

static inline vitte_ast_node_t *
vitte_ast_node_first_child(
    vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node) ||
        node->child_count == 0u) {
        return NULL;
    }

    return node->children[0];
}

static inline const vitte_ast_node_t *
vitte_ast_node_first_child_const(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node) ||
        node->child_count == 0u) {
        return NULL;
    }

    return node->children[0];
}

static inline vitte_ast_node_t *
vitte_ast_node_last_child(
    vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node) ||
        node->child_count == 0u) {
        return NULL;
    }

    return
        node->children[
            node->child_count - 1u];
}

static inline const vitte_ast_node_t *
vitte_ast_node_last_child_const(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node) ||
        node->child_count == 0u) {
        return NULL;
    }

    return
        node->children[
            node->child_count - 1u];
}

/* ========================================================================= */
/* Inline name helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_ast_node_has_name(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->name != NULL;
}

static inline bool
vitte_ast_node_name_equals(
    const vitte_ast_node_t *node,
    const char *name,
    size_t length)
{
    size_t index;

    if (!vitte_ast_node_is_valid(node) ||
        node->name == NULL ||
        name == NULL ||
        node->name_length != length) {
        return false;
    }

    for (index = 0u;
         index < length;
         ++index) {
        if (node->name[index] !=
            name[index]) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Inline attribute helpers                                                  */
/* ========================================================================= */

static inline size_t
vitte_ast_node_attribute_count(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return 0u;
    }

    return node->attribute_count;
}

static inline bool
vitte_ast_node_has_attributes(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_attribute_count(node) !=
        0u;
}

/* ========================================================================= */
/* Inline AST helpers                                                        */
/* ========================================================================= */

static inline bool
vitte_ast_is_empty(
    const vitte_ast_t *ast)
{
    return
        vitte_ast_is_valid(ast) &&
        ast->node_count == 0u;
}

static inline bool
vitte_ast_has_root(
    const vitte_ast_t *ast)
{
    return
        vitte_ast_is_valid(ast) &&
        ast->root != NULL;
}

static inline bool
vitte_ast_has_errors(
    const vitte_ast_t *ast)
{
    return
        vitte_ast_error_count(ast) != 0u;
}

/* ========================================================================= */
/* Typed creation conveniences                                               */
/* ========================================================================= */

#define VITTE_AST_NEW(ast, kind, span) \
    vitte_ast_node_create( \
        (ast), \
        (kind), \
        (span))

#define VITTE_AST_NEW_NAMED( \
    ast, \
    kind, \
    span, \
    name, \
    length) \
    vitte_ast_node_create_named( \
        (ast), \
        (kind), \
        (span), \
        (name), \
        (length))

#define VITTE_AST_ADD(parent, child) \
    vitte_ast_node_add_child( \
        (parent), \
        (child))

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte AST requires 64-bit uint64_t");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "Vitte AST requires 32-bit uint32_t");

_Static_assert(
    VITTE_AST_INVALID == 0,
    "invalid AST kind must remain zero");

_Static_assert(
    VITTE_AST_UNARY_INVALID == 0,
    "invalid unary operator must remain zero");

_Static_assert(
    VITTE_AST_BINARY_INVALID == 0,
    "invalid binary operator must remain zero");

_Static_assert(
    VITTE_AST_VISIT_ENTER == 0,
    "AST enter traversal phase must remain zero");

_Static_assert(
    VITTE_AST_INITIAL_CHILD_CAPACITY != 0u,
    "AST initial child capacity must not be zero");

_Static_assert(
    VITTE_AST_INITIAL_ATTRIBUTE_CAPACITY != 0u,
    "AST initial attribute capacity must not be zero");

_Static_assert(
    VITTE_AST_MAX_DEPTH != 0u,
    "AST maximum depth must not be zero");

_Static_assert(
    (VITTE_AST_FLAG_INVALID &
     VITTE_AST_FLAG_PUBLIC) == 0u,
    "AST flags must use distinct bits");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_AST_AST_H */
