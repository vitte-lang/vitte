#ifndef VITTE_AST_AST_H
#define VITTE_AST_AST_H

/*
 * Vitte Compiler
 * src/ast/ast.h
 *
 * Core Abstract Syntax Tree public interface.
 *
 * ISO C17.
 *
 * Design:
 *
 *   - arena-backed AST storage
 *   - stable monotonically increasing node IDs
 *   - generation-based stale-node detection
 *   - explicit half-open source spans
 *   - strict single-parent tree ownership
 *   - deterministic ordered children
 *   - declaration / statement / expression / type / pattern families
 *   - extensible attributes
 *   - semantic attachments
 *   - deterministic traversal
 *   - structural statistics
 *   - deep validation
 *   - no individual AST-node destruction
 *
 * Lifetime:
 *
 *   A vitte_ast_t does not own its vitte_arena_context_t.
 *
 *   All AST nodes, child arrays, attributes and copied strings are allocated
 *   from that arena and therefore have arena lifetime.
 *
 *   vitte_ast_destroy() destroys AST metadata only. It does not destroy or
 *   reset the arena.
 *
 *   vitte_ast_reset() logically invalidates existing nodes by advancing the
 *   AST generation. It deliberately does not reset the external arena.
 *
 * Thread safety:
 *
 *   AST objects are not internally synchronized.
 *   Concurrent mutation requires external synchronization.
 */

#include "../arena/arena.h"
#include "span.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_ast vitte_ast_t;
typedef struct vitte_ast_node vitte_ast_node_t;
typedef struct vitte_ast_attribute vitte_ast_attribute_t;

/* ========================================================================= */
/* Node kinds                                                                */
/* ========================================================================= */

typedef enum vitte_ast_kind {
    VITTE_AST_INVALID = 0,

    /* Root. */
    VITTE_AST_MODULE,

    /* Declarations. */
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

    /* Declaration components. */
    VITTE_AST_GENERIC_PARAM,
    VITTE_AST_WHERE_CLAUSE,
    VITTE_AST_PARAMETER,
    VITTE_AST_FIELD,
    VITTE_AST_VARIANT,
    VITTE_AST_ATTRIBUTE,

    /* Statements. */
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

    /* Expressions. */
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

    /* Types. */
    VITTE_AST_NAMED_TYPE,
    VITTE_AST_POINTER_TYPE,
    VITTE_AST_REFERENCE_TYPE,
    VITTE_AST_ARRAY_TYPE,
    VITTE_AST_SLICE_TYPE,
    VITTE_AST_TUPLE_TYPE,
    VITTE_AST_FUNCTION_TYPE,
    VITTE_AST_DYN_TYPE,
    VITTE_AST_INFER_TYPE,

    /* Patterns. */
    VITTE_AST_WILDCARD_PATTERN,
    VITTE_AST_BINDING_PATTERN,
    VITTE_AST_LITERAL_PATTERN,
    VITTE_AST_TUPLE_PATTERN,
    VITTE_AST_FORM_PATTERN,
    VITTE_AST_PICK_PATTERN,
    VITTE_AST_RANGE_PATTERN,
    VITTE_AST_OR_PATTERN,

    /* Miscellaneous structural nodes. */
    VITTE_AST_MATCH_ARM,

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

    VITTE_AST_UNARY_OPERATOR_COUNT
} vitte_ast_unary_operator_t;

/* ========================================================================= */
/* Binary operators                                                          */
/* ========================================================================= */

