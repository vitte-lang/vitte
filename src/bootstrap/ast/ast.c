/*
 * Vitte Compiler
 * src/ast/ast.c
 *
 * Core Abstract Syntax Tree implementation.
 *
 * ISO C17.
 *
 * Properties:
 *
 *   - arena-backed lifetime
 *   - stable monotonically increasing node IDs
 *   - generation-based stale-node rejection
 *   - explicit source spans
 *   - strict single-parent tree ownership
 *   - ordered children
 *   - extensible attributes
 *   - semantic attachments
 *   - overflow-safe storage growth
 *   - deterministic traversal
 *   - bounded recursion
 *   - deep structural validation
 *   - no individual node destruction
 *
 * Ownership:
 *
 *   Every node belongs to exactly one vitte_ast_t.
 *   Every allocation is owned by the AST arena.
 *
 *   vitte_ast_destroy() destroys only AST metadata. It does not destroy or
 *   reset the externally-owned arena.
 *
 *   vitte_ast_reset() logically invalidates every existing AST node through
 *   generation advancement. It deliberately does not reset the arena because
 *   the arena may be shared with other compiler subsystems.
 */

#include "ast.h"

#include "../arena/arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* These classification switches intentionally use a default branch because
 * the AST grows independently of the public predicates. Keep strict builds
 * enabled for all other switches without turning this compatibility policy
 * into a compiler-specific build failure. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_AST_MAGIC
#define VITTE_AST_MAGIC UINT64_C(0x5649545445415354)
#endif

#ifndef VITTE_AST_DEAD_MAGIC
#define VITTE_AST_DEAD_MAGIC UINT64_C(0x4445414441535421)
#endif

#ifndef VITTE_AST_NODE_MAGIC
#define VITTE_AST_NODE_MAGIC UINT64_C(0x56495454454E4F44)
#endif

#ifndef VITTE_AST_NODE_DEAD_MAGIC
#define VITTE_AST_NODE_DEAD_MAGIC UINT64_C(0x444541444E4F4445)
#endif

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
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_AST_INITIAL_CHILD_CAPACITY != 0u,
    "AST initial child capacity must be non-zero");

_Static_assert(
    VITTE_AST_INITIAL_ATTRIBUTE_CAPACITY != 0u,
    "AST initial attribute capacity must be non-zero");

_Static_assert(
    VITTE_AST_MAX_DEPTH != 0u,
    "AST maximum depth must be non-zero");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte AST requires 64-bit uint64_t");

#endif

/* ========================================================================= */
/* Arithmetic                                                                */
/* ========================================================================= */

static bool
vitte_ast_add_overflow(
    size_t a,
    size_t b,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (a > SIZE_MAX - b) {
        return true;
    }

    *result = a + b;
    return false;
}

static bool
vitte_ast_mul_overflow(
    size_t a,
    size_t b,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (a != 0u &&
        b > SIZE_MAX / a) {
        return true;
    }

    *result = a * b;
    return false;
}

static size_t
vitte_ast_saturating_add(
    size_t a,
    size_t b)
{
    size_t result;

    if (vitte_ast_add_overflow(
            a,
            b,
            &result)) {
        return SIZE_MAX;
    }

    return result;
}

static uint64_t
vitte_ast_next_generation(
    uint64_t generation)
{
    ++generation;

    if (generation == UINT64_C(0)) {
        generation = UINT64_C(1);
    }

    return generation;
}

static bool
vitte_ast_is_power_of_two(
    size_t value)
{
    return
        value != 0u &&
        (value & (value - 1u)) == 0u;
}

/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static bool
vitte_ast_kind_is_known(
    vitte_ast_kind_t kind)
{
    return
        strcmp(
            vitte_ast_kind_name(kind),
            "unknown") != 0;
}

static bool
vitte_ast_node_belongs_to(
    const vitte_ast_t *ast,
    const vitte_ast_node_t *node)
{
    return
        ast != NULL &&
        node != NULL &&
        node->magic == VITTE_AST_NODE_MAGIC &&
        node->owner == ast &&
        node->generation == ast->generation;
}

static bool
vitte_ast_string_has_terminator(
    const char *string,
    size_t length)
{
    if (string == NULL) {
        return length == 0u;
    }

    return string[length] == '\0';
}

/* ========================================================================= */
/* Kind classification                                                       */
/* ========================================================================= */

bool
vitte_ast_kind_is_declaration(
    vitte_ast_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_SPACE_DECL:
        case VITTE_AST_USE_DECL:
        case VITTE_AST_CONST_DECL:
        case VITTE_AST_STATIC_DECL:
        case VITTE_AST_TYPE_DECL:
        case VITTE_AST_OPAQUE_DECL:
        case VITTE_AST_FORM_DECL:
        case VITTE_AST_PICK_DECL:
        case VITTE_AST_TRAIT_DECL:
        case VITTE_AST_IMPL_DECL:
        case VITTE_AST_PROC_DECL:
        case VITTE_AST_EXTERN_DECL:
        case VITTE_AST_MACRO_DECL:
        case VITTE_AST_TEST_DECL:
            return true;

        default:
            return false;
    }
}

bool
vitte_ast_kind_is_statement(
    vitte_ast_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_BLOCK_STMT:
        case VITTE_AST_LET_STMT:
        case VITTE_AST_EXPR_STMT:
        case VITTE_AST_RETURN_STMT:
        case VITTE_AST_DEFER_STMT:
        case VITTE_AST_IF_STMT:
        case VITTE_AST_WHILE_STMT:
        case VITTE_AST_LOOP_STMT:
        case VITTE_AST_FOR_STMT:
        case VITTE_AST_BREAK_STMT:
        case VITTE_AST_CONTINUE_STMT:
        case VITTE_AST_MATCH_STMT:
        case VITTE_AST_UNSAFE_STMT:
        case VITTE_AST_ASM_STMT:
        case VITTE_AST_ASSERT_STMT:
            return true;

        default:
            return false;
    }
}

bool
vitte_ast_kind_is_expression(
    vitte_ast_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NAME_EXPR:
        case VITTE_AST_INTEGER_EXPR:
        case VITTE_AST_FLOAT_EXPR:
        case VITTE_AST_STRING_EXPR:
        case VITTE_AST_CHAR_EXPR:
        case VITTE_AST_BOOL_EXPR:
        case VITTE_AST_NULL_EXPR:
        case VITTE_AST_ARRAY_EXPR:
        case VITTE_AST_TUPLE_EXPR:
        case VITTE_AST_MAP_EXPR:
        case VITTE_AST_UNARY_EXPR:
        case VITTE_AST_BINARY_EXPR:
        case VITTE_AST_ASSIGN_EXPR:
        case VITTE_AST_CALL_EXPR:
        case VITTE_AST_INDEX_EXPR:
        case VITTE_AST_MEMBER_EXPR:
        case VITTE_AST_CAST_EXPR:
        case VITTE_AST_RANGE_EXPR:
        case VITTE_AST_MATCH_EXPR:
        case VITTE_AST_AWAIT_EXPR:
        case VITTE_AST_MOVE_EXPR:
        case VITTE_AST_REF_EXPR:
        case VITTE_AST_SIZEOF_EXPR:
        case VITTE_AST_ALIGNOF_EXPR:
        case VITTE_AST_OFFSETOF_EXPR:
        case VITTE_AST_TYPEOF_EXPR:
            return true;

        default:
            return false;
    }
}

