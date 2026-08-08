#include "symbol.h"

#include <string.h>

static void vitte_symbol_set_error(
    vitte_symbol_table_t *table,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (table != NULL) {
        vitte_error_set_details(&table->last_error, status, code, message, details);
    }
}

void vitte_symbol_table_init(vitte_symbol_table_t *table) {
    if (table == NULL) {
        return;
    }
    memset(table, 0, sizeof(*table));
    vitte_error_init(&table->last_error);
    table->initialized = true;
}

void vitte_symbol_table_destroy(vitte_symbol_table_t *table) {
    if (table == NULL) {
        return;
    }
    memset(table, 0, sizeof(*table));
}

bool vitte_symbol_table_is_initialized(const vitte_symbol_table_t *table) {
    return table != NULL && table->initialized;
}

const vitte_error_t *vitte_symbol_table_last_error(const vitte_symbol_table_t *table) {
    return table != NULL ? &table->last_error : vitte_error_last();
}

void vitte_symbol_table_clear_error(vitte_symbol_table_t *table) {
    if (table != NULL) {
        vitte_error_reset(&table->last_error);
    }
}

size_t vitte_symbol_count(const vitte_symbol_table_t *table) {
    return vitte_symbol_table_is_initialized(table) ? table->count : 0u;
}

const vitte_symbol_t *vitte_symbol_at(const vitte_symbol_table_t *table, size_t index) {
    return vitte_symbol_table_is_initialized(table) && index < table->count ? &table->entries[index] : NULL;
}

const vitte_symbol_t *vitte_symbol_lookup(const vitte_symbol_table_t *table, const char *name) {
    size_t index;

    if (!vitte_symbol_table_is_initialized(table) || name == NULL || name[0] == '\0') {
        return NULL;
    }
    for (index = table->count; index > 0u; index--) {
        const vitte_symbol_t *symbol = &table->entries[index - 1u];
        if (symbol->initialized && symbol->name != NULL && strcmp(symbol->name, name) == 0) {
            return symbol;
        }
    }
    return NULL;
}