typedef enum vitte_ast_binary_operator {
    VITTE_AST_BINARY_INVALID = 0,

    VITTE_AST_BINARY_ADD,
    VITTE_AST_BINARY_SUB,
    VITTE_AST_BINARY_MUL,
    VITTE_AST_BINARY_DIV,
    VITTE_AST_BINARY_MOD,

    VITTE_AST_BINARY_BIT_AND,
    VITTE_AST_BINARY_BIT_OR,
    VITTE_AST_BINARY_BIT_XOR,
    VITTE_AST_BINARY_SHL,
    VITTE_AST_BINARY_SHR,

    VITTE_AST_BINARY_EQ,
    VITTE_AST_BINARY_NE,
    VITTE_AST_BINARY_LT,
    VITTE_AST_BINARY_LE,
    VITTE_AST_BINARY_GT,
    VITTE_AST_BINARY_GE,

    VITTE_AST_BINARY_AND,
    VITTE_AST_BINARY_OR,

    VITTE_AST_BINARY_OPERATOR_COUNT
} vitte_ast_binary_operator_t;

/* ========================================================================= */
/* Flags                                                                     */
/* ========================================================================= */

typedef uint64_t vitte_ast_flags_t;

#define VITTE_AST_FLAG_NONE          UINT64_C(0)
#define VITTE_AST_FLAG_INVALID       (UINT64_C(1) << 0)
#define VITTE_AST_FLAG_SYNTHETIC     (UINT64_C(1) << 1)
#define VITTE_AST_FLAG_PUBLIC        (UINT64_C(1) << 2)
#define VITTE_AST_FLAG_EXPORT        (UINT64_C(1) << 3)
#define VITTE_AST_FLAG_ASYNC         (UINT64_C(1) << 4)
#define VITTE_AST_FLAG_UNSAFE        (UINT64_C(1) << 5)
#define VITTE_AST_FLAG_COMPTIME      (UINT64_C(1) << 6)
#define VITTE_AST_FLAG_MUTABLE       (UINT64_C(1) << 7)
#define VITTE_AST_FLAG_EXTERN        (UINT64_C(1) << 8)
#define VITTE_AST_FLAG_INTRINSIC     (UINT64_C(1) << 9)
#define VITTE_AST_FLAG_TEST          (UINT64_C(1) << 10)
#define VITTE_AST_FLAG_IMPLICIT      (UINT64_C(1) << 11)
#define VITTE_AST_FLAG_RECOVERED     (UINT64_C(1) << 12)
#define VITTE_AST_FLAG_GENERATED     (UINT64_C(1) << 13)

#define VITTE_AST_FLAG_ALL_KNOWN \
    (VITTE_AST_FLAG_INVALID   | \
     VITTE_AST_FLAG_SYNTHETIC | \
     VITTE_AST_FLAG_PUBLIC    | \
     VITTE_AST_FLAG_EXPORT    | \
     VITTE_AST_FLAG_ASYNC     | \
     VITTE_AST_FLAG_UNSAFE    | \
     VITTE_AST_FLAG_COMPTIME  | \
     VITTE_AST_FLAG_MUTABLE   | \
     VITTE_AST_FLAG_EXTERN    | \
     VITTE_AST_FLAG_INTRINSIC | \
     VITTE_AST_FLAG_TEST      | \
     VITTE_AST_FLAG_IMPLICIT  | \
     VITTE_AST_FLAG_RECOVERED | \
     VITTE_AST_FLAG_GENERATED)

/* ========================================================================= */
/* Attributes                                                                */
/* ========================================================================= */

/*
 * Attribute strings are immutable arena-owned NUL-terminated copies.
 *
 * name is always non-NULL for a published attribute.
 *
 * value may be NULL. A NULL value means that the attribute has no explicit
 * value. In that case value_length is zero.
 */
struct vitte_ast_attribute {
    const char *name;
    size_t name_length;

    const char *value;
    size_t value_length;

    vitte_ast_span_t span;
};

/* ========================================================================= */
/* Node payload                                                              */
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

typedef struct vitte_ast_bool_data {
    bool value;
} vitte_ast_bool_data_t;

typedef struct vitte_ast_unary_data {
    vitte_ast_unary_operator_t op;
} vitte_ast_unary_data_t;

typedef struct vitte_ast_binary_data {
    vitte_ast_binary_operator_t op;
} vitte_ast_binary_data_t;