bool
vitte_ast_kind_is_type(
    vitte_ast_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NAMED_TYPE:
        case VITTE_AST_POINTER_TYPE:
        case VITTE_AST_REFERENCE_TYPE:
        case VITTE_AST_ARRAY_TYPE:
        case VITTE_AST_SLICE_TYPE:
        case VITTE_AST_TUPLE_TYPE:
        case VITTE_AST_FUNCTION_TYPE:
        case VITTE_AST_DYN_TYPE:
        case VITTE_AST_INFER_TYPE:
            return true;

        default:
            return false;
    }
}

bool
vitte_ast_kind_is_pattern(
    vitte_ast_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_WILDCARD_PATTERN:
        case VITTE_AST_BINDING_PATTERN:
        case VITTE_AST_LITERAL_PATTERN:
        case VITTE_AST_TUPLE_PATTERN:
        case VITTE_AST_FORM_PATTERN:
        case VITTE_AST_PICK_PATTERN:
        case VITTE_AST_RANGE_PATTERN:
        case VITTE_AST_OR_PATTERN:
            return true;

        default:
            return false;
    }
}

/* ========================================================================= */
/* Kind names                                                                */
/* ========================================================================= */

const char *
vitte_ast_kind_name(
    vitte_ast_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_INVALID: return "invalid";
        case VITTE_AST_MODULE: return "module";

        case VITTE_AST_SPACE_DECL: return "space-decl";
        case VITTE_AST_USE_DECL: return "use-decl";
        case VITTE_AST_CONST_DECL: return "const-decl";
        case VITTE_AST_STATIC_DECL: return "static-decl";
        case VITTE_AST_TYPE_DECL: return "type-decl";
        case VITTE_AST_OPAQUE_DECL: return "opaque-decl";
        case VITTE_AST_FORM_DECL: return "form-decl";
        case VITTE_AST_PICK_DECL: return "pick-decl";
        case VITTE_AST_TRAIT_DECL: return "trait-decl";
        case VITTE_AST_IMPL_DECL: return "impl-decl";
        case VITTE_AST_PROC_DECL: return "proc-decl";
        case VITTE_AST_EXTERN_DECL: return "extern-decl";
        case VITTE_AST_MACRO_DECL: return "macro-decl";
        case VITTE_AST_TEST_DECL: return "test-decl";

        case VITTE_AST_GENERIC_PARAM: return "generic-param";
        case VITTE_AST_WHERE_CLAUSE: return "where-clause";
        case VITTE_AST_PARAMETER: return "parameter";
        case VITTE_AST_FIELD: return "field";
        case VITTE_AST_VARIANT: return "variant";
        case VITTE_AST_ATTRIBUTE: return "attribute";

        case VITTE_AST_BLOCK_STMT: return "block-stmt";
        case VITTE_AST_LET_STMT: return "let-stmt";
        case VITTE_AST_EXPR_STMT: return "expr-stmt";
        case VITTE_AST_RETURN_STMT: return "return-stmt";
        case VITTE_AST_DEFER_STMT: return "defer-stmt";
        case VITTE_AST_IF_STMT: return "if-stmt";
        case VITTE_AST_WHILE_STMT: return "while-stmt";
        case VITTE_AST_LOOP_STMT: return "loop-stmt";
        case VITTE_AST_FOR_STMT: return "for-stmt";
        case VITTE_AST_BREAK_STMT: return "break-stmt";
        case VITTE_AST_CONTINUE_STMT: return "continue-stmt";
        case VITTE_AST_MATCH_STMT: return "match-stmt";
        case VITTE_AST_UNSAFE_STMT: return "unsafe-stmt";
        case VITTE_AST_ASM_STMT: return "asm-stmt";
        case VITTE_AST_ASSERT_STMT: return "assert-stmt";

        case VITTE_AST_NAME_EXPR: return "name-expr";
        case VITTE_AST_INTEGER_EXPR: return "integer-expr";
        case VITTE_AST_FLOAT_EXPR: return "float-expr";
        case VITTE_AST_STRING_EXPR: return "string-expr";
        case VITTE_AST_CHAR_EXPR: return "char-expr";
        case VITTE_AST_BOOL_EXPR: return "bool-expr";
        case VITTE_AST_NULL_EXPR: return "null-expr";
        case VITTE_AST_ARRAY_EXPR: return "array-expr";
        case VITTE_AST_TUPLE_EXPR: return "tuple-expr";
        case VITTE_AST_MAP_EXPR: return "map-expr";
        case VITTE_AST_UNARY_EXPR: return "unary-expr";
        case VITTE_AST_BINARY_EXPR: return "binary-expr";
        case VITTE_AST_ASSIGN_EXPR: return "assign-expr";
        case VITTE_AST_CALL_EXPR: return "call-expr";
        case VITTE_AST_INDEX_EXPR: return "index-expr";
        case VITTE_AST_MEMBER_EXPR: return "member-expr";
        case VITTE_AST_CAST_EXPR: return "cast-expr";
        case VITTE_AST_RANGE_EXPR: return "range-expr";
        case VITTE_AST_MATCH_EXPR: return "match-expr";
        case VITTE_AST_AWAIT_EXPR: return "await-expr";
        case VITTE_AST_MOVE_EXPR: return "move-expr";
        case VITTE_AST_REF_EXPR: return "ref-expr";
        case VITTE_AST_SIZEOF_EXPR: return "sizeof-expr";
        case VITTE_AST_ALIGNOF_EXPR: return "alignof-expr";
        case VITTE_AST_OFFSETOF_EXPR: return "offsetof-expr";
        case VITTE_AST_TYPEOF_EXPR: return "typeof-expr";

        case VITTE_AST_NAMED_TYPE: return "named-type";
        case VITTE_AST_POINTER_TYPE: return "pointer-type";
        case VITTE_AST_REFERENCE_TYPE: return "reference-type";
        case VITTE_AST_ARRAY_TYPE: return "array-type";
        case VITTE_AST_SLICE_TYPE: return "slice-type";
        case VITTE_AST_TUPLE_TYPE: return "tuple-type";
        case VITTE_AST_FUNCTION_TYPE: return "function-type";
        case VITTE_AST_DYN_TYPE: return "dyn-type";
        case VITTE_AST_INFER_TYPE: return "infer-type";

        case VITTE_AST_WILDCARD_PATTERN: return "wildcard-pattern";
        case VITTE_AST_BINDING_PATTERN: return "binding-pattern";
        case VITTE_AST_LITERAL_PATTERN: return "literal-pattern";
        case VITTE_AST_TUPLE_PATTERN: return "tuple-pattern";
        case VITTE_AST_FORM_PATTERN: return "form-pattern";
        case VITTE_AST_PICK_PATTERN: return "pick-pattern";
        case VITTE_AST_RANGE_PATTERN: return "range-pattern";
        case VITTE_AST_OR_PATTERN: return "or-pattern";

        case VITTE_AST_MATCH_ARM: return "match-arm";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Operators                                                                 */
/* ========================================================================= */

const char *
vitte_ast_unary_operator_name(
    vitte_ast_unary_operator_t op)
{
    switch (op) {
        case VITTE_AST_UNARY_PLUS: return "+";
        case VITTE_AST_UNARY_MINUS: return "-";
        case VITTE_AST_UNARY_NOT: return "not";
        case VITTE_AST_UNARY_BIT_NOT: return "~";
        case VITTE_AST_UNARY_DEREF: return "*";
        case VITTE_AST_UNARY_REF: return "ref";

        default:
            return "invalid";
    }
}

const char *
vitte_ast_binary_operator_name(
    vitte_ast_binary_operator_t op)
{
    switch (op) {
        case VITTE_AST_BINARY_ADD: return "+";
        case VITTE_AST_BINARY_SUB: return "-";
        case VITTE_AST_BINARY_MUL: return "*";
        case VITTE_AST_BINARY_DIV: return "/";
        case VITTE_AST_BINARY_MOD: return "%";

        case VITTE_AST_BINARY_BIT_AND: return "&";
        case VITTE_AST_BINARY_BIT_OR: return "|";
        case VITTE_AST_BINARY_BIT_XOR: return "^";
        case VITTE_AST_BINARY_SHL: return "<<";
        case VITTE_AST_BINARY_SHR: return ">>";

        case VITTE_AST_BINARY_EQ: return "==";
        case VITTE_AST_BINARY_NE: return "!=";
        case VITTE_AST_BINARY_LT: return "<";
        case VITTE_AST_BINARY_LE: return "<=";
        case VITTE_AST_BINARY_GT: return ">";
        case VITTE_AST_BINARY_GE: return ">=";

        case VITTE_AST_BINARY_AND: return "and";
        case VITTE_AST_BINARY_OR: return "or";

        default:
            return "invalid";
    }
}

/* ========================================================================= */
/* Source spans                                                              */
/* ========================================================================= */

vitte_ast_span_t
vitte_ast_span_make(
    uint32_t file_id,
    size_t begin,
    size_t end)
{
    vitte_ast_span_t span = {0};

    span.file_id = file_id;
    span.begin = begin;
    span.end = end;

    return span;
}

bool
vitte_ast_span_is_valid(
    vitte_ast_span_t span)
{
    return span.begin <= span.end;
}

size_t
vitte_ast_span_length(
    vitte_ast_span_t span)
{
    if (!vitte_ast_span_is_valid(span)) {
        return 0u;
    }

    return span.end - span.begin;
}

vitte_ast_span_t
vitte_ast_span_join(
    vitte_ast_span_t left,
    vitte_ast_span_t right)
{
    vitte_ast_span_t result = {0};

    if (!vitte_ast_span_is_valid(left)) {
        return right;
    }

    if (!vitte_ast_span_is_valid(right)) {
        return left;
    }

    /*
     * Spans from different source files cannot be represented by one
     * vitte_ast_span_t. Preserve the left operand deterministically.
     */
    if (left.file_id != right.file_id) {
        return left;
    }

    result.file_id = left.file_id;
    result.begin =
        left.begin < right.begin
            ? left.begin
            : right.begin;
    result.end =
        left.end > right.end
            ? left.end
            : right.end;

    return result;
}

/* ========================================================================= */
/* AST validity                                                              */
/* ========================================================================= */

bool
vitte_ast_is_valid(
    const vitte_ast_t *ast)
{
    if (ast == NULL ||
        ast->magic != VITTE_AST_MAGIC ||
        ast->arena == NULL) {
        return false;
    }

    if (!vitte_arena_context_is_valid(
            ast->arena)) {
        return false;
    }

    if (ast->generation == UINT64_C(0) ||
        ast->next_node_id == UINT64_C(0)) {
        return false;
    }

    if (ast->node_count >
        ast->peak_node_count) {
        return false;
    }

    return true;
}

bool
vitte_ast_node_is_valid(
    const vitte_ast_node_t *node)
{
    const vitte_ast_t *owner;

    if (node == NULL ||
        node->magic != VITTE_AST_NODE_MAGIC ||
        node->owner == NULL ||
        node->kind == VITTE_AST_INVALID) {
        return false;
    }

    owner = node->owner;

    if (!vitte_ast_is_valid(owner)) {
        return false;
    }

    if (node->generation !=
        owner->generation) {
        return false;
    }

    if (!vitte_ast_kind_is_known(
            node->kind)) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* AST lifecycle                                                             */
/* ========================================================================= */

bool
vitte_ast_init(
    vitte_ast_t *ast,
    vitte_arena_context_t *arena)
{
    if (ast == NULL ||
        arena == NULL ||
        !vitte_arena_context_is_valid(arena)) {
        return false;
    }

    /*
     * The caller must not reinitialize a live AST without first destroying
     * it. Reading ast before initialization would itself be unsafe when the
     * object contains indeterminate storage, therefore initialization starts
     * from a complete overwrite.
     */
    memset(
        ast,
        0,
        sizeof(*ast));

    ast->magic = VITTE_AST_MAGIC;
    ast->arena = arena;

    ast->root = NULL;

    ast->next_node_id = UINT64_C(1);

    ast->node_count = 0u;
    ast->peak_node_count = 0u;

    ast->generation = UINT64_C(1);

    ast->error_count = 0u;

    return true;
}

void
vitte_ast_destroy(
    vitte_ast_t *ast)
{
    if (ast == NULL ||
        ast->magic != VITTE_AST_MAGIC) {
        return;
    }

    /*
     * Do not inspect the external arena here. Destruction of coordinator
     * metadata should still work if the external arena has already been
     * destroyed.
     */
    ast->root = NULL;
    ast->arena = NULL;

    ast->node_count = 0u;
    ast->peak_node_count = 0u;

    ast->error_count = 0u;

    ast->generation = UINT64_C(0);
    ast->next_node_id = UINT64_C(0);

    ast->magic = VITTE_AST_DEAD_MAGIC;
}

void
vitte_ast_reset(
    vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return;
    }

    ast->root = NULL;

    ast->node_count = 0u;
    ast->error_count = 0u;

    ast->generation =
        vitte_ast_next_generation(
            ast->generation);

    /*
     * Node IDs deliberately remain monotonic across logical AST resets.
     *
     * Existing nodes are rejected by generation mismatch while their IDs
     * remain useful in logs and diagnostics.
     */
}

/* ========================================================================= */
/* Arena helpers                                                             */
/* ========================================================================= */

static void *
vitte_ast_alloc_bytes(
    vitte_ast_t *ast,
    size_t size,
    size_t alignment)
{
    if (!vitte_ast_is_valid(ast) ||
        size == 0u ||
        alignment == 0u ||
        !vitte_ast_is_power_of_two(alignment)) {
        return NULL;
    }

    return
        vitte_arena_context_alloc_aligned(
            ast->arena,
            size,
            alignment);
}

static void *
vitte_ast_alloc_array(
    vitte_ast_t *ast,
    size_t count,
    size_t element_size,
    size_t alignment)
{
    size_t bytes;

    if (!vitte_ast_is_valid(ast) ||
        count == 0u ||
        element_size == 0u) {
        return NULL;
    }

    if (vitte_ast_mul_overflow(
            count,
            element_size,
            &bytes)) {
        return NULL;
    }

    return
        vitte_ast_alloc_bytes(
            ast,
            bytes,
            alignment);
}

static char *
vitte_ast_copy_string(
    vitte_ast_t *ast,
    const char *string,
    size_t length)
{
    char *copy;
    size_t bytes;

    if (!vitte_ast_is_valid(ast) ||
        string == NULL) {
        return NULL;
    }

    if (vitte_ast_add_overflow(
            length,
            1u,
            &bytes)) {
        return NULL;
    }

    copy =
        (char *)vitte_ast_alloc_bytes(
            ast,
            bytes,
            _Alignof(char));

    if (copy == NULL) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(
            copy,
            string,
            length);
    }

    copy[length] = '\0';

    return copy;
}