static vitte_status_t vitte_symbol_append(
    vitte_symbol_table_t *table,
    const vitte_symbol_t *symbol,
    const vitte_symbol_t **out_symbol
) {
    vitte_symbol_t *stored;

    if (!vitte_symbol_table_is_initialized(table) || symbol == NULL) {
        vitte_symbol_set_error(table, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_SYMBOL_E_ARGUMENT", "invalid symbol append", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (table->count >= VITTE_SYMBOL_MAX_ENTRIES) {
        vitte_symbol_set_error(table, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_SYMBOL_E_LIMIT", "symbol table is full", symbol->name);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    table->entries[table->count] = *symbol;
    stored = &table->entries[table->count];
    if (symbol->type == &symbol->owned_type) {
        stored->type = &stored->owned_type;
    }
    if (out_symbol != NULL) {
        *out_symbol = stored;
    }
    table->count++;
    vitte_error_reset(&table->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_symbol_define(
    vitte_symbol_table_t *table,
    vitte_symbol_kind_t kind,
    const char *name,
    const vitte_type_t *type,
    const vitte_ast_node_t *declaration,
    bool mutable_value,
    const vitte_symbol_t **out_symbol
) {
    vitte_symbol_t symbol;

    if (!vitte_symbol_table_is_initialized(table) || name == NULL || name[0] == '\0' || !vitte_type_is_valid(type)) {
        vitte_symbol_set_error(table, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_SYMBOL_E_ARGUMENT", "invalid symbol definition", name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(&symbol, 0, sizeof(symbol));
    symbol.kind = kind;
    symbol.name = name;
    symbol.type = type;
    symbol.declaration = declaration;
    symbol.mutable_value = mutable_value;
    symbol.initialized = true;
    return vitte_symbol_append(table, &symbol, out_symbol);
}

vitte_status_t vitte_symbol_define_proc(
    vitte_symbol_table_t *table,
    const char *name,
    const vitte_type_t *return_type,
    size_t arity,
    bool variadic,
    const vitte_ast_node_t *declaration,
    const vitte_symbol_t **out_symbol
) {
    vitte_symbol_t symbol;

    if (!vitte_symbol_table_is_initialized(table) || name == NULL || name[0] == '\0' || !vitte_type_is_valid(return_type)) {
        vitte_symbol_set_error(table, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_SYMBOL_E_PROC", "invalid procedure symbol definition", name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(&symbol, 0, sizeof(symbol));
    symbol.kind = VITTE_SYMBOL_KIND_PROC;
    symbol.name = name;
    symbol.declaration = declaration;
    symbol.initialized = true;
    vitte_type_init_proc(&symbol.owned_type, name, return_type, arity, variadic);
    symbol.type = &symbol.owned_type;
    return vitte_symbol_append(table, &symbol, out_symbol);
}

vitte_status_t vitte_symbol_define_builtin_constant(
    vitte_symbol_table_t *table,
    const vitte_builtin_constant_t *builtin_constant,
    const vitte_type_t *type,
    const vitte_symbol_t **out_symbol
) {
    vitte_symbol_t symbol;

    if (!vitte_symbol_table_is_initialized(table) || builtin_constant == NULL || !vitte_type_is_valid(type)) {
        vitte_symbol_set_error(table, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_SYMBOL_E_BUILTIN", "invalid builtin constant symbol definition", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(&symbol, 0, sizeof(symbol));
    symbol.kind = VITTE_SYMBOL_KIND_BUILTIN_CONST;
    symbol.name = builtin_constant->name;
    symbol.type = type;
    symbol.builtin_constant = builtin_constant;
    symbol.builtin = true;
    symbol.initialized = true;
    return vitte_symbol_append(table, &symbol, out_symbol);
}

vitte_status_t vitte_symbol_define_builtin_function(
    vitte_symbol_table_t *table,
    const vitte_builtin_function_t *builtin_function,
    const vitte_type_t *return_type,
    const vitte_symbol_t **out_symbol
) {
    vitte_symbol_t symbol;

    if (!vitte_symbol_table_is_initialized(table) || builtin_function == NULL || !vitte_type_is_valid(return_type)) {
        vitte_symbol_set_error(table, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_SYMBOL_E_BUILTIN", "invalid builtin function symbol definition", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(&symbol, 0, sizeof(symbol));
    symbol.kind = VITTE_SYMBOL_KIND_BUILTIN_FUNC;
    symbol.name = builtin_function->name;
    symbol.builtin_function = builtin_function;
    symbol.builtin = true;
    symbol.initialized = true;
    vitte_type_init_proc(&symbol.owned_type, builtin_function->name, return_type, builtin_function->max_arity, builtin_function->variadic);
    symbol.type = &symbol.owned_type;
    return vitte_symbol_append(table, &symbol, out_symbol);
}

const char *vitte_symbol_kind_name(vitte_symbol_kind_t kind) {
    switch (kind) {
        case VITTE_SYMBOL_KIND_UNKNOWN:
            return "unknown";
        case VITTE_SYMBOL_KIND_CONST:
            return "const";
        case VITTE_SYMBOL_KIND_PROC:
            return "proc";
        case VITTE_SYMBOL_KIND_LOCAL:
            return "local";
        case VITTE_SYMBOL_KIND_PARAM:
            return "param";
        case VITTE_SYMBOL_KIND_BUILTIN_CONST:
            return "builtin-const";
        case VITTE_SYMBOL_KIND_BUILTIN_FUNC:
            return "builtin-func";
        default:
            return "invalid";
    }
}

bool vitte_symbol_kind_is_value(vitte_symbol_kind_t kind) {
    return kind == VITTE_SYMBOL_KIND_CONST ||
        kind == VITTE_SYMBOL_KIND_LOCAL ||
        kind == VITTE_SYMBOL_KIND_PARAM ||
        kind == VITTE_SYMBOL_KIND_BUILTIN_CONST;
}