typedef union vitte_ast_node_data {
    vitte_ast_integer_data_t integer;
    vitte_ast_float_data_t floating;
    vitte_ast_string_data_t string;
    vitte_ast_bool_data_t boolean;
    vitte_ast_unary_data_t unary;
    vitte_ast_binary_data_t binary;
} vitte_ast_node_data_t;

/* ========================================================================= */
/* AST node                                                                  */
/* ========================================================================= */

/*
 * Node layout is public because several compiler stages use AST nodes as a
 * compact intermediate frontend representation.
 *
 * Mutation should nevertheless go through the API whenever parent/child
 * invariants, ownership, generation or arena allocation are involved.
 */
struct vitte_ast_node {
    uint64_t magic;

    vitte_ast_t *owner;

    uint64_t id;
    uint64_t generation;

    vitte_ast_kind_t kind;
    vitte_ast_span_t span;

    vitte_ast_node_t *parent;

    vitte_ast_node_t **children;
    size_t child_count;
    size_t child_capacity;

    vitte_ast_attribute_t *attributes;
    size_t attribute_count;
    size_t attribute_capacity;

    vitte_ast_flags_t flags;

    const char *name;
    size_t name_length;

    /*
     * Opaque semantic attachment.
     *
     * Ownership is not transferred to the AST.
     */
    void *semantic;

    vitte_ast_node_data_t data;
};

/* ========================================================================= */
/* AST                                                                       */
/* ========================================================================= */

struct vitte_ast {
    uint64_t magic;

    /*
     * Externally-owned arena.
     */
    vitte_arena_context_t *arena;

    vitte_ast_node_t *root;

    /*
     * next_node_id is always the next ID that may be assigned.
     *
     * Zero is never a valid node ID.
     *
     * IDs are not reset by vitte_ast_reset().
     */
    uint64_t next_node_id;

    /*
     * Number of current-generation nodes logically owned by this AST.
     *
     * Deep validation requires all of these nodes to be reachable from root.
     */
    size_t node_count;

    /*
     * Lifetime high-water mark for simultaneous logical node count.
     */
    size_t peak_node_count;

    /*
     * Current AST generation.
     *
     * Zero is reserved as invalid.
     */
    uint64_t generation;

    /*
     * Parser/frontend error accounting associated with the current AST.
     *
     * Saturates at SIZE_MAX.
     */
    size_t error_count;
};

/* ========================================================================= */
/* Traversal                                                                 */
/* ========================================================================= */

typedef enum vitte_ast_visit_event {
    VITTE_AST_VISIT_ENTER = 0,
    VITTE_AST_VISIT_LEAVE = 1
} vitte_ast_visit_event_t;

/*
 * Return false to stop traversal immediately.
 *
 * depth:
 *
 *     root      -> 0
 *     child     -> 1
 *     grandchild-> 2
 */
typedef bool (*vitte_ast_visit_fn)(
    vitte_ast_node_t *node,
    vitte_ast_visit_event_t event,
    size_t depth,
    void *user_data);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_ast_stats {
    /*
     * Current logical AST node count.
     */
    size_t node_count;

    /*
     * Lifetime logical high-water mark.
     */
    size_t peak_node_count;

    /*
     * Number of nodes reachable from root during statistics collection.
     */
    size_t reachable_nodes;

    /*
     * Maximum root-relative depth.
     *
     * A root-only tree has depth zero.
     */
    size_t maximum_depth;

    size_t child_edges;
    size_t attribute_count;

    size_t declaration_count;
    size_t statement_count;
    size_t expression_count;
    size_t type_count;
    size_t pattern_count;

    size_t invalid_node_count;

    uint64_t generation;

    size_t error_count;
} vitte_ast_stats_t;

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

/* ========================================================================= */
/* Validity                                                                  */
/* ========================================================================= */

/*
 * Lightweight runtime validity checks.
 *
 * These are not substitutes for vitte_ast_validate().
 */
bool
vitte_ast_is_valid(
    const vitte_ast_t *ast);