/* ========================================================================= */
/* Node creation                                                             */
/* ========================================================================= */

vitte_ast_node_t *
vitte_ast_node_create(
    vitte_ast_t *ast,
    vitte_ast_kind_t kind,
    vitte_ast_span_t span)
{
    vitte_ast_node_t *node;

    if (!vitte_ast_is_valid(ast) ||
        kind == VITTE_AST_INVALID ||
        !vitte_ast_kind_is_known(kind) ||
        !vitte_ast_span_is_valid(span)) {
        return NULL;
    }

    /*
     * UINT64_MAX itself remains usable as an ID only if we can subsequently
     * represent the next ID. We choose the stronger invariant:
     *
     *     next_node_id is always a valid future non-zero ID.
     *
     * Therefore UINT64_MAX is reserved as exhaustion.
     */
    if (ast->next_node_id == UINT64_MAX ||
        ast->node_count == SIZE_MAX) {
        return NULL;
    }

    node =
        (vitte_ast_node_t *)
            vitte_ast_alloc_bytes(
                ast,
                sizeof(*node),
                _Alignof(vitte_ast_node_t));

    if (node == NULL) {
        return NULL;
    }

    memset(
        node,
        0,
        sizeof(*node));

    node->magic = VITTE_AST_NODE_MAGIC;
    node->owner = ast;

    node->id = ast->next_node_id;
    node->generation = ast->generation;
    node->kind = kind;
    node->span = span;

    node->parent = NULL;

    node->children = NULL;
    node->child_count = 0u;
    node->child_capacity = 0u;

    node->attributes = NULL;
    node->attribute_count = 0u;
    node->attribute_capacity = 0u;

    node->flags = VITTE_AST_FLAG_NONE;

    node->name = NULL;
    node->name_length = 0u;

    node->semantic = NULL;

    ++ast->next_node_id;
    ++ast->node_count;

    if (ast->node_count >
        ast->peak_node_count) {
        ast->peak_node_count =
            ast->node_count;
    }

    return node;
}

