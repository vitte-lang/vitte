#include "references.inc"

/* Static diagnostics run after all declarations are typed, so forward calls
 * and contextual literal conversions see the same complete semantic model. */
static void
vitte_sema_contract_diagnostic(vitte_sema_t *context,
    vitte_sema_diagnostic_code_t code, const vitte_ast_node_t *site,
    const vitte_ast_node_t *clause, vitte_symbol_id_t procedure)
{
    if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
            code, site->span, site->id, procedure,
            VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
        vitte_sema_diagnostic_t *diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
        diagnostic->contract_flags = clause->flags;
        diagnostic->contract_span = clause->span;
        diagnostic->contract_symbol_id = procedure;
    }
}

static const vitte_ast_node_t *
vitte_sema_unsupported_constant_operation_private(
    const vitte_sema_t *context, const vitte_ast_node_t *node, size_t depth)
{
    size_t index;
    if (context == NULL || node == NULL || depth >= 128u) return NULL;
    if (node->kind == VITTE_AST_NODE_CALL_EXPR ||
        node->kind == VITTE_AST_NODE_ASSIGN_EXPR ||
        node->kind == VITTE_AST_NODE_AWAIT_EXPR ||
        node->kind == VITTE_AST_NODE_TRY_EXPR) return node;
    if (node->kind == VITTE_AST_NODE_IDENTIFIER || node->kind == VITTE_AST_NODE_PATH) {
        const vitte_sema_node_info_t *info = vitte_sema_get_node_info(context, node->id);
        const vitte_symbol_t *symbol = info != NULL
            ? vitte_scope_get_symbol(&context->scopes, info->symbol_id) : NULL;
        if (symbol != NULL && symbol->kind == VITTE_SYMBOL_KIND_LOCAL &&
            (symbol->flags & VITTE_SYMBOL_FLAG_MUTABLE) == 0u &&
            symbol->payload != NULL) {
            const vitte_ast_node_t *nested = vitte_sema_unsupported_constant_operation_private(
                context,
                vitte_sema_initializer(context,
                    (const vitte_ast_node_t *)symbol->payload),
                depth + 1u);
            if (nested != NULL) return nested;
        }
    }
    for (index = 0u; index < node->child_count; ++index) {
        const vitte_ast_node_t *unsupported = vitte_sema_unsupported_constant_operation_private(
            context, vitte_sema_node(context, node->children[index]), depth + 1u);
        if (unsupported != NULL) return unsupported;
    }
    return NULL;
}

/* Find the first source-level value read that cannot participate in a
 * compile-time initializer.  Keep the reference as the primary diagnostic
 * site; the referenced declaration is attached as related source evidence. */
static const vitte_ast_node_t *
vitte_sema_nonconstant_dependency_private(
    const vitte_sema_t *context, const vitte_ast_node_t *node,
    const vitte_symbol_t **dependency, size_t depth)
{
    size_t index;
    if (context == NULL || node == NULL || depth >= 128u) return NULL;
    if (node->kind == VITTE_AST_NODE_IDENTIFIER || node->kind == VITTE_AST_NODE_PATH) {
        const vitte_sema_node_info_t *info = vitte_sema_get_node_info(context, node->id);
        const vitte_symbol_t *symbol = info != NULL
            ? vitte_scope_get_symbol(&context->scopes, info->symbol_id) : NULL;
        if (symbol != NULL && symbol->kind != VITTE_SYMBOL_KIND_CONST &&
            (symbol->kind == VITTE_SYMBOL_KIND_STATIC ||
             symbol->kind == VITTE_SYMBOL_KIND_PARAMETER ||
             (symbol->flags & VITTE_SYMBOL_FLAG_MUTABLE) != 0u)) {
            if (dependency != NULL) *dependency = symbol;
            return node;
        }
        if (symbol != NULL && symbol->kind == VITTE_SYMBOL_KIND_LOCAL &&
            (symbol->flags & VITTE_SYMBOL_FLAG_MUTABLE) == 0u &&
            symbol->payload != NULL) {
            const vitte_ast_node_t *nested = vitte_sema_nonconstant_dependency_private(
                context,
                vitte_sema_initializer(context,
                    (const vitte_ast_node_t *)symbol->payload),
                dependency, depth + 1u);
            if (nested != NULL) return nested;
        }
    }
    for (index = 0u; index < node->child_count; ++index) {
        const vitte_ast_node_t *nested = vitte_sema_nonconstant_dependency_private(
            context, vitte_sema_node(context, node->children[index]),
            dependency, depth + 1u);
        if (nested != NULL) return nested;
    }
    return NULL;
}

static void
vitte_sema_check_call_contracts(vitte_sema_t *context, const vitte_ast_node_t *call)
{
    const vitte_sema_node_info_t *call_info = vitte_sema_get_node_info(context, call->id);
    const vitte_sema_node_info_t *callee_info;
    const vitte_sema_type_t *callee_type;
    const vitte_symbol_t *symbol;
    const vitte_ast_node_t *procedure;
    vitte_sema_eval_binding_t bindings[128];
    size_t binding_count = 0u, argument_index = 1u, index;
    if (call->child_count == 0u || call_info == NULL || call_info->type_id == context->type_error) return;
    callee_info = vitte_sema_get_node_info(context, call->children[0]);
    callee_type = callee_info != NULL ? vitte_sema_type_private(context, callee_info->type_id) : NULL;
    symbol = callee_type != NULL ? vitte_scope_get_symbol(&context->scopes, callee_type->symbol_id) : NULL;
    procedure = symbol != NULL ? (const vitte_ast_node_t *)symbol->payload : NULL;
    if (procedure == NULL || (procedure->kind != VITTE_AST_NODE_PROC_DECL &&
        procedure->kind != VITTE_AST_NODE_EXTERN_PROC_DECL)) return;
    for (index = 0u; index < procedure->child_count; ++index) {
        const vitte_ast_node_t *parameter = vitte_sema_node(context, procedure->children[index]);
        if (parameter == NULL || parameter->kind != VITTE_AST_NODE_PARAMETER) continue;
        if (binding_count >= 128u || argument_index >= call->child_count || parameter->child_count == 0u) return;
        bindings[binding_count].name = vitte_sema_node_token(context,
            vitte_sema_node(context, parameter->children[0]));
        bindings[binding_count].value = vitte_sema_known_value(context,
            vitte_sema_node(context, call->children[argument_index++]));
        ++binding_count;
    }
    if (argument_index != call->child_count) return;
    for (index = 0u; index < procedure->child_count; ++index) {
        const vitte_ast_node_t *clause = vitte_sema_node(context, procedure->children[index]);
        vitte_sema_eval_t eval;
        vitte_constant_t value, declaration_value;
        if (clause == NULL || (clause->flags & VITTE_AST_FLAG_CONTRACT_REQUIRES) == 0u) continue;
        /* A declaration-level contradiction already explains every call. */
        declaration_value = vitte_sema_known_value(context, clause);
        if (declaration_value.kind == VITTE_CONSTANT_KIND_BOOL && !declaration_value.as.boolean) continue;
        memset(&eval, 0, sizeof(eval));
        eval.sema = context;
        eval.bindings = bindings;
        eval.binding_count = binding_count;
        value = vitte_sema_eval_node(&eval, clause);
        if (value.kind == VITTE_CONSTANT_KIND_BOOL && !value.as.boolean)
            vitte_sema_contract_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_CONTRACT_PRECONDITION,
                call, clause, symbol->id);
    }
}