bool
vitte_ast_node_is_valid(
    const vitte_ast_node_t *node);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize AST metadata over an existing valid arena.
 *
 * The arena remains owned by the caller.
 *
 * The destination object may be fresh/indeterminate storage because init()
 * fully overwrites it before use.
 *
 * Reinitializing an already-live AST without first calling
 * vitte_ast_destroy() is a caller error.
 */
bool
vitte_ast_init(
    vitte_ast_t *ast,
    vitte_arena_context_t *arena);

/*
 * Destroy AST metadata.
 *
 * Does not destroy or reset the arena.
 */
void
vitte_ast_destroy(
    vitte_ast_t *ast);

/*
 * Logically reset the AST.
 *
 * Existing node pointers become stale because generation changes.
 *
 * The external arena is not reset.
 * Node IDs remain monotonic.
 * peak_node_count remains a lifetime high-water mark.
 */
void
vitte_ast_reset(
    vitte_ast_t *ast);

/* ========================================================================= */
/* Node creation                                                             */
/* ========================================================================= */

vitte_ast_node_t *
vitte_ast_node_create(
    vitte_ast_t *ast,
    vitte_ast_kind_t kind,
    vitte_ast_span_t span);

/*
 * Create a node with an immutable arena-owned name copy.
 *
 * name need not be NUL-terminated when name_length is explicitly supplied.
 */
vitte_ast_node_t *
vitte_ast_node_create_named(
    vitte_ast_t *ast,
    vitte_ast_kind_t kind,
    vitte_ast_span_t span,
    const char *name,
    size_t name_length);

/* ========================================================================= */
/* Children                                                                  */
/* ========================================================================= */

/*
 * Attach child to parent at the end of the ordered child sequence.
 *
 * Requirements:
 *
 *   - parent and child are valid
 *   - both belong to the same AST/generation
 *   - child currently has no parent
 *   - the operation must not create a cycle
 */
bool
vitte_ast_node_add_child(
    vitte_ast_node_t *parent,
    vitte_ast_node_t *child);

/*
 * Insert child before the current element at index.
 *
 * index == child_count appends.
 */
bool
vitte_ast_node_insert_child(
    vitte_ast_node_t *parent,
    size_t index,
    vitte_ast_node_t *child);

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

/*
 * Replace one attached child.
 *
 * replacement must currently be detached unless it is already the child at
 * index.
 *
 * The replaced child becomes detached.
 */
bool
vitte_ast_node_replace_child(
    vitte_ast_node_t *parent,
    size_t index,
    vitte_ast_node_t *replacement);

/*
 * Remove one child from the ordered child sequence.
 *
 * The returned child becomes detached.
 */
vitte_ast_node_t *
vitte_ast_node_detach_child(
    vitte_ast_node_t *parent,
    size_t index);

/* ========================================================================= */
/* Attributes                                                                */
/* ========================================================================= */

/*
 * Add an immutable attribute.
 *
 * name is required.
 *
 * value may be NULL only when value_length == 0.
 *
 * name/value byte sequences need not be NUL-terminated; the AST stores
 * NUL-terminated arena-owned copies.
 */
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

/*
 * semantic is an opaque non-owning attachment.
 *
 * The AST does not allocate, destroy or otherwise manage the pointed object.
 */
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
/* Literal values                                                            */
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

/*
 * Store an immutable arena-owned NUL-terminated copy.
 *
 * value need not already be NUL-terminated.
 */
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
/* Operators                                                                 */
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

/*
 * Find a node by ID in the rooted tree.
 *
 * Detached nodes are intentionally not searched.
 */
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
 * For each node:
 *
 *   ENTER(node)
 *       children in ascending index order
 *   LEAVE(node)
 *
 * Returning false from the visitor stops traversal and causes this function
 * to return false.
 *
 * An empty AST is considered successfully traversed.
 */
bool
vitte_ast_visit(
    vitte_ast_t *ast,
    vitte_ast_visit_fn visitor,
    void *user_data);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_ast_stats_t