/* ========================================================================= */
/* Named nodes                                                               */
/* ========================================================================= */

vitte_ast_node_t *
vitte_ast_node_create_named(
    vitte_ast_t *ast,
    vitte_ast_kind_t kind,
    vitte_ast_span_t span,
    const char *name,
    size_t name_length)
{
    vitte_ast_node_t *node;
    char *copy;

    if (!vitte_ast_is_valid(ast) ||
        name == NULL) {
        return NULL;
    }

    /*
     * Allocate the string first.
     *
     * Arena allocations cannot be individually rolled back here. Allocating
     * the name before publishing a node avoids creating a counted node whose
     * initialization subsequently fails.
     */
    copy =
        vitte_ast_copy_string(
            ast,
            name,
            name_length);

    if (copy == NULL) {
        return NULL;
    }

    node =
        vitte_ast_node_create(
            ast,
            kind,
            span);

    if (node == NULL) {
        /*
         * copy remains arena-owned unused storage. This is acceptable for a
         * monotonic allocator and avoids corrupting AST logical accounting.
         */
        return NULL;
    }

    node->name = copy;
    node->name_length = name_length;

    return node;
}

/* ========================================================================= */
/* Child storage                                                             */
/* ========================================================================= */

static bool
vitte_ast_node_reserve_children(
    vitte_ast_node_t *node,
    size_t minimum)
{
    vitte_ast_node_t **storage;
    size_t capacity;
    size_t copy_bytes;

    if (!vitte_ast_node_is_valid(node)) {
        return false;
    }

    if (minimum <=
        node->child_capacity) {
        return true;
    }

    capacity = node->child_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_AST_INITIAL_CHILD_CAPACITY;
    }

    if (capacity < minimum) {
        while (capacity < minimum) {
            if (capacity >
                SIZE_MAX / 2u) {
                capacity = minimum;
                break;
            }

            capacity *= 2u;
        }
    }

    if (capacity < minimum ||
        capacity == 0u) {
        return false;
    }

    storage =
        (vitte_ast_node_t **)
            vitte_ast_alloc_array(
                node->owner,
                capacity,
                sizeof(*storage),
                _Alignof(vitte_ast_node_t *));

    if (storage == NULL) {
        return false;
    }

    /*
     * Zero all capacity. Besides deterministic state this guarantees unused
     * entries are NULL and makes structural diagnostics substantially safer.
     */
    memset(
        storage,
        0,
        capacity * sizeof(*storage));

    if (node->child_count != 0u) {
        if (node->children == NULL ||
            vitte_ast_mul_overflow(
                node->child_count,
                sizeof(*storage),
                &copy_bytes)) {
            return false;
        }

        memcpy(
            storage,
            node->children,
            copy_bytes);
    }

    /*
     * Publish only after successful construction.
     */
    node->children = storage;
    node->child_capacity = capacity;

    return true;
}

/* ========================================================================= */
/* Parent-chain validation                                                   */
/* ========================================================================= */

static bool
vitte_ast_parent_chain_contains(
    const vitte_ast_node_t *start,
    const vitte_ast_node_t *target)
{
    const vitte_ast_node_t *cursor;
    size_t depth;

    if (start == NULL ||
        target == NULL) {
        return false;
    }

    cursor = start;
    depth = 0u;

    while (cursor != NULL) {
        if (!vitte_ast_node_is_valid(cursor)) {
            /*
             * Treat structural corruption conservatively as an unsafe chain.
             */
            return true;
        }

        if (cursor == target) {
            return true;
        }

        cursor = cursor->parent;

        if (depth == VITTE_AST_MAX_DEPTH) {
            return true;
        }

        ++depth;
    }

    return false;
}

static bool
vitte_ast_would_create_cycle(
    const vitte_ast_node_t *parent,
    const vitte_ast_node_t *child)
{
    if (!vitte_ast_node_is_valid(parent) ||
        !vitte_ast_node_is_valid(child)) {
        return true;
    }

    if (parent->owner != child->owner) {
        return true;
    }

    if (parent == child) {
        return true;
    }

    return
        vitte_ast_parent_chain_contains(
            parent,
            child);
}

/* ========================================================================= */
/* Children                                                                  */
/* ========================================================================= */

bool
vitte_ast_node_add_child(
    vitte_ast_node_t *parent,
    vitte_ast_node_t *child)
{
    size_t minimum;

    if (!vitte_ast_node_is_valid(parent) ||
        !vitte_ast_node_is_valid(child)) {
        return false;
    }

    if (parent->owner != child->owner ||
        parent->generation != child->generation) {
        return false;
    }

    if (child->parent != NULL) {
        return false;
    }

    if (vitte_ast_would_create_cycle(
            parent,
            child)) {
        return false;
    }

    if (vitte_ast_add_overflow(
            parent->child_count,
            1u,
            &minimum)) {
        return false;
    }

    if (!vitte_ast_node_reserve_children(
            parent,
            minimum)) {
        return false;
    }

    parent->children[parent->child_count] =
        child;

    ++parent->child_count;

    child->parent = parent;

    return true;
}