static void
vitte_sema_check_return_contracts(vitte_sema_t *context,
    const vitte_ast_node_t *node, const vitte_ast_node_t *procedure,
    vitte_symbol_id_t symbol, const bool *dead, size_t depth)
{
    size_t index;
    if (node == NULL || depth >= 128u || node->id == VITTE_AST_INVALID_ID ||
        node->id > context->parser->node_count || dead[node->id]) return;
    if (node->kind == VITTE_AST_NODE_PROC_DECL || node->kind == VITTE_AST_NODE_EXTERN_PROC_DECL) return;
    if (node->kind == VITTE_AST_NODE_MATCH_ARM && node->child_count >= 2u) {
        const vitte_ast_node_t *body = vitte_sema_node(context, node->children[node->child_count - 1u]);
        if (body != NULL && (body->kind == VITTE_AST_NODE_PATH ||
            body->kind == VITTE_AST_NODE_INTEGER_LITERAL || body->kind == VITTE_AST_NODE_BOOL_LITERAL ||
            body->kind == VITTE_AST_NODE_FLOAT_LITERAL || body->kind == VITTE_AST_NODE_STRING_LITERAL ||
            body->kind == VITTE_AST_NODE_CHARACTER_LITERAL || body->kind == VITTE_AST_NODE_NULL_LITERAL ||
            body->kind == VITTE_AST_NODE_UNARY_EXPR ||
            body->kind == VITTE_AST_NODE_BINARY_EXPR || body->kind == VITTE_AST_NODE_CALL_EXPR ||
            body->kind == VITTE_AST_NODE_GROUP_EXPR)) {
            vitte_ast_node_t implicit_return = *body;
            vitte_ast_node_id_t value = body->id;
            implicit_return.kind = VITTE_AST_NODE_RETURN_STMT;
            implicit_return.children = &value;
            implicit_return.child_count = 1u;
            vitte_sema_check_return_contracts(context, &implicit_return, procedure, symbol, dead, depth + 1u);
            return;
        }
    }
    if (node->kind == VITTE_AST_NODE_RETURN_STMT && node->child_count == 1u) {
        vitte_constant_t result = vitte_sema_known_value(context,
            vitte_sema_node(context, node->children[0]));
        if (result.kind == VITTE_CONSTANT_KIND_INVALID) return;
        for (index = 0u; index < procedure->child_count; ++index) {
            const vitte_ast_node_t *clause = vitte_sema_node(context, procedure->children[index]);
            vitte_sema_eval_t eval;
            vitte_constant_t value, declaration_value;
            if (clause == NULL || (clause->flags & VITTE_AST_FLAG_CONTRACT_ENSURES) == 0u) continue;
            declaration_value = vitte_sema_known_value(context, clause);
            if (declaration_value.kind == VITTE_CONSTANT_KIND_BOOL && !declaration_value.as.boolean) continue;
            memset(&eval, 0, sizeof(eval));
            eval.sema = context;
            eval.result = &result;
            value = vitte_sema_eval_node(&eval, clause);
            if (value.kind == VITTE_CONSTANT_KIND_BOOL && !value.as.boolean)
                vitte_sema_contract_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_CONTRACT_POSTCONDITION,
                    node, clause, symbol);
        }
        return;
    }
    for (index = 0u; index < node->child_count; ++index)
        vitte_sema_check_return_contracts(context, vitte_sema_node(context, node->children[index]),
            procedure, symbol, dead, depth + 1u);
}

static void
vitte_sema_check_contract_effects(vitte_sema_t *context, const vitte_ast_node_t *node,
    const vitte_ast_node_t *clause, vitte_symbol_id_t symbol, size_t depth)
{
    size_t index;
    if (node == NULL || depth >= 128u) return;
    if (node->kind == VITTE_AST_NODE_ASSIGN_EXPR || node->kind == VITTE_AST_NODE_AWAIT_EXPR ||
        node->kind == VITTE_AST_NODE_TRY_EXPR) {
        vitte_sema_contract_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_CONTRACT_EFFECT, node, clause, symbol);
        return;
    }
    for (index = 0u; index < node->child_count; ++index)
        vitte_sema_check_contract_effects(context, vitte_sema_node(context, node->children[index]),
            clause, symbol, depth + 1u);
}

static void
vitte_sema_pattern_boolean_coverage(
    vitte_sema_t *context,
    const vitte_ast_node_t *pattern,
    bool *covers_true,
    bool *covers_false,
    size_t depth)
{
    size_t index;
    if (context == NULL || pattern == NULL || depth >= 128u) return;
    if (pattern->kind == VITTE_AST_NODE_BOOL_LITERAL) {
        vitte_constant_t value = vitte_sema_known_value(context, pattern);
        if (value.kind == VITTE_CONSTANT_KIND_BOOL) {
            if (value.as.boolean) *covers_true = true;
            else *covers_false = true;
        }
        return;
    }
    if (pattern->kind == VITTE_AST_NODE_PATTERN) {
        for (index = 0u; index < pattern->child_count; ++index) {
            vitte_sema_pattern_boolean_coverage(
                context,
                vitte_sema_node(context, pattern->children[index]),
                covers_true,
                covers_false,
                depth + 1u);
        }
    }
}

static bool
vitte_sema_pattern_is_wildcard_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *pattern)
{
    const vitte_token_t *token;
    if (context == NULL || pattern == NULL) return false;
    if (pattern->kind == VITTE_AST_NODE_PATTERN &&
        (pattern->flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u &&
        pattern->child_count == 0u) return true;
    if (pattern->kind == VITTE_AST_NODE_PATTERN &&
        (pattern->flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u &&
        pattern->child_count == 1u) {
        const vitte_ast_node_t *head = vitte_sema_node(
            context, pattern->children[0]);
        if (head != NULL && head->kind == VITTE_AST_NODE_IDENTIFIER)
            return true;
    }
    /* A bare path in a pattern introduces a binding and therefore matches
     * every value of the subject type. */
    if (pattern->kind == VITTE_AST_NODE_IDENTIFIER ||
        (pattern->kind == VITTE_AST_NODE_PATH &&
         pattern->child_count <= 1u)) return true;
    token = vitte_sema_node_token(context, pattern);
    return token != NULL && token->lexeme != NULL &&
        token->length == 1u && token->lexeme[0] == '_';
}

static bool
vitte_sema_pattern_is_irrefutable_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *pattern,
    size_t depth)
{
    size_t index;
    if (context == NULL || pattern == NULL || depth >= 128u) return false;
    if (vitte_sema_pattern_is_wildcard_private(context, pattern)) return true;
    if (pattern->kind == VITTE_AST_NODE_IDENTIFIER) return true;
    if (pattern->kind == VITTE_AST_NODE_PATH) return pattern->child_count <= 1u;
    if (pattern->kind != VITTE_AST_NODE_PATTERN ||
        (pattern->flags & (VITTE_AST_FLAG_PATTERN_RANGE |
                           VITTE_AST_FLAG_PATTERN_STRUCT |
                           VITTE_AST_FLAG_PATTERN_HEAD)) != 0u) return false;
    if ((pattern->flags & VITTE_AST_FLAG_PATTERN_OR) != 0u) {
        for (index = 0u; index < pattern->child_count; ++index) {
            if (vitte_sema_pattern_is_irrefutable_private(
                    context,
                    vitte_sema_node(context, pattern->children[index]),
                    depth + 1u)) return true;
        }
        return false;
    }
    for (index = 0u; index < pattern->child_count; ++index) {
        if (!vitte_sema_pattern_is_irrefutable_private(
                context,
                vitte_sema_node(context, pattern->children[index]),
                depth + 1u)) return false;
    }
    return true;
}

static bool
vitte_sema_pattern_covers_pick_variant_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *pattern,
    vitte_scope_id_t scope_id,
    vitte_symbol_id_t pick_symbol,
    const vitte_ast_node_t *variant,
    size_t depth)
{
    const vitte_ast_node_t *head;
    vitte_sema_pick_variant_resolution_t resolution;
    size_t expected;
    size_t actual;
    size_t index;

    if (context == NULL || pattern == NULL || variant == NULL ||
        depth >= 128u) return false;
    if (vitte_sema_pattern_is_wildcard_private(context, pattern)) return true;
    if (pattern->kind == VITTE_AST_NODE_PATTERN &&
        (pattern->flags & VITTE_AST_FLAG_PATTERN_OR) != 0u) {
        for (index = 0u; index < pattern->child_count; ++index) {
            if (vitte_sema_pattern_covers_pick_variant_private(
                    context,
                    vitte_sema_node(context, pattern->children[index]),
                    scope_id, pick_symbol, variant, depth + 1u)) return true;
        }
        return false;
    }
    head = pattern;
    if (pattern->kind == VITTE_AST_NODE_PATTERN &&
        (pattern->flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u) {
        if (pattern->child_count == 0u) return false;
        head = vitte_sema_node(context, pattern->children[0]);
    }
    if (head == NULL ||
        !vitte_sema_resolve_pick_variant_path(
            context, head, scope_id, &resolution) ||
        resolution.pick_symbol_id != pick_symbol ||
        resolution.variant_declaration != variant) return false;
    expected = variant->child_count == 0u ? 0u :
        variant->child_count - 1u;
    actual = pattern == head ? 0u : pattern->child_count - 1u;
    if (expected != actual) return false;
    for (index = 0u; index < actual; ++index) {
        if (!vitte_sema_pattern_is_irrefutable_private(
                context,
                vitte_sema_node(context, pattern->children[index + 1u]),
                depth + 1u)) return false;
    }
    return true;
}

static bool vitte_sema_pattern_has_prior_error_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *pattern);

