// Vitte Compiler - Configuration Implementation
// Flexible configuration management for enterprise use

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// Helper Functions
// ============================================================================

static char* config_strdup(const char *str) {
    if (!str) return NULL;
    char *copy = malloc(strlen(str) + 1);
    if (copy) strcpy(copy, str);
    return copy;
}

static char* config_trim(char *str) {
    if (!str) return str;
    
    // Trim leading whitespace
    while (*str && isspace((unsigned char)*str)) str++;
    
    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    
    return str;
}

static char* config_read_value(char *line) {
    char *eq = strchr(line, '=');
    if (!eq) return NULL;
    
    return config_trim(eq + 1);
}

// ============================================================================
// Configuration API Implementation
// ============================================================================

vitte_config_t* vitte_config_create(void) {
    vitte_config_t *config = malloc(sizeof(vitte_config_t));
    if (!config) return NULL;
    
    config->entries = NULL;
    config->count = 0;
    config->capacity = 0;
    config->config_file = NULL;
    config->config_dir = NULL;
    
    // Allocate initial capacity
    config->capacity = 32;
    config->entries = malloc(sizeof(vitte_config_entry_t) * config->capacity);
    if (!config->entries) {
        free(config);
        return NULL;
    }
    
    return config;
}

int vitte_config_set(vitte_config_t *config, const char *key,
                     const void *value, vitte_config_type_t type) {
    if (!config || !key) return -1;
    
    // Find existing entry
    for (size_t i = 0; i < config->count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            // Free old value
            free(config->entries[i].value);
            
            // Set new value
            if (value) {
                size_t size;
                switch (type) {
                    case VITTE_CONFIG_STRING:
                    case VITTE_CONFIG_PATH:
                        size = strlen((const char*)value) + 1;
                        config->entries[i].value = malloc(size);
                        if (config->entries[i].value)
                            strcpy((char*)config->entries[i].value, (const char*)value);
                        break;
                    case VITTE_CONFIG_INT:
                        config->entries[i].value = malloc(sizeof(int));
                        if (config->entries[i].value)
                            *(int*)config->entries[i].value = *(const int*)value;
                        break;
                    case VITTE_CONFIG_BOOL:
                        config->entries[i].value = malloc(sizeof(int));
                        if (config->entries[i].value)
                            *(int*)config->entries[i].value = *(const int*)value;
                        break;
                    case VITTE_CONFIG_FLOAT:
                        config->entries[i].value = malloc(sizeof(float));
                        if (config->entries[i].value)
                            *(float*)config->entries[i].value = *(const float*)value;
                        break;
                }
            } else {
                config->entries[i].value = NULL;
            }
            
            config->entries[i].type = type;
            return 0;
        }
    }
    
    // Create new entry
    if (config->count >= config->capacity) {
        config->capacity *= 2;
        vitte_config_entry_t *new_entries = realloc(config->entries,
            sizeof(vitte_config_entry_t) * config->capacity);
        if (!new_entries) return -1;
        config->entries = new_entries;
    }
    
    config->entries[config->count].key = config_strdup(key);
    config->entries[config->count].type = type;
    
    // Allocate and set value
    if (value) {
        size_t size;
        switch (type) {
            case VITTE_CONFIG_STRING:
            case VITTE_CONFIG_PATH:
                size = strlen((const char*)value) + 1;
                config->entries[config->count].value = malloc(size);
                if (config->entries[config->count].value)
                    strcpy((char*)config->entries[config->count].value, (const char*)value);
                break;
            case VITTE_CONFIG_INT:
            case VITTE_CONFIG_BOOL:
                config->entries[config->count].value = malloc(sizeof(int));
                if (config->entries[config->count].value)
                    *(int*)config->entries[config->count].value = *(const int*)value;
                break;
            case VITTE_CONFIG_FLOAT:
                config->entries[config->count].value = malloc(sizeof(float));
                if (config->entries[config->count].value)
                    *(float*)config->entries[config->count].value = *(const float*)value;
                break;
        }
    } else {
        config->entries[config->count].value = NULL;
    }
    
    config->count++;
    return 0;
}

void* vitte_config_get(vitte_config_t *config, const char *key) {
    if (!config || !key) return NULL;
    
    for (size_t i = 0; i < config->count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            return config->entries[i].value;
        }
    }
    
    return NULL;
}

void* vitte_config_get_default(vitte_config_t *config, const char *key,
                               const void *default_value) {
    void *value = vitte_config_get(config, key);
    return value ? value : (void*)default_value;
}