bool
vitte_ast_node_insert_child(
    vitte_ast_node_t *parent,
    size_t index,
    vitte_ast_node_t *child)
{
    size_t minimum;
    size_t move_count;
    size_t move_bytes;

    if (!vitte_ast_node_is_valid(parent) ||
        !vitte_ast_node_is_valid(child) ||
        index > parent->child_count) {
        return false;
    }

    if (parent->owner != child->owner ||
        parent->generation != child->generation ||
        child->parent != NULL ||
        vitte_ast_would_create_cycle(
            parent,
            child)) {
        return false;
    }

    if (vitte_ast_add_overflow(
            parent->child_count,
            1u,
            &minimum)) {
        return false;
    }

    if (!vitte_ast_node_reserve_children(
            parent,
            minimum)) {
        return false;
    }

    move_count =
        parent->child_count - index;

    if (move_count != 0u) {
        if (vitte_ast_mul_overflow(
                move_count,
                sizeof(parent->children[0]),
                &move_bytes)) {
            return false;
        }

        memmove(
            &parent->children[index + 1u],
            &parent->children[index],
            move_bytes);
    }

    parent->children[index] = child;
    ++parent->child_count;

    child->parent = parent;

    return true;
}

vitte_ast_node_t *
vitte_ast_node_child(
    vitte_ast_node_t *node,
    size_t index)
{
    vitte_ast_node_t *child;

    if (!vitte_ast_node_is_valid(node) ||
        index >= node->child_count ||
        node->children == NULL) {
        return NULL;
    }

    child = node->children[index];

    if (!vitte_ast_node_is_valid(child) ||
        child->owner != node->owner ||
        child->parent != node) {
        return NULL;
    }

    return child;
}

const vitte_ast_node_t *
vitte_ast_node_child_const(
    const vitte_ast_node_t *node,
    size_t index)
{
    const vitte_ast_node_t *child;

    if (!vitte_ast_node_is_valid(node) ||
        index >= node->child_count ||
        node->children == NULL) {
        return NULL;
    }

    child = node->children[index];

    if (!vitte_ast_node_is_valid(child) ||
        child->owner != node->owner ||
        child->parent != node) {
        return NULL;
    }

    return child;
}

size_t
vitte_ast_node_child_count(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return 0u;
    }

    return node->child_count;
}

/* ========================================================================= */
/* Replace child                                                             */
/* ========================================================================= */

bool
vitte_ast_node_replace_child(
    vitte_ast_node_t *parent,
    size_t index,
    vitte_ast_node_t *replacement)
{
    vitte_ast_node_t *old;

    if (!vitte_ast_node_is_valid(parent) ||
        !vitte_ast_node_is_valid(replacement) ||
        index >= parent->child_count ||
        parent->children == NULL) {
        return false;
    }

    if (parent->owner != replacement->owner ||
        parent->generation != replacement->generation) {
        return false;
    }

    old = parent->children[index];

    if (old == NULL ||
        !vitte_ast_node_is_valid(old) ||
        old->owner != parent->owner ||
        old->parent != parent) {
        return false;
    }

    if (old == replacement) {
        return true;
    }

    if (replacement->parent != NULL) {
        return false;
    }

    if (vitte_ast_would_create_cycle(
            parent,
            replacement)) {
        return false;
    }

    /*
     * Both pointer writes are non-failing. Publish replacement first and then
     * detach the old child. At no point is replacement externally visible as
     * a child without a matching parent once this function returns.
     */
    parent->children[index] = replacement;
    replacement->parent = parent;
    old->parent = NULL;

    return true;
}

/* ========================================================================= */
/* Detach child                                                              */
/* ========================================================================= */

vitte_ast_node_t *
vitte_ast_node_detach_child(
    vitte_ast_node_t *parent,
    size_t index)
{
    vitte_ast_node_t *child;
    size_t remaining;
    size_t move_bytes;

    if (!vitte_ast_node_is_valid(parent) ||
        index >= parent->child_count ||
        parent->children == NULL) {
        return NULL;
    }

    child = parent->children[index];

    if (child == NULL ||
        !vitte_ast_node_is_valid(child) ||
        child->owner != parent->owner ||
        child->parent != parent) {
        return NULL;
    }

    remaining =
        parent->child_count -
        index -
        1u;

    if (remaining != 0u) {
        if (vitte_ast_mul_overflow(
                remaining,
                sizeof(parent->children[0]),
                &move_bytes)) {
            return NULL;
        }

        memmove(
            &parent->children[index],
            &parent->children[index + 1u],
            move_bytes);
    }

    --parent->child_count;

    parent->children[parent->child_count] =
        NULL;

    child->parent = NULL;

    return child;
}

/* ========================================================================= */
/* Attribute storage                                                         */
/* ========================================================================= */

static bool
vitte_ast_node_reserve_attributes(
    vitte_ast_node_t *node,
    size_t minimum)
{
    vitte_ast_attribute_t *storage;
    size_t capacity;
    size_t storage_bytes;
    size_t copy_bytes;

    if (!vitte_ast_node_is_valid(node)) {
        return false;
    }

    if (minimum <=
        node->attribute_capacity) {
        return true;
    }

    capacity = node->attribute_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_AST_INITIAL_ATTRIBUTE_CAPACITY;
    }

    while (capacity < minimum) {
        if (capacity >
            SIZE_MAX / 2u) {
            capacity = minimum;
            break;
        }

        capacity *= 2u;
    }

    if (capacity < minimum ||
        capacity == 0u ||
        vitte_ast_mul_overflow(
            capacity,
            sizeof(*storage),
            &storage_bytes)) {
        return false;
    }

    storage =
        (vitte_ast_attribute_t *)
            vitte_ast_alloc_bytes(
                node->owner,
                storage_bytes,
                _Alignof(vitte_ast_attribute_t));

    if (storage == NULL) {
        return false;
    }

    memset(
        storage,
        0,
        storage_bytes);

    if (node->attribute_count != 0u) {
        if (node->attributes == NULL ||
            vitte_ast_mul_overflow(
                node->attribute_count,
                sizeof(*storage),
                &copy_bytes)) {
            return false;
        }

        memcpy(
            storage,
            node->attributes,
            copy_bytes);
    }

    node->attributes = storage;
    node->attribute_capacity = capacity;

    return true;
}

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
    vitte_ast_span_t span)
{
    vitte_ast_attribute_t *attribute;
    char *name_copy;
    char *value_copy;
    size_t minimum;

    if (!vitte_ast_node_is_valid(node) ||
        name == NULL ||
        !vitte_ast_span_is_valid(span)) {
        return false;
    }

    if (value == NULL &&
        value_length != 0u) {
        return false;
    }

    if (vitte_ast_add_overflow(
            node->attribute_count,
            1u,
            &minimum)) {
        return false;
    }

    /*
     * Allocate immutable payload first. This prevents publication of a
     * partially initialized attribute.
     */
    name_copy =
        vitte_ast_copy_string(
            node->owner,
            name,
            name_length);

    if (name_copy == NULL) {
        return false;
    }

    value_copy = NULL;

    if (value != NULL) {
        value_copy =
            vitte_ast_copy_string(
                node->owner,
                value,
                value_length);

        if (value_copy == NULL) {
            return false;
        }
    }

    if (!vitte_ast_node_reserve_attributes(
            node,
            minimum)) {
        return false;
    }

    attribute =
        &node->attributes[
            node->attribute_count];

    memset(
        attribute,
        0,
        sizeof(*attribute));

    attribute->name = name_copy;
    attribute->name_length = name_length;

    attribute->value = value_copy;
    attribute->value_length =
        value != NULL
            ? value_length
            : 0u;

    attribute->span = span;

    ++node->attribute_count;

    return true;
}