static bool
vitte_sema_match_exhaustive_for_flow_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *match_node,
    size_t last_arm_index)
{
    const vitte_sema_node_info_t *subject_info;
    const vitte_sema_type_t *subject_type;
    const vitte_symbol_t *subject_symbol;
    const vitte_ast_node_t *pick_declaration;
    bool covers_true = false;
    bool covers_false = false;
    bool wildcard = false;
    size_t arm_end;
    size_t arm_index;

    if (context == NULL || match_node == NULL ||
        match_node->kind != VITTE_AST_NODE_MATCH_STMT ||
        match_node->child_count == 0u) return false;
    subject_info = vitte_sema_get_node_info(context, match_node->children[0]);
    if (subject_info == NULL || subject_info->type_id == context->type_error)
        return false;
    subject_type = vitte_sema_type_private(context, subject_info->type_id);
    subject_symbol = subject_type != NULL &&
            subject_type->kind == VITTE_SEMA_TYPE_NAMED
        ? vitte_scope_get_symbol(&context->scopes, subject_type->symbol_id)
        : NULL;
    pick_declaration = subject_symbol != NULL &&
            subject_symbol->kind == VITTE_SYMBOL_KIND_PICK
        ? (const vitte_ast_node_t *)subject_symbol->payload : NULL;
    arm_end = match_node->child_count;
    if (arm_end > 1u && last_arm_index < arm_end - 1u)
        arm_end = last_arm_index + 1u;

    for (arm_index = 1u; arm_index < arm_end; ++arm_index) {
        const vitte_ast_node_t *arm = vitte_sema_node(
            context, match_node->children[arm_index]);
        const vitte_ast_node_t *pattern;
        bool unguarded;
        if (arm == NULL || arm->kind != VITTE_AST_NODE_MATCH_ARM ||
            arm->child_count < 2u) return false;
        unguarded = arm->child_count == 2u;
        if (!unguarded && arm->child_count == 3u) {
            const vitte_ast_node_t *guard = vitte_sema_node(
                context, arm->children[1]);
            vitte_constant_t guard_value = vitte_sema_known_value(context, guard);
            unguarded = guard_value.kind == VITTE_CONSTANT_KIND_BOOL &&
                guard_value.as.boolean;
        }
        if (!unguarded) continue;
        pattern = vitte_sema_node(context, arm->children[0]);
        if (pattern == NULL ||
            vitte_sema_pattern_has_prior_error_private(context, pattern))
            return false;
        if (vitte_sema_pattern_is_wildcard_private(context, pattern)) {
            wildcard = true;
            break;
        }
        vitte_sema_pattern_boolean_coverage(
            context, pattern, &covers_true, &covers_false, 0u);
    }
    if (wildcard || (subject_info->type_id == context->type_bool &&
        covers_true && covers_false)) return true;
    if (pick_declaration == NULL) return false;

    for (arm_index = 1u; arm_index < pick_declaration->child_count; ++arm_index) {
        const vitte_ast_node_t *variant = vitte_sema_node(
            context, pick_declaration->children[arm_index]);
        size_t candidate_index;
        bool covered = false;
        if (variant == NULL || variant->kind != VITTE_AST_NODE_VARIANT_DECL)
            continue;
        for (candidate_index = 1u;
             candidate_index < arm_end;
             ++candidate_index) {
            const vitte_ast_node_t *arm = vitte_sema_node(
                context, match_node->children[candidate_index]);
            const vitte_ast_node_t *pattern;
            bool unguarded = arm != NULL && arm->child_count == 2u;
            if (!unguarded && arm != NULL && arm->child_count == 3u) {
                const vitte_ast_node_t *guard = vitte_sema_node(
                    context, arm->children[1]);
                vitte_constant_t guard_value = vitte_sema_known_value(
                    context, guard);
                unguarded = guard_value.kind == VITTE_CONSTANT_KIND_BOOL &&
                    guard_value.as.boolean;
            }
            if (!unguarded || arm == NULL || arm->child_count < 2u) continue;
            pattern = vitte_sema_node(context, arm->children[0]);
            if (pattern != NULL &&
                vitte_sema_pattern_covers_pick_variant_private(
                    context, pattern, subject_info->scope_id,
                    subject_symbol->id, variant, 0u)) {
                covered = true;
                break;
            }
        }
        if (!covered) return false;
    }
    return true;
}

static bool
vitte_sema_constant_integer_private(
    vitte_constant_t value,
    int64_t *result)
{
    if (result == NULL) return false;
    switch (value.kind) {
        case VITTE_CONSTANT_KIND_I8: *result = value.as.i8; return true;
        case VITTE_CONSTANT_KIND_I16: *result = value.as.i16; return true;
        case VITTE_CONSTANT_KIND_I32: *result = value.as.i32; return true;
        case VITTE_CONSTANT_KIND_I64: *result = value.as.i64; return true;
        case VITTE_CONSTANT_KIND_U8: *result = value.as.u8; return true;
        case VITTE_CONSTANT_KIND_U16: *result = value.as.u16; return true;
        case VITTE_CONSTANT_KIND_U32: *result = value.as.u32; return true;
        case VITTE_CONSTANT_KIND_U64:
            if (value.as.u64 > INT64_MAX) return false;
            *result = (int64_t)value.as.u64;
            return true;
        case VITTE_CONSTANT_KIND_CHAR:
            *result = (int64_t)value.as.character;
            return true;
        default:
            return false;
    }
}

