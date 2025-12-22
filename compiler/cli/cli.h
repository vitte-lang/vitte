// Vitte Compiler - CLI Framework
// Enterprise-grade command line interface system

#ifndef VITTE_CLI_H
#define VITTE_CLI_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types and Structures
// ============================================================================

typedef enum {
    CLI_ARG_NONE,
    CLI_ARG_BOOL,
    CLI_ARG_STRING,
    CLI_ARG_INT,
    CLI_ARG_FLOAT,
    CLI_ARG_MULTI,          // Multiple values
    CLI_ARG_ENUM,           // Enumerated choice
    CLI_ARG_PATH,           // File/directory path
    CLI_ARG_LIST,           // Comma-separated list
} cli_arg_type_t;

typedef enum {
    CLI_OK = 0,
    CLI_ERR_INVALID_ARG,
    CLI_ERR_MISSING_ARG,
    CLI_ERR_UNKNOWN_OPTION,
    CLI_ERR_TYPE_MISMATCH,
    CLI_ERR_FILE_NOT_FOUND,
    CLI_ERR_INVALID_ENUM,
    CLI_ERR_RANGE_ERROR,
    CLI_ERR_PARSE_ERROR,
    CLI_ERR_USAGE,
} cli_error_t;

typedef struct cli_option {
    const char *short_name;      // -x or NULL
    const char *long_name;       // --option
    const char *description;     // Help text
    cli_arg_type_t type;
    
    // Value storage
    void *value;                 // Pointer to store value
    const char *default_value;   // Default if not specified
    
    // Validation
    bool required;               // Must be provided
    int min_value;               // For INT type
    int max_value;               // For INT type
    const char **enum_values;    // For ENUM type
    int enum_count;
    
    // Callbacks
    bool (*validator)(const char *value);
    void (*on_change)(const char *value);
} cli_option_t;

struct cli_context;

typedef struct cli_command {
    const char *name;
    const char *description;
    const char *long_description;
    const char *group;          // Logical grouping for help output
    
    // Options for this command
    cli_option_t *options;
    size_t option_count;
    
    // Positional arguments
    const char **positional_args;
    size_t positional_count;
    
    // Execution
    int (*execute)(struct cli_context *ctx,
                   struct cli_command *cmd,
                   int argc,
                   char **argv);
    
    // Subcommands
    struct cli_command *subcommands;
    size_t subcommand_count;
} cli_command_t;

typedef struct cli_config {
    const char *app_name;
    const char *version;
    const char *description;
    const char *author;
    const char *license;
    
    // Behavior
    bool show_help_on_error;
    bool case_sensitive_options;
    bool allow_abbreviations;
    
    // Callbacks
    void (*on_error)(cli_error_t error, const char *message);
    void (*on_help)(const char *text);
    void (*on_version)(const char *text);
    
    // Log level
    int verbosity;  // 0=quiet, 1=normal, 2=verbose, 3=debug
} cli_config_t;

typedef struct cli_context {
    cli_config_t config;
    cli_command_t *commands;
    size_t command_count;
    
    // Parsed values
    char **parsed_args;
    size_t parsed_count;
    
    // Current command being executed
    cli_command_t *current_command;

    // Global execution state
    char *profile;              // Selected profile (dev, release, debug-asm)
    char *preset_path;          // Loaded preset file path (e.g., .vittec/config)
    bool preset_loaded;
    vitte_config_t *config_store;
} cli_context_t;

typedef struct cli_parser_state {
    int argc;
    char **argv;
    int current_index;
    
    cli_error_t last_error;
    char error_message[512];
} cli_parser_state_t;

// ============================================================================
// Core API
// ============================================================================

// Initialize CLI context
cli_context_t* cli_init(void);

// Free CLI context
void cli_free(cli_context_t *ctx);

// Register command
cli_error_t cli_register_command(cli_context_t *ctx, cli_command_t cmd);

// Parse command line arguments
cli_error_t cli_parse(cli_context_t *ctx, int argc, char **argv);

// Execute parsed command
int cli_execute(cli_context_t *ctx);

// ============================================================================
// Option Management
// ============================================================================

// Create option builder
cli_option_t cli_option_builder(const char *long_name);

// Chainable option configuration
cli_option_t* cli_set_short_name(cli_option_t *opt, const char *name);
cli_option_t* cli_set_description(cli_option_t *opt, const char *desc);
cli_option_t* cli_set_type(cli_option_t *opt, cli_arg_type_t type);
cli_option_t* cli_set_required(cli_option_t *opt, bool required);
cli_option_t* cli_set_default(cli_option_t *opt, const char *value);
cli_option_t* cli_set_validator(cli_option_t *opt, bool (*validator)(const char*));
cli_option_t* cli_set_enum_values(cli_option_t *opt, const char **values, int count);

// Value retrieval
const char* cli_get_string(cli_context_t *ctx, const char *option_name);
int cli_get_int(cli_context_t *ctx, const char *option_name);
bool cli_get_bool(cli_context_t *ctx, const char *option_name);
double cli_get_float(cli_context_t *ctx, const char *option_name);
char** cli_get_multi(cli_context_t *ctx, const char *option_name, int *count);

// ============================================================================
// Validation
// ============================================================================

// Built-in validators
bool cli_validate_email(const char *value);
bool cli_validate_url(const char *value);
bool cli_validate_ipv4(const char *value);
bool cli_validate_integer(const char *value);
bool cli_validate_float(const char *value);
bool cli_validate_file_exists(const char *value);
bool cli_validate_dir_exists(const char *value);
bool cli_validate_regex(const char *value, const char *pattern);

// ============================================================================
// Help and Documentation
// ============================================================================

// Generate help text
char* cli_generate_help(cli_context_t *ctx, cli_command_t *cmd);

// Print help
void cli_print_help(cli_context_t *ctx, cli_command_t *cmd);

// Print usage
void cli_print_usage(cli_context_t *ctx, cli_command_t *cmd);

// Print version
void cli_print_version(cli_context_t *ctx);

// ============================================================================
// Error Handling
// ============================================================================

// Get last error message
const char* cli_get_error_message(cli_context_t *ctx);

// Format error message
char* cli_format_error(cli_error_t error, const char *detail);

// ============================================================================
// Logging
// ============================================================================

void cli_log_debug(cli_context_t *ctx, const char *fmt, ...);
void cli_log_info(cli_context_t *ctx, const char *fmt, ...);
void cli_log_warning(cli_context_t *ctx, const char *fmt, ...);
void cli_log_error(cli_context_t *ctx, const char *fmt, ...);

// ============================================================================
// Utilities
// ============================================================================

// Color output
typedef enum {
    CLI_COLOR_RESET,
    CLI_COLOR_RED,
    CLI_COLOR_GREEN,
    CLI_COLOR_YELLOW,
    CLI_COLOR_BLUE,
    CLI_COLOR_CYAN,
} cli_color_t;

void cli_print_colored(cli_color_t color, const char *fmt, ...);

// Progress bar
void cli_progress_start(const char *label, size_t total);
void cli_progress_update(size_t current);
void cli_progress_finish(void);

// Confirmation prompt
bool cli_confirm(const char *prompt);

// Interactive input
char* cli_prompt(const char *prompt, bool hidden);

#ifdef __cplusplus
}
#endif

#endif  // VITTE_CLI_H