const vitte_ast_attribute_t *
vitte_ast_node_attribute(
    const vitte_ast_node_t *node,
    size_t index)
{
    if (!vitte_ast_node_is_valid(node) ||
        node->attributes == NULL ||
        index >= node->attribute_count) {
        return NULL;
    }

    return
        &node->attributes[index];
}

const vitte_ast_attribute_t *
vitte_ast_node_find_attribute(
    const vitte_ast_node_t *node,
    const char *name,
    size_t name_length)
{
    size_t index;

    if (!vitte_ast_node_is_valid(node) ||
        name == NULL ||
        (node->attribute_count != 0u &&
         node->attributes == NULL)) {
        return NULL;
    }

    for (index = 0u;
         index < node->attribute_count;
         ++index) {
        const vitte_ast_attribute_t *attribute;

        attribute =
            &node->attributes[index];

        if (attribute->name == NULL ||
            attribute->name_length !=
                name_length) {
            continue;
        }

        if (name_length == 0u ||
            memcmp(
                attribute->name,
                name,
                name_length) == 0) {
            return attribute;
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Flags                                                                     */
/* ========================================================================= */

void
vitte_ast_node_set_flag(
    vitte_ast_node_t *node,
    vitte_ast_flags_t flag)
{
    if (!vitte_ast_node_is_valid(node)) {
        return;
    }

    node->flags |= flag;
}

void
vitte_ast_node_clear_flag(
    vitte_ast_node_t *node,
    vitte_ast_flags_t flag)
{
    if (!vitte_ast_node_is_valid(node)) {
        return;
    }

    node->flags &= ~flag;
}

bool
vitte_ast_node_has_flag(
    const vitte_ast_node_t *node,
    vitte_ast_flags_t flag)
{
    if (!vitte_ast_node_is_valid(node)) {
        return false;
    }

    return
        (node->flags & flag) == flag;
}

/* ========================================================================= */
/* Root                                                                      */
/* ========================================================================= */

bool
vitte_ast_set_root(
    vitte_ast_t *ast,
    vitte_ast_node_t *root)
{
    if (!vitte_ast_is_valid(ast) ||
        !vitte_ast_node_is_valid(root) ||
        root->owner != ast ||
        root->generation != ast->generation ||
        root->parent != NULL) {
        return false;
    }

    /*
     * Replacing the root is permitted. The old root remains an arena-owned
     * detached tree and therefore vitte_ast_validate() will intentionally
     * fail until the AST's logical node accounting/tree is reconciled.
     */
    ast->root = root;

    return true;
}

vitte_ast_node_t *
vitte_ast_root(
    vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return NULL;
    }

    if (ast->root != NULL &&
        !vitte_ast_node_belongs_to(
            ast,
            ast->root)) {
        return NULL;
    }

    return ast->root;
}

const vitte_ast_node_t *
vitte_ast_root_const(
    const vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return NULL;
    }

    if (ast->root != NULL &&
        !vitte_ast_node_belongs_to(
            ast,
            ast->root)) {
        return NULL;
    }

    return ast->root;
}

/* ========================================================================= */
/* Basic accessors                                                           */
/* ========================================================================= */

uint64_t
vitte_ast_node_id(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return UINT64_C(0);
    }

    return node->id;
}

vitte_ast_kind_t
vitte_ast_node_kind(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return VITTE_AST_INVALID;
    }

    return node->kind;
}

vitte_ast_span_t
vitte_ast_node_span(
    const vitte_ast_node_t *node)
{
    vitte_ast_span_t empty = {0};

    empty.file_id = UINT32_C(0);
    empty.begin = 0u;
    empty.end = 0u;

    if (!vitte_ast_node_is_valid(node)) {
        return empty;
    }

    return node->span;
}

vitte_ast_node_t *
vitte_ast_node_parent(
    vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return NULL;
    }

    if (node->parent != NULL &&
        (!vitte_ast_node_is_valid(node->parent) ||
         node->parent->owner != node->owner)) {
        return NULL;
    }

    return node->parent;
}

const vitte_ast_node_t *
vitte_ast_node_parent_const(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return NULL;
    }

    if (node->parent != NULL &&
        (!vitte_ast_node_is_valid(node->parent) ||
         node->parent->owner != node->owner)) {
        return NULL;
    }

    return node->parent;
}

const char *
vitte_ast_node_name(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return NULL;
    }

    return node->name;
}

size_t
vitte_ast_node_name_length(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return 0u;
    }

    return node->name_length;
}

/* ========================================================================= */
/* Semantic attachment                                                       */
/* ========================================================================= */

void
vitte_ast_node_set_semantic(
    vitte_ast_node_t *node,
    void *semantic)
{
    if (!vitte_ast_node_is_valid(node)) {
        return;
    }

    node->semantic = semantic;
}

void *
vitte_ast_node_semantic(
    vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return NULL;
    }

    return node->semantic;
}

const void *
vitte_ast_node_semantic_const(
    const vitte_ast_node_t *node)
{
    if (!vitte_ast_node_is_valid(node)) {
        return NULL;
    }

    return node->semantic;
}

/* ========================================================================= */
/* Literal values                                                            */
/* ========================================================================= */

void
vitte_ast_node_set_integer(
    vitte_ast_node_t *node,
    uint64_t value,
    bool negative)
{
    if (!vitte_ast_node_is_valid(node)) {
        return;
    }

    node->data.integer.value = value;
    node->data.integer.negative = negative;
}

void
vitte_ast_node_set_float(
    vitte_ast_node_t *node,
    double value)
{
    if (!vitte_ast_node_is_valid(node)) {
        return;
    }

    node->data.floating.value = value;
}

bool
vitte_ast_node_set_string(
    vitte_ast_node_t *node,
    const char *value,
    size_t length)
{
    char *copy;

    if (!vitte_ast_node_is_valid(node) ||
        value == NULL) {
        return false;
    }

    copy =
        vitte_ast_copy_string(
            node->owner,
            value,
            length);

    if (copy == NULL) {
        return false;
    }

    node->data.string.value = copy;
    node->data.string.length = length;

    return true;
}

void
vitte_ast_node_set_bool(
    vitte_ast_node_t *node,
    bool value)
{
    if (!vitte_ast_node_is_valid(node)) {
        return;
    }

    node->data.boolean.value = value;
}

/* ========================================================================= */
/* Operators                                                                 */
/* ========================================================================= */

void
vitte_ast_node_set_unary_operator(
    vitte_ast_node_t *node,
    vitte_ast_unary_operator_t op)
{
    if (!vitte_ast_node_is_valid(node)) {
        return;
    }

    node->data.unary.op = op;
}

void
vitte_ast_node_set_binary_operator(
    vitte_ast_node_t *node,
    vitte_ast_binary_operator_t op)
{
    if (!vitte_ast_node_is_valid(node)) {
        return;
    }

    node->data.binary.op = op;
}

/* ========================================================================= */
/* Search by ID                                                              */
/* ========================================================================= */

static vitte_ast_node_t *
vitte_ast_find_id_recursive(
    vitte_ast_node_t *node,
    uint64_t id,
    size_t depth)
{
    size_t index;

    if (!vitte_ast_node_is_valid(node) ||
        depth > VITTE_AST_MAX_DEPTH) {
        return NULL;
    }

    if (node->id == id) {
        return node;
    }

    if (node->child_count != 0u &&
        node->children == NULL) {
        return NULL;
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        vitte_ast_node_t *found;

        found =
            vitte_ast_find_id_recursive(
                node->children[index],
                id,
                depth + 1u);

        if (found != NULL) {
            return found;
        }
    }

    return NULL;
}