static bool
vitte_sema_pattern_integer_interval_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *pattern,
    int64_t *lower,
    int64_t *upper,
    bool *upper_inclusive)
{
    if (context == NULL || pattern == NULL || lower == NULL ||
        upper == NULL || upper_inclusive == NULL) return false;
    if (pattern->kind == VITTE_AST_NODE_PATTERN &&
        (pattern->flags & VITTE_AST_FLAG_PATTERN_RANGE) != 0u &&
        pattern->child_count == 2u) {
        const vitte_ast_node_t *lower_node = vitte_sema_node(
            context, pattern->children[0]);
        const vitte_ast_node_t *upper_node = vitte_sema_node(
            context, pattern->children[1]);
        if (!vitte_sema_constant_integer_private(
                vitte_sema_known_value(context, lower_node), lower) ||
            !vitte_sema_constant_integer_private(
                vitte_sema_known_value(context, upper_node), upper)) return false;
        *upper_inclusive =
            pattern->operator_kind == VITTE_TOKEN_DOT_DOT_EQUAL;
        return true;
    }
    if (pattern->kind == VITTE_AST_NODE_INTEGER_LITERAL) {
        int64_t value;
        if (!vitte_sema_constant_integer_private(
                vitte_sema_known_value(context, pattern), &value)) return false;
        *lower = value;
        *upper = value;
        *upper_inclusive = true;
        return true;
    }
    return false;
}

static bool
vitte_sema_integer_intervals_overlap_private(
    int64_t left_lower,
    int64_t left_upper,
    bool left_upper_inclusive,
    int64_t right_lower,
    int64_t right_upper,
    bool right_upper_inclusive)
{
    if (left_upper < right_lower || right_upper < left_lower) return false;
    if (left_upper == right_lower && !left_upper_inclusive) return false;
    if (right_upper == left_lower && !right_upper_inclusive) return false;
    return true;
}

static bool
vitte_sema_integer_interval_contains_private(
    int64_t outer_lower,
    int64_t outer_upper,
    bool outer_upper_inclusive,
    int64_t inner_lower,
    int64_t inner_upper,
    bool inner_upper_inclusive)
{
    if (outer_lower > inner_lower || outer_upper < inner_upper) return false;
    return outer_upper != inner_upper || outer_upper_inclusive ||
        !inner_upper_inclusive;
}

static bool
vitte_sema_pattern_has_prior_error_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *pattern)
{
    size_t index;
    if (context == NULL || pattern == NULL) return false;
    for (index = 0u; index < context->diagnostic_count; ++index) {
        const vitte_sema_diagnostic_t *diagnostic =
            &context->diagnostics[index];
        if (diagnostic->node_id == pattern->id &&
            (diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INVALID_PATTERN_BINDINGS ||
             diagnostic->code == VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE ||
             diagnostic->code == VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER ||
             diagnostic->code == VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME)) {
            return true;
        }
    }
    return false;
}