vitte_ast_stats(
    const vitte_ast_t *ast);

/* ========================================================================= */
/* Deep validation                                                           */
/* ========================================================================= */

/*
 * Perform deep structural validation.
 *
 * A finalized valid AST requires:
 *
 *   - valid AST metadata and arena
 *   - non-zero generation
 *   - valid root ownership
 *   - root has no parent
 *   - every current logical node is reachable from root
 *   - every node belongs to this AST/current generation
 *   - valid source spans
 *   - non-zero unique node IDs below next_node_id
 *   - child_count <= child_capacity
 *   - attribute_count <= attribute_capacity
 *   - valid parent/child symmetry
 *   - no duplicate sibling child pointers
 *   - no structural cycles
 *   - valid copied strings and attributes
 *
 * During parser construction detached nodes may temporarily exist. In that
 * state vitte_ast_validate() intentionally returns false until the finalized
 * tree contains all current logical nodes.
 */
bool
vitte_ast_validate(
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
/* Ancestor utilities                                                        */
/* ========================================================================= */

/*
 * Return true only for a strict ancestor.
 *
 * A node is not considered its own ancestor.
 */
bool
vitte_ast_node_is_ancestor_of(
    const vitte_ast_node_t *ancestor,
    const vitte_ast_node_t *node);

/*
 * Return root-relative depth.
 *
 * root -> 0
 *
 * SIZE_MAX indicates detected structural corruption or excessive depth.
 *
 * Note that zero is also returned for a valid detached node because it has
 * no parent. Use node->parent/root membership when that distinction matters.
 */
size_t
vitte_ast_node_depth(
    const vitte_ast_node_t *node);

/* ========================================================================= */
/* Convenience predicates                                                    */
/* ========================================================================= */

static inline bool
vitte_ast_node_is_root(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->owner != NULL &&
        node->owner->root == node &&
        node->parent == NULL;
}

static inline bool
vitte_ast_node_is_detached(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->parent == NULL &&
        (node->owner == NULL ||
         node->owner->root != node);
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
vitte_ast_node_has_attributes(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->attribute_count != 0u;
}

static inline bool
vitte_ast_node_is_named(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        node->name != NULL;
}

static inline bool
vitte_ast_node_is_invalid(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        (node->flags &
         VITTE_AST_FLAG_INVALID) != 0u;
}

static inline bool
vitte_ast_node_is_synthetic(
    const vitte_ast_node_t *node)
{
    return
        vitte_ast_node_is_valid(node) &&
        (node->flags &
         VITTE_AST_FLAG_SYNTHETIC) != 0u;
}

static inline bool
vitte_ast_node_is_current_generation(
    const vitte_ast_node_t *node)
{
    return
        node != NULL &&
        node->owner != NULL &&
        vitte_ast_is_valid(node->owner) &&
        node->generation ==
            node->owner->generation;
}

static inline bool
vitte_ast_is_empty(
    const vitte_ast_t *ast)
{
    return
        vitte_ast_is_valid(ast) &&
        ast->node_count == 0u;
}

/* ========================================================================= */
/* Compile-time ABI checks                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(vitte_ast_flags_t) == sizeof(uint64_t),
    "vitte_ast_flags_t must remain 64-bit");

_Static_assert(
    VITTE_AST_INVALID == 0,
    "VITTE_AST_INVALID must remain zero");

_Static_assert(
    VITTE_AST_UNARY_INVALID == 0,
    "VITTE_AST_UNARY_INVALID must remain zero");

_Static_assert(
    VITTE_AST_BINARY_INVALID == 0,
    "VITTE_AST_BINARY_INVALID must remain zero");

_Static_assert(
    VITTE_AST_VISIT_ENTER == 0,
    "VITTE_AST_VISIT_ENTER must remain zero");

_Static_assert(
    VITTE_AST_VISIT_LEAVE == 1,
    "VITTE_AST_VISIT_LEAVE must remain one");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_AST_AST_H */
