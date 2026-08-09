#include "type.h"

#include <string.h>

static void vitte_type_registry_set_error(
    vitte_type_registry_t *registry,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (registry != NULL) {
        vitte_error_set_details(&registry->last_error, status, code, message, details);
    }
}

void vitte_type_init_invalid(vitte_type_t *type) {
    if (type == NULL) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = VITTE_TYPE_KIND_INVALID;
    type->builtin_kind = VITTE_BUILTIN_TYPE_ERROR;
    type->error = true;
}

void vitte_type_init_proc(
    vitte_type_t *type,
    const char *name,
    const vitte_type_t *return_type,
    const vitte_type_t *const *parameter_types,
    size_t arity,
    bool variadic
) {
    size_t index;

    if (type == NULL) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = VITTE_TYPE_KIND_PROC;
    type->name = name != NULL ? name : "<proc>";
    type->builtin_kind = VITTE_BUILTIN_TYPE_ERROR;
    type->return_type = return_type;
    type->arity = arity;
    type->variadic = variadic;
    type->valid = true;
    for (index = 0u; index < arity && index < VITTE_TYPE_MAX_PROC_PARAMETERS; index++) {
        type->parameter_types[index] = parameter_types != NULL ? parameter_types[index] : NULL;
    }
}

static void vitte_type_init_builtin(
    vitte_type_t *type,
    const vitte_builtin_type_t *builtin
) {
    if (type == NULL || builtin == NULL) {
        return;
    }
    memset(type, 0, sizeof(*type));
    type->kind = VITTE_TYPE_KIND_BUILTIN;
    type->name = builtin->name;
    type->builtin_kind = builtin->kind;
    type->valid = true;
    type->error = builtin->error;
}