int vitte_config_load_file(vitte_config_t *config, const char *path) {
    if (!config || !path) return -1;
    
    FILE *file = fopen(path, "r");
    if (!file) return -1;
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        
        // Skip empty lines and comments
        if (!line[0] || line[0] == '#') continue;
        
        // Parse key=value
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        char *key = line;
        char *value = config_trim(eq + 1);
        
        *eq = '\0';
        key = config_trim(key);
        
        // Simple type detection
        vitte_config_type_t type = VITTE_CONFIG_STRING;
        if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
            type = VITTE_CONFIG_BOOL;
        } else if (value[0] == '/' || value[0] == '\\') {
            type = VITTE_CONFIG_PATH;
        }
        
        vitte_config_set(config, key, value, type);
    }
    
    fclose(file);
    config->config_file = config_strdup(path);
    return 0;
}

int vitte_config_load_env(vitte_config_t *config) {
    if (!config) return -1;
    
    // Load VITTE_* environment variables
    const char *env_vars[] = {
        "VITTE_VERBOSE", "VITTE_OPTIMIZATION", "VITTE_DEBUG",
        "VITTE_HOME", "VITTE_INCLUDE", NULL
    };
    
    for (int i = 0; env_vars[i]; i++) {
        const char *value = getenv(env_vars[i]);
        if (value) {
            vitte_config_set(config, env_vars[i], value, VITTE_CONFIG_STRING);
        }
    }
    
    return 0;
}

int vitte_config_save(vitte_config_t *config, const char *path) {
    if (!config || !path) return -1;
    
    FILE *file = fopen(path, "w");
    if (!file) return -1;
    
    fprintf(file, "# Vitte Compiler Configuration\n");
    fprintf(file, "# Generated configuration file\n\n");
    
    for (size_t i = 0; i < config->count; i++) {
        fprintf(file, "%s=", config->entries[i].key);
        
        switch (config->entries[i].type) {
            case VITTE_CONFIG_STRING:
            case VITTE_CONFIG_PATH:
                fprintf(file, "%s\n", (char*)config->entries[i].value);
                break;
            case VITTE_CONFIG_INT:
                fprintf(file, "%d\n", *(int*)config->entries[i].value);
                break;
            case VITTE_CONFIG_BOOL:
                fprintf(file, "%s\n", *(int*)config->entries[i].value ? "true" : "false");
                break;
            case VITTE_CONFIG_FLOAT:
                fprintf(file, "%f\n", *(float*)config->entries[i].value);
                break;
        }
    }
    
    fclose(file);
    config->config_file = config_strdup(path);
    return 0;
}

void vitte_config_print(vitte_config_t *config) {
    if (!config) return;
    
    printf("Configuration Entries:\n");
    printf("─────────────────────────────────────────────\n");
    
    for (size_t i = 0; i < config->count; i++) {
        printf("  %-40s = ", config->entries[i].key);
        
        switch (config->entries[i].type) {
            case VITTE_CONFIG_STRING:
            case VITTE_CONFIG_PATH:
                printf("%s", (char*)config->entries[i].value);
                break;
            case VITTE_CONFIG_INT:
                printf("%d", *(int*)config->entries[i].value);
                break;
            case VITTE_CONFIG_BOOL:
                printf("%s", *(int*)config->entries[i].value ? "true" : "false");
                break;
            case VITTE_CONFIG_FLOAT:
                printf("%f", *(float*)config->entries[i].value);
                break;
        }
        printf("\n");
    }
}

void vitte_config_set_defaults(vitte_config_t *config) {
    if (!config) return;
    
    // Default values
    int verbosity = 1;
    int opt_level = 2;
    int bool_true = 1;
    int bool_false = 0;
    float timeout = 30.0f;
    
    vitte_config_set(config, "verbosity.level", &verbosity, VITTE_CONFIG_INT);
    vitte_config_set(config, "compile.optimization", &opt_level, VITTE_CONFIG_INT);
    vitte_config_set(config, "output.color", &bool_true, VITTE_CONFIG_BOOL);
    vitte_config_set(config, "compile.emit_ir", &bool_false, VITTE_CONFIG_BOOL);
    vitte_config_set(config, "compile.emit_asm", &bool_false, VITTE_CONFIG_BOOL);
    vitte_config_set(config, "compile.debug", &bool_false, VITTE_CONFIG_BOOL);
    vitte_config_set(config, "build.parallel", &bool_true, VITTE_CONFIG_BOOL);
    
    const char *format = "text";
    const char *cache = ".vitte/cache";
    
    vitte_config_set(config, "output.format", format, VITTE_CONFIG_STRING);
    vitte_config_set(config, "cache.directory", cache, VITTE_CONFIG_PATH);
}

void vitte_config_free(vitte_config_t *config) {
    if (!config) return;
    
    for (size_t i = 0; i < config->count; i++) {
        free(config->entries[i].key);
        free(config->entries[i].value);
    }
    
    free(config->entries);
    free(config->config_file);
    free(config->config_dir);
    free(config);
}