static bool
vitte_sema_match_has_invalid_pattern_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *match_node)
{
    size_t arm_index;
    if (context == NULL || match_node == NULL) return false;
    for (arm_index = 1u; arm_index < match_node->child_count; ++arm_index) {
        const vitte_ast_node_t *arm = vitte_sema_node(
            context, match_node->children[arm_index]);
        const vitte_ast_node_t *pattern = arm != NULL && arm->child_count >= 2u
            ? vitte_sema_node(context, arm->children[0]) : NULL;
        size_t diagnostic_index;
        if (pattern == NULL) continue;
        for (diagnostic_index = 0u;
             diagnostic_index < context->diagnostic_count;
             ++diagnostic_index) {
            const vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[diagnostic_index];
            bool pattern_error =
                diagnostic->code == VITTE_SEMA_DIAGNOSTIC_INVALID_PATTERN_BINDINGS ||
                diagnostic->code == VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE ||
                diagnostic->code == VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER ||
                diagnostic->code == VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME ||
                diagnostic->code == VITTE_SEMA_DIAGNOSTIC_DUPLICATE_BINDING;
            if (pattern_error &&
                diagnostic->span.file_id == pattern->span.file_id &&
                diagnostic->span.begin >= pattern->span.begin &&
                diagnostic->span.end <= pattern->span.end) {
                return true;
            }
        }
        if (arm != NULL && arm->child_count == 3u) {
            const vitte_ast_node_t *guard = vitte_sema_node(
                context, arm->children[1]);
            if (guard != NULL) {
                for (diagnostic_index = 0u;
                     diagnostic_index < context->diagnostic_count;
                     ++diagnostic_index) {
                    const vitte_sema_diagnostic_t *diagnostic =
                        &context->diagnostics[diagnostic_index];
                    if (diagnostic->kind == VITTE_SEMA_DIAGNOSTIC_ERROR &&
                        diagnostic->span.file_id == guard->span.file_id &&
                        diagnostic->span.begin >= guard->span.begin &&
                        diagnostic->span.end <= guard->span.end) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

static void
vitte_sema_check_match(vitte_sema_t *context, const vitte_ast_node_t *node)
{
    const vitte_sema_node_info_t *subject;
    const vitte_sema_type_t *subject_type;
    const vitte_symbol_t *subject_symbol;
    const vitte_ast_node_t *pick_declaration;
    const vitte_ast_node_t *true_cover = NULL;
    const vitte_ast_node_t *false_cover = NULL;
    const vitte_ast_node_t *wildcard_cover = NULL;
    vitte_scope_id_t match_scope;
    bool has_true = false, has_false = false, has_wildcard = false;
    bool exhaustive = false;
    size_t index;
    if (node->child_count == 0u) return;
    subject = vitte_sema_get_node_info(context, node->children[0]);
    if (subject == NULL || subject->type_id == context->type_error) return;
    if (vitte_sema_match_has_invalid_pattern_private(context, node)) return;
    /* An invalid pattern can look exhaustive to the structural coverage
     * pass. Preserve its primary diagnostic and avoid a derived unreachable
     * or non-exhaustive report for another arm in the same match. */
    for (index = 1u; index < node->child_count; ++index) {
        const vitte_ast_node_t *arm = vitte_sema_node(
            context, node->children[index]);
        const vitte_ast_node_t *pattern = arm != NULL && arm->child_count >= 2u
            ? vitte_sema_node(context, arm->children[0]) : NULL;
        if (vitte_sema_pattern_has_prior_error_private(context, pattern))
            return;
        if (pattern != NULL) {
            const vitte_sema_node_info_t *pattern_info =
                vitte_sema_get_node_info(context, pattern->id);
            if (pattern_info != NULL &&
                pattern_info->type_id == context->type_error) return;
        }
    }
    subject_type = vitte_sema_type_private(context, subject->type_id);
    subject_symbol = subject_type != NULL &&
            subject_type->kind == VITTE_SEMA_TYPE_NAMED
        ? vitte_scope_get_symbol(&context->scopes, subject_type->symbol_id)
        : NULL;
    pick_declaration = subject_symbol != NULL &&
            subject_symbol->kind == VITTE_SYMBOL_KIND_PICK
        ? (const vitte_ast_node_t *)subject_symbol->payload : NULL;
    match_scope = subject->scope_id;
    for (index = 1u; index < node->child_count; ++index) {
        const vitte_ast_node_t *arm = vitte_sema_node(context, node->children[index]);
        const vitte_ast_node_t *pattern;
        const vitte_ast_node_t *covering_pattern = NULL;
        const vitte_ast_node_t *guard_node = NULL;
        const vitte_token_t *token;
        bool duplicate = has_wildcard;
        bool false_guard = false;
        size_t covering_line = 0u;
        bool arm_true = false;
        bool arm_false = false;
        bool arm_wildcard;
        bool unguarded;
        bool overlapping_range = false;
        const vitte_ast_node_t *overlap_cause = NULL;
        if (arm == NULL || arm->child_count < 2u) continue;
        pattern = vitte_sema_node(context, arm->children[0]);
        token = vitte_sema_node_token(context, pattern);
        if (vitte_sema_pattern_has_prior_error_private(context, pattern))
            return;
        if (pattern != NULL) {
            const vitte_sema_node_info_t *pattern_info = vitte_sema_get_node_info(context, pattern->id);
            if (pattern_info != NULL && pattern_info->type_id == context->type_error) return;
        }
        unguarded = arm->child_count == 2u;
        if (!unguarded && arm->child_count == 3u) {
            guard_node = vitte_sema_node(context, arm->children[1]);
            {
                vitte_constant_t guard = vitte_sema_known_value(
                    context, guard_node);
                unguarded = guard.kind == VITTE_CONSTANT_KIND_BOOL &&
                    guard.as.boolean;
            }
        }
        arm_wildcard = vitte_sema_pattern_is_wildcard_private(
            context, pattern);
        if (arm_wildcard) {
            if (has_wildcard) covering_pattern = wildcard_cover;
            if (unguarded) {
                has_wildcard = true;
                if (wildcard_cover == NULL) wildcard_cover = pattern;
            }
        }
        vitte_sema_pattern_boolean_coverage(
            context, pattern, &arm_true, &arm_false, 0u);
        if (arm_true) {
            duplicate |= has_true;
            if (has_true && covering_pattern == NULL)
                covering_pattern = true_cover;
            if (unguarded) {
                has_true = true;
                if (true_cover == NULL) true_cover = pattern;
            }
        }
        if (arm_false) {
            duplicate |= has_false;
            if (has_false && covering_pattern == NULL)
                covering_pattern = false_cover;
            if (unguarded) {
                has_false = true;
                if (false_cover == NULL) false_cover = pattern;
            }
        }
        if (duplicate && covering_pattern == NULL && wildcard_cover != NULL) {
            covering_pattern = wildcard_cover;
        }
        /* Only an earlier unguarded arm proves that a value is covered.
         * Compare evaluated integers, so alternate radix spellings agree. */
        if (!duplicate && pattern != NULL) {
            size_t previous;
            vitte_constant_t value = vitte_sema_known_value(context, pattern);
            for (previous = 1u; previous < index; ++previous) {
                const vitte_ast_node_t *earlier = vitte_sema_node(context, node->children[previous]);
                const vitte_ast_node_t *earlier_pattern;
                const vitte_token_t *earlier_token;
                vitte_constant_t earlier_value;
                vitte_constant_result_t equal;
                if (earlier == NULL || earlier->child_count != 2u) continue;
                earlier_pattern = vitte_sema_node(context, earlier->children[0]);
                if (earlier_pattern == NULL) continue;
                earlier_value = vitte_sema_known_value(context, earlier_pattern);
                if (value.kind != VITTE_CONSTANT_KIND_INVALID &&
                    earlier_value.kind == value.kind) {
                    equal = vitte_constant_fold_binary(VITTE_CONSTANT_BINARY_EQUAL, &value, &earlier_value);
                    if (equal.status == VITTE_CONSTANT_FOLD_SUCCESS &&
                        equal.value.kind == VITTE_CONSTANT_KIND_BOOL && equal.value.as.boolean)
                        duplicate = true;
                }
                /* Exact text equality is sufficient for string and character
                 * literals; distinct escape spellings remain conservative. */
                earlier_token = vitte_sema_node_token(context, earlier_pattern);
                if (token != NULL && earlier_token != NULL &&
                    (pattern->kind == VITTE_AST_NODE_STRING_LITERAL ||
                     pattern->kind == VITTE_AST_NODE_CHARACTER_LITERAL) &&
                    earlier_pattern->kind == pattern->kind && token->length == earlier_token->length &&
                    memcmp(token->lexeme, earlier_token->lexeme, token->length) == 0)
                    duplicate = true;
                if (duplicate) {
                    covering_line = earlier_pattern->span.line;
                    covering_pattern = earlier_pattern;
                    break;
                }
            }
        }
        if (!duplicate && pattern != NULL) {
            int64_t lower, upper;
            bool upper_inclusive;
            if (vitte_sema_pattern_integer_interval_private(
                    context, pattern, &lower, &upper, &upper_inclusive)) {
                size_t previous;
                for (previous = 1u; previous < index; ++previous) {
                    const vitte_ast_node_t *earlier = vitte_sema_node(
                        context, node->children[previous]);
                    const vitte_ast_node_t *earlier_pattern;
                    int64_t earlier_lower, earlier_upper;
                    bool earlier_upper_inclusive;
                    bool earlier_unguarded;
                    if (earlier == NULL || earlier->child_count < 2u) continue;
                    earlier_unguarded = earlier->child_count == 2u;
                    if (!earlier_unguarded && earlier->child_count == 3u) {
                        const vitte_ast_node_t *earlier_guard =
                            vitte_sema_node(context, earlier->children[1]);
                        vitte_constant_t value = vitte_sema_known_value(
                            context, earlier_guard);
                        earlier_unguarded = value.kind ==
                                VITTE_CONSTANT_KIND_BOOL && value.as.boolean;
                    }
                    if (!earlier_unguarded) continue;
                    earlier_pattern = vitte_sema_node(
                        context, earlier->children[0]);
                    if (!vitte_sema_pattern_integer_interval_private(
                            context, earlier_pattern,
                            &earlier_lower, &earlier_upper,
                            &earlier_upper_inclusive) ||
                        !vitte_sema_integer_intervals_overlap_private(
                            earlier_lower, earlier_upper,
                            earlier_upper_inclusive,
                            lower, upper, upper_inclusive)) continue;
                    if (vitte_sema_integer_interval_contains_private(
                            earlier_lower, earlier_upper,
                            earlier_upper_inclusive,
                            lower, upper, upper_inclusive)) {
                        duplicate = true;
                        covering_pattern = earlier_pattern;
                        covering_line = earlier_pattern->span.line;
                        break;
                    }
                    overlapping_range = true;
                    overlap_cause = earlier_pattern;
                    break;
                }
            }
        }
        if (arm->child_count == 3u) {
            guard_node = vitte_sema_node(context, arm->children[1]);
            vitte_constant_t guard = vitte_sema_known_value(
                context, guard_node);
            if (guard.kind == VITTE_CONSTANT_KIND_BOOL && !guard.as.boolean) {
                duplicate = true;
                false_guard = true;
            }
        }
        if (!duplicate && pick_declaration != NULL && pattern != NULL) {
            size_t variant_index;
            size_t current_variant_count = 0u;
            bool every_current_variant_covered = true;
            /* An overlap is not enough to make an arm unreachable. Require
             * the earlier arms' union to cover every variant this pattern
             * can match. */
            for (variant_index = 1u;
                 variant_index < pick_declaration->child_count;
                 ++variant_index) {
                const vitte_ast_node_t *variant = vitte_sema_node(
                    context,
                    pick_declaration->children[variant_index]);
                bool covered = false;
                size_t previous;
                if (variant == NULL ||
                    variant->kind != VITTE_AST_NODE_VARIANT_DECL ||
                    !vitte_sema_pattern_covers_pick_variant_private(
                        context, pattern, match_scope,
                        subject_symbol->id, variant, 0u)) continue;
                ++current_variant_count;
                for (previous = 1u; previous < index; ++previous) {
                    const vitte_ast_node_t *earlier = vitte_sema_node(
                        context, node->children[previous]);
                    const vitte_ast_node_t *earlier_pattern = earlier != NULL &&
                            earlier->child_count >= 2u
                        ? vitte_sema_node(context, earlier->children[0]) : NULL;
                    bool earlier_unguarded = earlier != NULL &&
                        earlier->child_count == 2u;
                    if (!earlier_unguarded && earlier != NULL &&
                        earlier->child_count == 3u) {
                        const vitte_ast_node_t *earlier_guard =
                            vitte_sema_node(context, earlier->children[1]);
                        vitte_constant_t value = vitte_sema_known_value(
                            context, earlier_guard);
                        earlier_unguarded = value.kind ==
                                VITTE_CONSTANT_KIND_BOOL && value.as.boolean;
                    }
                    if (earlier_unguarded && earlier_pattern != NULL &&
                        vitte_sema_pattern_covers_pick_variant_private(
                            context, earlier_pattern, match_scope,
                            subject_symbol->id, variant, 0u)) {
                        covered = true;
                        if (covering_pattern == NULL)
                            covering_pattern = earlier_pattern;
                        covering_line = earlier_pattern->span.line;
                        break;
                    }
                }
                if (!covered) every_current_variant_covered = false;
            }
            duplicate = current_variant_count != 0u &&
                every_current_variant_covered;
        }
        if (duplicate && pattern != NULL &&
            vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_MATCH_UNREACHABLE,
                false_guard && guard_node != NULL
                    ? guard_node->span : pattern->span,
                false_guard && guard_node != NULL
                    ? guard_node->id : pattern->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID, VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
            char *help = context->diagnostics[context->diagnostic_count - 1u].help;
            if (false_guard)
                (void)snprintf(help, 512u,
                    "This arm's guard is always false. Correct the guard or remove the arm.");
            else if (covering_line != 0u)
                (void)snprintf(help, 512u,
                    "An unguarded arm on line %zu already covers this value. Remove the duplicate or use a distinct pattern.",
                    covering_line);
            if (false_guard && guard_node != NULL) {
                vitte_sema_set_last_related_location(
                    context, pattern,
                    "this pattern is disabled by the guard");
            } else if (covering_pattern != NULL) {
                vitte_sema_set_last_related_location(
                    context, covering_pattern,
                    "earlier arm already covers this pattern");
            }
        }
        if (!duplicate && overlapping_range && pattern != NULL &&
            vitte_sema_add_diagnostic(context,
                VITTE_SEMA_DIAGNOSTIC_WARNING,
                VITTE_SEMA_DIAGNOSTIC_OVERLAPPING_PATTERN,
                pattern->span, pattern->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[context->diagnostic_count - 1u];
            (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                "This range overlaps an earlier match pattern, so values in the intersection are handled by the earlier arm first.");
            if (overlap_cause != NULL) {
                vitte_sema_set_last_related_location(
                    context, overlap_cause,
                    "earlier pattern overlaps this range");
            }
        }
        if (subject->type_id == context->type_bool && has_true && has_false)
            exhaustive = true;
        if (has_wildcard) exhaustive = true;
    }

    if (pick_declaration != NULL && !exhaustive) {
        char missing[384];
        size_t offset = 0u;
        size_t missing_count = 0u;
        size_t variant_index;
        missing[0] = '\0';
        for (variant_index = 1u;
             variant_index < pick_declaration->child_count;
             ++variant_index) {
            const vitte_ast_node_t *variant = vitte_sema_node(
                context, pick_declaration->children[variant_index]);
            size_t arm_index;
            bool covered_variant = false;
            const vitte_ast_node_t *variant_name_node;
            const vitte_token_t *variant_name;
            if (variant == NULL ||
                variant->kind != VITTE_AST_NODE_VARIANT_DECL) continue;
            for (arm_index = 1u; arm_index < node->child_count; ++arm_index) {
                const vitte_ast_node_t *arm = vitte_sema_node(
                    context, node->children[arm_index]);
                const vitte_ast_node_t *pattern = arm != NULL &&
                        arm->child_count >= 2u
                    ? vitte_sema_node(context, arm->children[0]) : NULL;
                bool unguarded = arm != NULL && arm->child_count == 2u;
                if (!unguarded && arm != NULL && arm->child_count == 3u) {
                    const vitte_ast_node_t *guard = vitte_sema_node(
                        context, arm->children[1]);
                    vitte_constant_t value = vitte_sema_known_value(
                        context, guard);
                    unguarded = value.kind == VITTE_CONSTANT_KIND_BOOL &&
                        value.as.boolean;
                }
                if (unguarded && pattern != NULL &&
                    vitte_sema_pattern_covers_pick_variant_private(
                        context, pattern, match_scope, subject_symbol->id,
                        variant, 0u)) {
                    covered_variant = true;
                    break;
                }
            }
            if (covered_variant) continue;
            variant_name_node = variant->child_count != 0u
                ? vitte_sema_node(context, variant->children[0]) : NULL;
            variant_name = vitte_sema_node_token(context, variant_name_node);
            if (variant_name == NULL || variant_name->lexeme == NULL) continue;
            if (missing_count < 5u && offset < sizeof(missing)) {
                int written = snprintf(missing + offset,
                    sizeof(missing) - offset,
                    "%s%.*s::%.*s%s",
                    missing_count == 0u ? "" : ", ",
                    (int)(subject_symbol->name_length > 64u ? 64u :
                          subject_symbol->name_length), subject_symbol->name,
                    (int)(variant_name->length > 64u ? 64u :
                          variant_name->length), variant_name->lexeme,
                    variant->child_count > 1u ? "(_, ...)" : "");
                if (written > 0 && (size_t)written < sizeof(missing) - offset)
                    offset += (size_t)written;
            }
            ++missing_count;
        }
        if (missing_count == 0u) exhaustive = true;
        else if (vitte_sema_add_diagnostic(context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_MATCH_NON_EXHAUSTIVE,
                node->span, node->id, subject_symbol->id,
                VITTE_SEMA_INVALID_TYPE_ID, subject->type_id)) {
            vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[context->diagnostic_count - 1u];
            (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                "Add an unguarded arm for each missing variant: %s%s.",
                missing, missing_count > 5u ? ", ..." : "");
            vitte_sema_set_last_related_location(
                context, pick_declaration,
                "all variants are declared here");
        }
    } else if (!exhaustive && subject->type_id == context->type_bool) {
        if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_MATCH_NON_EXHAUSTIVE, node->span, node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID, VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
            (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help, 512u,
                "Add an unguarded arm for the missing pattern `%s`, or add `_` to cover all remaining boolean values.",
                has_true ? "false" : "true");
        }
    } else if (!exhaustive) {
        if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_MATCH_NON_EXHAUSTIVE, node->span, node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID, VITTE_SEMA_INVALID_TYPE_ID,
                subject->type_id)) {
            (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                512u,
                "This match still allows values outside its listed patterns. Add an unguarded `_` arm for the remaining values.");
        }
    }
}

static void
vitte_sema_mark_dead(vitte_sema_t *context, const vitte_ast_node_t *node,
                     bool inherited, bool *dead, size_t depth)
{
    size_t index;
    bool terminated = false;
    vitte_constant_t condition = vitte_constant_make_invalid();
    if (node == NULL || depth >= 128u || node->id == VITTE_AST_INVALID_ID ||
        node->id > context->parser->node_count) return;
    dead[node->id] = inherited;
    if (node->child_count != 0u &&
        (node->kind == VITTE_AST_NODE_BINARY_EXPR || node->kind == VITTE_AST_NODE_IF_STMT ||
         node->kind == VITTE_AST_NODE_IF_EXPR || node->kind == VITTE_AST_NODE_WHILE_STMT))
        condition = vitte_sema_known_value(context, vitte_sema_node(context, node->children[0]));
    for (index = 0u; index < node->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(context, node->children[index]);
        bool child_dead = inherited || terminated;
        if (index != 0u && condition.kind == VITTE_CONSTANT_KIND_BOOL) {
            if (node->kind == VITTE_AST_NODE_BINARY_EXPR) {
                vitte_constant_binary_operator_t op = vitte_sema_eval_binary_operator(node->operator_kind);
                child_dead |= (op == VITTE_CONSTANT_BINARY_LOGICAL_AND && !condition.as.boolean) ||
                              (op == VITTE_CONSTANT_BINARY_LOGICAL_OR && condition.as.boolean);
            } else if (node->kind == VITTE_AST_NODE_IF_STMT || node->kind == VITTE_AST_NODE_IF_EXPR) {
                child_dead |= index == 1u ? !condition.as.boolean : condition.as.boolean;
            } else if (node->kind == VITTE_AST_NODE_WHILE_STMT) child_dead |= !condition.as.boolean;
        }
        vitte_sema_mark_dead(context, child, child_dead, dead, depth + 1u);
        if (node->kind == VITTE_AST_NODE_BLOCK &&
            vitte_sema_exit_flow(context, child, 0u) == VITTE_SEMA_EXIT_TERMINATED) terminated = true;
    }
}

static void
vitte_sema_check_static_program(vitte_sema_t *context)
{
    size_t index;
    bool *dead = calloc(context->parser->node_count + 1u, sizeof(*dead));
    bool *minimum = calloc(context->parser->node_count + 1u, sizeof(*minimum));
    if (dead == NULL || minimum == NULL) {
        free(dead);
        free(minimum);
        (void)vitte_sema_fail(context, VITTE_SEMA_ERROR_OUT_OF_MEMORY);
        return;
    }
    /* Unvisited nodes beyond the traversal budget remain excluded. */
    for (index = 0u; index <= context->parser->node_count; ++index) dead[index] = true;
    vitte_sema_mark_dead(context, vitte_sema_node(context, vitte_parser_root(context->parser)), false, dead, 0u);
    for (index = 0u; index < context->parser->node_count; ++index) {
        const vitte_ast_node_t *node = &context->parser->nodes[index];
        size_t depth = 0u;
        uint64_t magnitude;
        if (node->kind != VITTE_AST_NODE_UNARY_EXPR || node->operator_kind != VITTE_TOKEN_MINUS ||
            node->child_count != 1u) continue;
        node = vitte_sema_node(context, node->children[0]);
        while (node != NULL && node->kind == VITTE_AST_NODE_GROUP_EXPR && node->child_count == 1u && depth++ < 128u)
            node = vitte_sema_node(context, node->children[0]);
        if (node != NULL && node->id <= context->parser->node_count && vitte_sema_parse_unsigned_integer_literal(context, node, &magnitude) &&
            magnitude == (uint64_t)INT64_MAX + UINT64_C(1)) minimum[node->id] = true;
    }
    for (index = 0u; index < context->parser->node_count; ++index) {
        const vitte_ast_node_t *node = &context->parser->nodes[index];
        const vitte_sema_node_info_t *info = vitte_sema_get_node_info(context, node->id);
        if (context->diagnostic_count >= context->max_diagnostics) break;
        if (node->id == VITTE_AST_INVALID_ID || node->id > context->parser->node_count || dead[node->id]) continue;
        if (node->kind == VITTE_AST_NODE_INTEGER_LITERAL && info != NULL &&
            info->type_id == context->type_i64 && !minimum[node->id]) {
            uint64_t magnitude;
            if (vitte_sema_parse_unsigned_integer_literal(context, node, &magnitude) && magnitude > (uint64_t)INT64_MAX) {
                if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_CONSTANT_OVERFLOW, node->span, node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID, VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
                    (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help, 512u,
                        "This integer literal does not fit in i64. Use a representable value or an explicit u64 context.");
                }
            }
        } else if (node->kind == VITTE_AST_NODE_BINARY_EXPR || node->kind == VITTE_AST_NODE_UNARY_EXPR) {
            vitte_sema_eval_t eval;
            memset(&eval, 0, sizeof(eval));
            eval.sema = context;
            (void)vitte_sema_eval_node(&eval, node);
            if (eval.failure_node == node->id) {
                vitte_sema_diagnostic_code_t code =
                    eval.error == VITTE_CONSTANT_ERROR_DIVISION_BY_ZERO
                        ? VITTE_SEMA_DIAGNOSTIC_CONSTANT_DIVIDE_BY_ZERO
                        : eval.error == VITTE_CONSTANT_ERROR_REMAINDER_BY_ZERO
                            ? VITTE_SEMA_DIAGNOSTIC_CONSTANT_REMAINDER_BY_ZERO
                        : eval.error == VITTE_CONSTANT_ERROR_INVALID_SHIFT
                            ? VITTE_SEMA_DIAGNOSTIC_INVALID_BINARY_OPERANDS
                            : VITTE_SEMA_DIAGNOSTIC_CONSTANT_OVERFLOW;
                if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                        code, node->span, node->id, VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
                    char type_label[128];
                    vitte_sema_format_type_label_private(context,
                        info != NULL ? info->type_id : VITTE_SEMA_INVALID_TYPE_ID,
                        type_label, sizeof(type_label), 0u);
                    if (info != NULL && info->type_id == context->type_i64)
                        (void)snprintf(type_label, sizeof(type_label), "i64");
                    else if (info != NULL && info->type_id == context->type_u64)
                        (void)snprintf(type_label, sizeof(type_label), "u64");
                    else if (info != NULL && info->type_id == context->type_u8)
                        (void)snprintf(type_label, sizeof(type_label), "u8");
                    if (eval.error == VITTE_CONSTANT_ERROR_INVALID_SHIFT) {
                        (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                            sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                            "This shift expression uses an invalid count for `%s`. Use a count from 0 through 63 for a 64-bit integer.",
                            type_label);
                    } else if (eval.error == VITTE_CONSTANT_ERROR_DIVISION_BY_ZERO ||
                               eval.error == VITTE_CONSTANT_ERROR_REMAINDER_BY_ZERO) {
                        const char *operation = node->operator_kind == VITTE_TOKEN_PERCENT
                            ? "remainder" : "division";
                        (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                            sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                            "The divisor of this compile-time %s evaluates to zero in the underlined expression. Change the divisor or guard the operation.",
                            operation);
                    } else {
                        const char *operation = node->kind == VITTE_AST_NODE_UNARY_EXPR
                            ? "unary arithmetic" : vitte_token_kind_name(node->operator_kind);
                        (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                            sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                            "This compile-time `%s` operation overflows type `%s` in the underlined expression. Reduce the operands or use a wider representable type.",
                            operation, type_label);
                    }
                }
            }
        } else if (node->kind == VITTE_AST_NODE_INDEX_EXPR && node->child_count == 2u &&
                   info != NULL && info->type_id != context->type_error) {
            const vitte_sema_node_info_t *base = vitte_sema_get_node_info(context, node->children[0]);
            const vitte_sema_type_t *type = base != NULL ? vitte_sema_type_private(context, base->type_id) : NULL;
            vitte_constant_t subscript = vitte_sema_known_value(context,
                vitte_sema_node(context, node->children[1]));
            bool invalid = subscript.kind == VITTE_CONSTANT_KIND_I64 && subscript.as.i64 < 0;
            uint64_t offset = subscript.kind == VITTE_CONSTANT_KIND_U64
                ? subscript.as.u64 : subscript.kind == VITTE_CONSTANT_KIND_I64 && subscript.as.i64 >= 0
                    ? (uint64_t)subscript.as.i64 : 0u;
            bool known = subscript.kind == VITTE_CONSTANT_KIND_I64 || subscript.kind == VITTE_CONSTANT_KIND_U64;
            /* Zero also represents an unresolved generic length in the
             * current type model; do not invent a bound for that case. */
            if (type != NULL && type->kind == VITTE_SEMA_TYPE_ARRAY && type->array_length != 0u && known &&
                (invalid || offset >= (uint64_t)type->array_length)) {
                if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INDEX_OUT_OF_BOUNDS, node->span, node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID, VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
                    (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help, 512u,
                        "This array has %zu element(s). Use a non-negative index smaller than %zu.",
                        type->array_length, type->array_length);
                }
            }
        } else if (node->kind == VITTE_AST_NODE_CONST_DECL ||
                   node->kind == VITTE_AST_NODE_STATIC_DECL) {
            vitte_sema_eval_t eval;
            const vitte_ast_node_t *initializer = vitte_sema_initializer(context, node);
            const vitte_ast_node_t *unsupported;
            const vitte_ast_node_t *dependency_node;
            const vitte_symbol_t *dependency = NULL;
            vitte_constant_t value;
            memset(&eval, 0, sizeof(eval));
            eval.sema = context;
            eval.dependencies_only = true;
            value = vitte_sema_eval_node(&eval, initializer);
            if (eval.cycle_node != VITTE_AST_INVALID_ID) {
                const vitte_ast_node_t *cycle = vitte_sema_node(context, eval.cycle_node);
                if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_CONSTANT_CYCLE,
                        cycle != NULL ? cycle->span : node->span,
                        cycle != NULL ? cycle->id : node->id,
                        info != NULL ? info->symbol_id : VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_diagnostic_t *diagnostic =
                        &context->diagnostics[context->diagnostic_count - 1u];
                    diagnostic->related_span = node->span;
                    (void)snprintf(diagnostic->related_label,
                        sizeof(diagnostic->related_label),
                        "constant declaration participates in this cycle");
                }
            } else if (value.kind == VITTE_CONSTANT_KIND_INVALID &&
                       eval.error == VITTE_CONSTANT_ERROR_NONE &&
                       (dependency_node = vitte_sema_nonconstant_dependency_private(
                           context, initializer, &dependency, 0u)) != NULL) {
                const vitte_ast_node_t *declaration = dependency->payload != NULL
                    ? (const vitte_ast_node_t *)dependency->payload : NULL;
                const vitte_sema_node_info_t *dependency_info =
                    vitte_sema_get_node_info(context, dependency_node->id);
                if ((dependency_info == NULL || dependency_info->type_id != context->type_error) &&
                    vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_CONSTANT_NOT_CONSTANT,
                        dependency_node->span, dependency_node->id,
                        info != NULL ? info->symbol_id : VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_diagnostic_t *diagnostic =
                        &context->diagnostics[context->diagnostic_count - 1u];
                    const char *kind = dependency->kind == VITTE_SYMBOL_KIND_STATIC
                        ? "static value" : dependency->kind == VITTE_SYMBOL_KIND_PARAMETER
                            ? "parameter" : "mutable value";
                    (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                        "This initializer reads a %s `%.*s`, whose value is available only at runtime. Use a compile-time constant here or move the initialization to runtime code.",
                        kind, (int)dependency->name_length, dependency->name);
                    if (declaration != NULL) {
                        diagnostic->related_span = declaration->span;
                        (void)snprintf(diagnostic->related_label,
                            sizeof(diagnostic->related_label),
                            "runtime value is declared here");
                    }
                }
            } else if (value.kind == VITTE_CONSTANT_KIND_INVALID &&
                     eval.error == VITTE_CONSTANT_ERROR_NONE &&
                     (unsupported = vitte_sema_unsupported_constant_operation_private(
                         context, initializer, 0u)) != NULL) {
                const vitte_sema_node_info_t *unsupported_info =
                    vitte_sema_get_node_info(context, unsupported->id);
                if (unsupported_info == NULL || unsupported_info->type_id != context->type_error) {
                    if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_CONSTANT_NOT_CONSTANT,
                            unsupported->span, unsupported->id,
                            info != NULL ? info->symbol_id : VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
                        const char *operation = unsupported->kind == VITTE_AST_NODE_CALL_EXPR
                            ? "procedure call" : unsupported->kind == VITTE_AST_NODE_ASSIGN_EXPR
                                ? "assignment" : unsupported->kind == VITTE_AST_NODE_AWAIT_EXPR
                                    ? "await" : "try propagation";
                        (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                            sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                            "This %s requires runtime execution and is not valid in a const or static initializer. Replace it with a compile-time expression or initialize the value at runtime.",
                            operation);
                    }
                }
            }
        } else if (node->kind == VITTE_AST_NODE_MATCH_STMT) {
            vitte_sema_check_match(context, node);
        } else if (node->kind == VITTE_AST_NODE_CALL_EXPR) {
            vitte_sema_check_call_contracts(context, node);
        } else if (node->kind == VITTE_AST_NODE_PROC_DECL || node->kind == VITTE_AST_NODE_EXTERN_PROC_DECL) {
            size_t child_index;
            vitte_symbol_id_t symbol = info != NULL ? info->symbol_id : VITTE_SCOPE_INVALID_SYMBOL_ID;
            vitte_sema_check_references_proc(context, node, dead);
            for (child_index = 0u; child_index < node->child_count; ++child_index) {
                const vitte_ast_node_t *child = vitte_sema_node(context, node->children[child_index]);
                vitte_constant_t value;
                if (child == NULL) continue;
                if ((child->flags & (VITTE_AST_FLAG_CONTRACT_REQUIRES | VITTE_AST_FLAG_CONTRACT_ENSURES)) != 0u) {
                    vitte_sema_check_contract_effects(context, child, child, symbol, 0u);
                    value = vitte_sema_known_value(context, child);
                    if (value.kind == VITTE_CONSTANT_KIND_BOOL && !value.as.boolean)
                        vitte_sema_contract_diagnostic(context,
                            (child->flags & VITTE_AST_FLAG_CONTRACT_ENSURES) != 0u
                                ? VITTE_SEMA_DIAGNOSTIC_CONTRACT_POSTCONDITION
                                : VITTE_SEMA_DIAGNOSTIC_CONTRACT_CONTRADICTION,
                            child, child, symbol);
                } else if (child->kind == VITTE_AST_NODE_BLOCK) {
                    vitte_sema_check_return_contracts(context, child, node, symbol, dead, 0u);
                }
            }
        }
    }
    free(dead);
    free(minimum);
}