vitte_ast_node_t *
vitte_ast_find_id(
    vitte_ast_t *ast,
    uint64_t id)
{
    if (!vitte_ast_is_valid(ast) ||
        ast->root == NULL ||
        id == UINT64_C(0)) {
        return NULL;
    }

    /*
     * IDs not yet issued cannot exist in the current AST or in detached
     * current-generation trees.
     */
    if (id >= ast->next_node_id) {
        return NULL;
    }

    return
        vitte_ast_find_id_recursive(
            ast->root,
            id,
            0u);
}

/* ========================================================================= */
/* Traversal                                                                 */
/* ========================================================================= */

static bool
vitte_ast_visit_recursive(
    vitte_ast_node_t *node,
    vitte_ast_visit_fn visitor,
    void *user_data,
    size_t depth)
{
    size_t index;

    if (!vitte_ast_node_is_valid(node) ||
        visitor == NULL ||
        depth > VITTE_AST_MAX_DEPTH) {
        return false;
    }

    if (node->child_count != 0u &&
        node->children == NULL) {
        return false;
    }

    if (!visitor(
            node,
            VITTE_AST_VISIT_ENTER,
            depth,
            user_data)) {
        return false;
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        vitte_ast_node_t *child;

        child = node->children[index];

        if (!vitte_ast_node_is_valid(child) ||
            child->owner != node->owner ||
            child->parent != node) {
            return false;
        }

        if (!vitte_ast_visit_recursive(
                child,
                visitor,
                user_data,
                depth + 1u)) {
            return false;
        }
    }

    return
        visitor(
            node,
            VITTE_AST_VISIT_LEAVE,
            depth,
            user_data);
}

bool
vitte_ast_visit(
    vitte_ast_t *ast,
    vitte_ast_visit_fn visitor,
    void *user_data)
{
    if (!vitte_ast_is_valid(ast) ||
        visitor == NULL) {
        return false;
    }

    if (ast->root == NULL) {
        return true;
    }

    if (!vitte_ast_node_belongs_to(
            ast,
            ast->root) ||
        ast->root->parent != NULL) {
        return false;
    }

    return
        vitte_ast_visit_recursive(
            ast->root,
            visitor,
            user_data,
            0u);
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

static bool
vitte_ast_collect_stats_recursive(
    const vitte_ast_t *ast,
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *expected_parent,
    size_t depth,
    vitte_ast_stats_t *stats)
{
    size_t index;

    if (ast == NULL ||
        node == NULL ||
        stats == NULL ||
        depth > VITTE_AST_MAX_DEPTH) {
        return false;
    }

    if (!vitte_ast_node_belongs_to(
            ast,
            node) ||
        node->parent != expected_parent) {
        return false;
    }

    if (node->child_count >
        node->child_capacity ||
        (node->child_count != 0u &&
         node->children == NULL)) {
        return false;
    }

    stats->reachable_nodes =
        vitte_ast_saturating_add(
            stats->reachable_nodes,
            1u);

    if (depth >
        stats->maximum_depth) {
        stats->maximum_depth = depth;
    }

    stats->child_edges =
        vitte_ast_saturating_add(
            stats->child_edges,
            node->child_count);

    stats->attribute_count =
        vitte_ast_saturating_add(
            stats->attribute_count,
            node->attribute_count);

    if (vitte_ast_kind_is_declaration(
            node->kind)) {
        stats->declaration_count =
            vitte_ast_saturating_add(
                stats->declaration_count,
                1u);
    }

    if (vitte_ast_kind_is_statement(
            node->kind)) {
        stats->statement_count =
            vitte_ast_saturating_add(
                stats->statement_count,
                1u);
    }

    if (vitte_ast_kind_is_expression(
            node->kind)) {
        stats->expression_count =
            vitte_ast_saturating_add(
                stats->expression_count,
                1u);
    }

    if (vitte_ast_kind_is_type(
            node->kind)) {
        stats->type_count =
            vitte_ast_saturating_add(
                stats->type_count,
                1u);
    }

    if (vitte_ast_kind_is_pattern(
            node->kind)) {
        stats->pattern_count =
            vitte_ast_saturating_add(
                stats->pattern_count,
                1u);
    }

    if ((node->flags &
         VITTE_AST_FLAG_INVALID) != 0u) {
        stats->invalid_node_count =
            vitte_ast_saturating_add(
                stats->invalid_node_count,
                1u);
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        const vitte_ast_node_t *child;

        child = node->children[index];

        if (!vitte_ast_collect_stats_recursive(
                ast,
                child,
                node,
                depth + 1u,
                stats)) {
            return false;
        }
    }

    return true;
}

vitte_ast_stats_t
vitte_ast_stats(
    const vitte_ast_t *ast)
{
    vitte_ast_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_ast_is_valid(ast)) {
        return stats;
    }

    stats.node_count =
        ast->node_count;

    stats.peak_node_count =
        ast->peak_node_count;

    stats.generation =
        ast->generation;

    stats.error_count =
        ast->error_count;

    if (ast->root != NULL) {
        (void)vitte_ast_collect_stats_recursive(
            ast,
            ast->root,
            NULL,
            0u,
            &stats);
    }

    return stats;
}

/* ========================================================================= */
/* Validation helpers                                                        */
/* ========================================================================= */

static bool
vitte_ast_validate_attribute(
    const vitte_ast_attribute_t *attribute)
{
    if (attribute == NULL ||
        attribute->name == NULL ||
        !vitte_ast_span_is_valid(
            attribute->span)) {
        return false;
    }

    if (!vitte_ast_string_has_terminator(
            attribute->name,
            attribute->name_length)) {
        return false;
    }

    if (attribute->value == NULL) {
        if (attribute->value_length != 0u) {
            return false;
        }
    } else if (!vitte_ast_string_has_terminator(
                   attribute->value,
                   attribute->value_length)) {
        return false;
    }

    return true;
}

static bool
vitte_ast_validate_node_storage(
    const vitte_ast_t *ast,
    const vitte_ast_node_t *node)
{
    size_t index;

    if (!vitte_ast_node_belongs_to(
            ast,
            node)) {
        return false;
    }

    if (node->kind == VITTE_AST_INVALID ||
        !vitte_ast_kind_is_known(
            node->kind)) {
        return false;
    }

    if (!vitte_ast_span_is_valid(
            node->span)) {
        return false;
    }

    if (node->id == UINT64_C(0) ||
        node->id >= ast->next_node_id) {
        return false;
    }

    if (node->child_count >
        node->child_capacity) {
        return false;
    }

    if (node->child_capacity == 0u) {
        if (node->children != NULL ||
            node->child_count != 0u) {
            return false;
        }
    } else if (node->children == NULL) {
        return false;
    }

    if (node->attribute_count >
        node->attribute_capacity) {
        return false;
    }

    if (node->attribute_capacity == 0u) {
        if (node->attributes != NULL ||
            node->attribute_count != 0u) {
            return false;
        }
    } else if (node->attributes == NULL) {
        return false;
    }

    if (node->name == NULL) {
        if (node->name_length != 0u) {
            return false;
        }
    } else if (!vitte_ast_string_has_terminator(
                   node->name,
                   node->name_length)) {
        return false;
    }

    for (index = 0u;
         index < node->attribute_count;
         ++index) {
        if (!vitte_ast_validate_attribute(
                &node->attributes[index])) {
            return false;
        }
    }

    /*
     * Storage beyond the logical child count is expected to remain NULL.
     * reserve_children() guarantees this invariant.
     */
    for (index = node->child_count;
         index < node->child_capacity;
         ++index) {
        if (node->children[index] != NULL) {
            return false;
        }
    }

    return true;
}

