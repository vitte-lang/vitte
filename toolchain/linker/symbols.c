/*
 * VITTE Toolchain Symbol Table Implementation
 * Complete symbol management, resolution and verification
 */

#include "symbols.h"

symbol_table_t *symbol_table_create(void) {
    symbol_table_t *table = malloc(sizeof(symbol_table_t));
    if (!table) return NULL;
    table->entry_count = 0;
    memset(table->entries, 0, sizeof(table->entries));
    return table;
}

void symbol_table_free(symbol_table_t *table) {
    if (!table) return;
    for (int i = 0; i < table->entry_count; i++) {
        if (table->entries[i].name) {
            free(table->entries[i].name);
            table->entries[i].name = NULL;
        }
    }
    free(table);
}

int symbol_table_add(symbol_table_t *table, const char *name, symbol_type_t type, uint64_t address, uint64_t size) {
    if (!table || !name) return -1;
    if (table->entry_count >= MAX_SYMBOLS) {
        fprintf(stderr, "[symbols] ✗ Symbol table full (%d max symbols)\n", MAX_SYMBOLS);
        return -1;
    }
    symbol_entry_t *entry = &table->entries[table->entry_count++];
    entry->name = malloc(strlen(name) + 1);
    if (!entry->name) return -1;
    strcpy(entry->name, name);
    entry->type = type;
    entry->address = address;
    entry->size = size;
    entry->visibility = (type == SYM_FUNCTION) ? 1 : 0;
    return 0;
}

int symbol_table_lookup(symbol_table_t *table, const char *name, symbol_entry_t *entry) {
    if (!table || !name || !entry) return -1;
    for (int i = 0; i < table->entry_count; i++) {
        if (table->entries[i].name && strcmp(table->entries[i].name, name) == 0) {
            *entry = table->entries[i];
            return 0;
        }
    }
    return -1;
}

int symbol_table_remove(symbol_table_t *table, const char *name) {
    if (!table || !name) return -1;
    for (int i = 0; i < table->entry_count; i++) {
        if (table->entries[i].name && strcmp(table->entries[i].name, name) == 0) {
            free(table->entries[i].name);
            if (i < table->entry_count - 1) {
                memmove(&table->entries[i], &table->entries[i + 1], sizeof(symbol_entry_t) * (table->entry_count - i - 1));
            }
            table->entry_count--;
            return 0;
        }
    }
    return -1;
}

int symbol_resolve_all(symbol_table_t *table) {
    if (!table) return -1;
    int resolved = 0, undefined = 0;
    for (int i = 0; i < table->entry_count; i++) {
        if (table->entries[i].type != SYM_UNDEFINED) {
            resolved++;
        } else {
            undefined++;
        }
    }
    fprintf(stderr, "[symbols] ✓ Resolution report: %d resolved, %d undefined (total: %d)\n", resolved, undefined, table->entry_count);
    return resolved;
}

int symbol_check_undefined(symbol_table_t *table) {
    if (!table) return -1;
    int undefined = 0;
    for (int i = 0; i < table->entry_count; i++) {
        if (table->entries[i].type == SYM_UNDEFINED) {
            fprintf(stderr, "[symbols] ⚠ Undefined: %s@0x%lx\n", table->entries[i].name, table->entries[i].address);
            undefined++;
        }
    }
    if (undefined > 0) {
        fprintf(stderr, "[symbols] ✗ %d undefined symbol(s) found\n", undefined);
    } else {
        fprintf(stderr, "[symbols] ✓ All symbols resolved\n");
    }
    return undefined == 0 ? 0 : -1;
}
