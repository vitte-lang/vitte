// Vitte Compiler - Configuration Management
// Handles configuration files, environment variables, and preferences

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#ifndef VITTE_CONFIG_H
#define VITTE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration Structure
// ============================================================================

typedef enum {
    VITTE_CONFIG_STRING,
    VITTE_CONFIG_INT,
    VITTE_CONFIG_BOOL,
    VITTE_CONFIG_FLOAT,
    VITTE_CONFIG_PATH,
} vitte_config_type_t;

typedef struct {
    char *key;
    void *value;
    vitte_config_type_t type;
} vitte_config_entry_t;

typedef struct {
    vitte_config_entry_t *entries;
    size_t count;
    size_t capacity;
    char *config_file;
    char *config_dir;
} vitte_config_t;

// ============================================================================
// Configuration API
// ============================================================================

// Create and initialize configuration
vitte_config_t* vitte_config_create(void);

// Load from configuration file
int vitte_config_load_file(vitte_config_t *config, const char *path);

// Load from environment variables
int vitte_config_load_env(vitte_config_t *config);

// Set configuration value
int vitte_config_set(vitte_config_t *config, const char *key, 
                     const void *value, vitte_config_type_t type);

// Get configuration value
void* vitte_config_get(vitte_config_t *config, const char *key);

// Get with default fallback
void* vitte_config_get_default(vitte_config_t *config, const char *key,
                               const void *default_value);

// Save to file
int vitte_config_save(vitte_config_t *config, const char *path);

// Print configuration
void vitte_config_print(vitte_config_t *config);

// Free configuration
void vitte_config_free(vitte_config_t *config);

// ============================================================================
// Default Configuration
// ============================================================================

void vitte_config_set_defaults(vitte_config_t *config);

// ============================================================================
// Common Configuration Keys
// ============================================================================

#define VITTE_CONFIG_OUTPUT_FORMAT      "output.format"      // "text", "json", "xml"
#define VITTE_CONFIG_VERBOSITY_LEVEL    "verbosity.level"    // 0-3
#define VITTE_CONFIG_COLOR_OUTPUT       "output.color"       // true/false
#define VITTE_CONFIG_OPTIMIZATION       "compile.optimization" // 0-3
#define VITTE_CONFIG_EMIT_IR            "compile.emit_ir"    // true/false
#define VITTE_CONFIG_EMIT_ASM           "compile.emit_asm"   // true/false
#define VITTE_CONFIG_EMIT_C             "compile.emit_c"     // true/false
#define VITTE_CONFIG_DEBUG_SYMBOLS      "compile.debug"      // true/false
#define VITTE_CONFIG_PARALLEL_BUILD     "build.parallel"     // true/false
#define VITTE_CONFIG_NUM_THREADS        "build.threads"      // integer
#define VITTE_CONFIG_CACHE_DIR          "cache.directory"    // path
#define VITTE_CONFIG_PROJECT_ROOT       "project.root"       // path
#define VITTE_CONFIG_INCLUDE_PATHS      "include.paths"      // paths list

#ifdef __cplusplus
}
#endif

#endif // VITTE_CONFIG_H