/*
 * Detect repeated IDs/pointers without allocating a side table.
 *
 * Tree parent invariants already prevent a normal child from belonging to
 * multiple parents. A complete duplicate-ID check is nevertheless useful
 * because IDs are intended to be globally unique within one AST generation.
 *
 * This helper searches only the already reachable subtree and is therefore
 * O(n) per lookup. Deep validation is intentionally allowed to be expensive.
 */
static bool
vitte_ast_subtree_contains_other_id(
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *self,
    uint64_t id,
    size_t depth)
{
    size_t index;

    if (node == NULL ||
        depth > VITTE_AST_MAX_DEPTH) {
        return false;
    }

    if (node != self &&
        node->id == id) {
        return true;
    }

    if (node->children == NULL) {
        return false;
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        if (vitte_ast_subtree_contains_other_id(
                node->children[index],
                self,
                id,
                depth + 1u)) {
            return true;
        }
    }

    return false;
}

static bool
vitte_ast_validate_node_recursive(
    const vitte_ast_t *ast,
    const vitte_ast_node_t *root,
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *expected_parent,
    size_t depth,
    size_t *reachable)
{
    size_t index;

    if (ast == NULL ||
        root == NULL ||
        node == NULL ||
        reachable == NULL ||
        depth > VITTE_AST_MAX_DEPTH) {
        return false;
    }

    if (!vitte_ast_validate_node_storage(
            ast,
            node)) {
        return false;
    }

    if (node->parent !=
        expected_parent) {
        return false;
    }

    /*
     * A valid parent chain must terminate at the supplied root within the
     * configured maximum depth. This also rejects parent cycles.
     */
    if (node != root) {
        const vitte_ast_node_t *cursor;
        size_t parent_depth;

        cursor = node;
        parent_depth = 0u;

        while (cursor != root) {
            if (cursor == NULL ||
                cursor->parent == NULL ||
                !vitte_ast_node_belongs_to(
                    ast,
                    cursor->parent)) {
                return false;
            }

            cursor = cursor->parent;

            if (parent_depth ==
                VITTE_AST_MAX_DEPTH) {
                return false;
            }

            ++parent_depth;
        }
    } else if (node->parent != NULL) {
        return false;
    }

    /*
     * IDs are a global AST invariant, not merely a sibling invariant.
     */
    if (vitte_ast_subtree_contains_other_id(
            root,
            node,
            node->id,
            0u)) {
        return false;
    }

    if (*reachable == SIZE_MAX) {
        return false;
    }

    ++(*reachable);

    if (*reachable > ast->node_count) {
        /*
         * This catches cycles/shared-node corruption early without requiring
         * an auxiliary visited allocation.
         */
        return false;
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        const vitte_ast_node_t *child;
        size_t duplicate;

        child = node->children[index];

        if (child == NULL ||
            !vitte_ast_node_belongs_to(
                ast,
                child) ||
            child->parent != node) {
            return false;
        }

        if (child == node) {
            return false;
        }

        /*
         * Reject duplicate child references within one parent explicitly.
         */
        for (duplicate = index + 1u;
             duplicate < node->child_count;
             ++duplicate) {
            if (child ==
                node->children[duplicate]) {
                return false;
            }
        }

        if (!vitte_ast_validate_node_recursive(
                ast,
                root,
                child,
                node,
                depth + 1u,
                reachable)) {
            return false;
        }
    }

    return true;
}

bool
vitte_ast_validate(
    const vitte_ast_t *ast)
{
    size_t reachable;

    if (!vitte_ast_is_valid(ast)) {
        return false;
    }

    if (ast->generation == UINT64_C(0) ||
        ast->next_node_id == UINT64_C(0)) {
        return false;
    }

    if (ast->peak_node_count <
        ast->node_count) {
        return false;
    }

    /*
     * next_node_id begins at one and advances exactly once per successfully
     * created node over the AST object's lifetime. Since IDs survive reset,
     * node_count cannot be derived from it, but next_node_id must remain
     * strictly greater than every current node ID.
     */

    if (ast->root == NULL) {
        return ast->node_count == 0u;
    }

    if (ast->node_count == 0u) {
        return false;
    }

    if (!vitte_ast_node_belongs_to(
            ast,
            ast->root) ||
        ast->root->parent != NULL) {
        return false;
    }

    reachable = 0u;

    if (!vitte_ast_validate_node_recursive(
            ast,
            ast->root,
            ast->root,
            NULL,
            0u,
            &reachable)) {
        return false;
    }

    /*
     * The canonical finalized AST is a single rooted tree.
     *
     * Parser construction may temporarily create detached nodes, but deep
     * validation intentionally rejects that intermediate state.
     */
    if (reachable !=
        ast->node_count) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Error accounting                                                          */
/* ========================================================================= */

void
vitte_ast_record_error(
    vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return;
    }

    if (ast->error_count != SIZE_MAX) {
        ++ast->error_count;
    }
}

size_t
vitte_ast_error_count(
    const vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return 0u;
    }

    return ast->error_count;
}

/* ========================================================================= */
/* AST accessors                                                             */
/* ========================================================================= */

size_t
vitte_ast_node_count(
    const vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return 0u;
    }

    return ast->node_count;
}

size_t
vitte_ast_peak_node_count(
    const vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return 0u;
    }

    return ast->peak_node_count;
}

uint64_t
vitte_ast_generation(
    const vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return UINT64_C(0);
    }

    return ast->generation;
}

vitte_arena_context_t *
vitte_ast_arena(
    vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return NULL;
    }

    return ast->arena;
}

const vitte_arena_context_t *
vitte_ast_arena_const(
    const vitte_ast_t *ast)
{
    if (!vitte_ast_is_valid(ast)) {
        return NULL;
    }

    return ast->arena;
}

/* ========================================================================= */
/* Ancestor utilities                                                        */
/* ========================================================================= */

bool
vitte_ast_node_is_ancestor_of(
    const vitte_ast_node_t *ancestor,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *cursor;
    size_t depth;

    if (!vitte_ast_node_is_valid(ancestor) ||
        !vitte_ast_node_is_valid(node) ||
        ancestor->owner != node->owner ||
        ancestor->generation !=
            node->generation) {
        return false;
    }

    cursor = node->parent;
    depth = 0u;

    while (cursor != NULL) {
        if (!vitte_ast_node_is_valid(cursor) ||
            cursor->owner != node->owner) {
            return false;
        }

        if (cursor == ancestor) {
            return true;
        }

        cursor = cursor->parent;

        if (depth == VITTE_AST_MAX_DEPTH) {
            return false;
        }

        ++depth;
    }

    return false;
}

size_t
vitte_ast_node_depth(
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *cursor;
    const vitte_ast_t *owner;
    size_t depth;

    if (!vitte_ast_node_is_valid(node)) {
        return 0u;
    }

    owner = node->owner;
    cursor = node->parent;
    depth = 0u;

    while (cursor != NULL) {
        if (!vitte_ast_node_is_valid(cursor) ||
            cursor->owner != owner) {
            return SIZE_MAX;
        }

        if (depth == VITTE_AST_MAX_DEPTH) {
            return SIZE_MAX;
        }

        ++depth;
        cursor = cursor->parent;
    }

    return depth;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