vitte_status_t vitte_type_registry_init(vitte_type_registry_t *registry) {
    size_t index;

    if (registry == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(registry, 0, sizeof(*registry));
    vitte_error_init(&registry->last_error);
    vitte_builtin_registry_init(&registry->builtins);
    if (vitte_builtin_registry_validate(&registry->builtins) != VITTE_STATUS_OK) {
        vitte_error_copy(&registry->last_error, vitte_builtin_registry_last_error(&registry->builtins));
        return VITTE_STATUS_ERROR_INTERNAL;
    }

    for (index = 0u; index < VITTE_BUILTIN_TYPE_COUNT; index++) {
        const vitte_builtin_type_t *builtin = vitte_builtin_type_by_kind(&registry->builtins, (vitte_builtin_type_kind_t)index);
        if (builtin == NULL) {
            vitte_error_copy(&registry->last_error, vitte_builtin_registry_last_error(&registry->builtins));
            return VITTE_STATUS_ERROR_INTERNAL;
        }
        vitte_type_init_builtin(&registry->builtin_types[index], builtin);
    }

    registry->initialized = true;
    return VITTE_STATUS_OK;
}

void vitte_type_registry_destroy(vitte_type_registry_t *registry) {
    if (registry == NULL) {
        return;
    }
    memset(registry, 0, sizeof(*registry));
}

bool vitte_type_registry_is_initialized(const vitte_type_registry_t *registry) {
    return registry != NULL && registry->initialized;
}

const vitte_error_t *vitte_type_registry_last_error(const vitte_type_registry_t *registry) {
    return registry != NULL ? &registry->last_error : vitte_error_last();
}

const vitte_type_t *vitte_type_builtin(
    vitte_type_registry_t *registry,
    vitte_builtin_type_kind_t kind
) {
    if (!vitte_type_registry_is_initialized(registry) || !vitte_builtin_type_kind_is_valid(kind)) {
        vitte_type_registry_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_TYPE_E_KIND", "invalid builtin type kind", NULL);
        return NULL;
    }
    vitte_error_reset(&registry->last_error);
    return &registry->builtin_types[kind];
}

const vitte_type_t *vitte_type_lookup(
    vitte_type_registry_t *registry,
    const char *name
) {
    const vitte_builtin_type_t *builtin;
    size_t index;

    if (!vitte_type_registry_is_initialized(registry) || name == NULL || name[0] == '\0') {
        vitte_type_registry_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_TYPE_E_LOOKUP", "invalid type lookup name", name);
        return NULL;
    }
    for (index = 0u; index < registry->pick_type_count; index++) {
        if (strcmp(registry->pick_types[index].name, name) == 0) {
            vitte_error_reset(&registry->last_error);
            return &registry->pick_types[index];
        }
    }
    for (index = 0u; index < registry->form_type_count; index++) {
        if (strcmp(registry->form_types[index].name, name) == 0) {
            vitte_error_reset(&registry->last_error);
            return &registry->form_types[index];
        }
    }
    builtin = vitte_builtin_lookup_type(&registry->builtins, name);
    if (builtin == NULL) {
        vitte_error_copy(&registry->last_error, vitte_builtin_registry_last_error(&registry->builtins));
        return NULL;
    }
    vitte_error_reset(&registry->last_error);
    return &registry->builtin_types[builtin->kind];
}

const vitte_type_t *vitte_type_register_pick(vitte_type_registry_t *registry, const char *name) {
    vitte_type_t *type;

    if (!vitte_type_registry_is_initialized(registry) || name == NULL || name[0] == '\0') {
        vitte_type_registry_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_TYPE_E_PICK", "invalid pick type name", name);
        return NULL;
    }
    type = (vitte_type_t *)vitte_type_lookup(registry, name);
    if (type != NULL && type->kind == VITTE_TYPE_KIND_PICK) {
        return type;
    }
    if (registry->pick_type_count >= VITTE_TYPE_MAX_PICK_TYPES) {
        vitte_type_registry_set_error(registry, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_TYPE_E_PICK", "pick type registry is full", name);
        return NULL;
    }
    type = &registry->pick_types[registry->pick_type_count++];
    memset(type, 0, sizeof(*type));
    type->kind = VITTE_TYPE_KIND_PICK;
    type->name = name;
    type->builtin_kind = VITTE_BUILTIN_TYPE_ERROR;
    type->valid = true;
    type->error = false;
    vitte_error_reset(&registry->last_error);
    return type;
}

const vitte_type_t *vitte_type_register_form(vitte_type_registry_t *registry, const char *name) {
    vitte_type_t *type;
    if (!vitte_type_registry_is_initialized(registry) || name == NULL || name[0] == '\0') {
        vitte_type_registry_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_TYPE_E_FORM", "invalid form type name", name);
        return NULL;
    }
    type = (vitte_type_t *)vitte_type_lookup(registry, name);
    if (type != NULL && type->kind == VITTE_TYPE_KIND_FORM) return type;
    if (registry->form_type_count >= VITTE_TYPE_MAX_FORM_TYPES) {
        vitte_type_registry_set_error(registry, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_TYPE_E_FORM", "form type registry is full", name);
        return NULL;
    }
    type = &registry->form_types[registry->form_type_count++];
    memset(type, 0, sizeof(*type));
    type->kind = VITTE_TYPE_KIND_FORM;
    type->name = name;
    type->builtin_kind = VITTE_BUILTIN_TYPE_ERROR;
    type->valid = true;
    vitte_error_reset(&registry->last_error);
    return type;
}

const vitte_type_t *vitte_type_from_ast(
    vitte_type_registry_t *registry,
    const vitte_ast_type_ref_t *type_ref
) {
    if (!vitte_type_registry_is_initialized(registry) || type_ref == NULL || type_ref->kind != VITTE_AST_NODE_TYPE_NAME || type_ref->as.type_name.name == NULL) {
        vitte_type_registry_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_TYPE_E_AST", "invalid AST type reference", NULL);
        return NULL;
    }
    return vitte_type_lookup(registry, type_ref->as.type_name.name);
}

const char *vitte_type_kind_name(vitte_type_kind_t kind) {
    switch (kind) {
        case VITTE_TYPE_KIND_INVALID:
            return "invalid";
        case VITTE_TYPE_KIND_BUILTIN:
            return "builtin";
        case VITTE_TYPE_KIND_PROC:
            return "proc";
        case VITTE_TYPE_KIND_PICK:
            return "pick";
        case VITTE_TYPE_KIND_FORM:
            return "form";
        default:
            return "unknown";
    }
}

const char *vitte_type_name(const vitte_type_t *type) {
    if (type == NULL) {
        return "<null>";
    }
    if (type->name != NULL) {
        return type->name;
    }
    if (type->kind == VITTE_TYPE_KIND_BUILTIN) {
        return vitte_builtin_type_kind_name(type->builtin_kind);
    }
    return "<type>";
}

bool vitte_type_is_valid(const vitte_type_t *type) {
    return type != NULL && type->valid;
}

bool vitte_type_is_builtin(const vitte_type_t *type) {
    return vitte_type_is_valid(type) && type->kind == VITTE_TYPE_KIND_BUILTIN;
}

bool vitte_type_is_proc(const vitte_type_t *type) {
    return vitte_type_is_valid(type) && type->kind == VITTE_TYPE_KIND_PROC;
}

bool vitte_type_is_error(const vitte_type_t *type) {
    return type == NULL ||
        !type->valid ||
        (type->kind == VITTE_TYPE_KIND_BUILTIN && type->builtin_kind == VITTE_BUILTIN_TYPE_ERROR) ||
        type->error;
}

bool vitte_type_is_void(const vitte_type_t *type) {
    return vitte_type_is_builtin(type) && type->builtin_kind == VITTE_BUILTIN_TYPE_VOID;
}

bool vitte_type_is_bool(const vitte_type_t *type) {
    return vitte_type_is_builtin(type) && type->builtin_kind == VITTE_BUILTIN_TYPE_BOOL;
}

bool vitte_type_is_integer(const vitte_type_t *type) {
    return vitte_type_is_builtin(type) && vitte_builtin_type_is_integer(type->builtin_kind);
}

bool vitte_type_is_numeric(const vitte_type_t *type) {
    return vitte_type_is_builtin(type) && vitte_builtin_type_is_numeric(type->builtin_kind);
}

bool vitte_type_is_textual(const vitte_type_t *type) {
    return vitte_type_is_builtin(type) && vitte_builtin_type_is_textual(type->builtin_kind);
}

bool vitte_type_equals(const vitte_type_t *left, const vitte_type_t *right) {
    if (!vitte_type_is_valid(left) || !vitte_type_is_valid(right) || left->kind != right->kind) {
        return false;
    }
    if (left->kind == VITTE_TYPE_KIND_BUILTIN) {
        return left->builtin_kind == right->builtin_kind;
    }
    if (left->kind == VITTE_TYPE_KIND_PROC) {
        size_t index;

        if (left->arity != right->arity ||
            left->variadic != right->variadic ||
            !vitte_type_equals(left->return_type, right->return_type)) {
            return false;
        }
        for (index = 0u; index < left->arity; index++) {
            if (!vitte_type_equals(left->parameter_types[index], right->parameter_types[index])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool vitte_type_is_assignable(const vitte_type_t *destination, const vitte_type_t *source) {
    if (!vitte_type_is_valid(destination) || !vitte_type_is_valid(source)) {
        return false;
    }
    if (vitte_type_equals(destination, source)) {
        return true;
    }
    if (vitte_type_is_integer(destination) && vitte_type_is_integer(source)) {
        return true;
    }
    if (vitte_type_is_numeric(destination) && vitte_type_is_numeric(source) &&
        destination->builtin_kind == source->builtin_kind) {
        return true;
    }
    return false;
}

bool vitte_type_is_condition(const vitte_type_t *type) {
    return vitte_type_is_bool(type) || vitte_type_is_integer(type);
}

const vitte_type_t *vitte_type_proc_parameter(const vitte_type_t *type, size_t index) {
    if (!vitte_type_is_proc(type) || index >= type->arity || index >= VITTE_TYPE_MAX_PROC_PARAMETERS) {
        return NULL;
    }
    return type->parameter_types[index];
}
