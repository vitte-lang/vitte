// Vitte Compiler - CLI Implementation
// Enterprise-grade command line interface

#include "cli.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

// ============================================================================ 
// Static Helpers
// ============================================================================

static char* cli_strdup(const char *str) {
    if (!str) return NULL;
    char *dup = malloc(strlen(str) + 1);
    if (dup) strcpy(dup, str);
    return dup;
}

static void cli_trim(char *str) {
    if (!str) return;
    char *start = str;
    while (*start && isspace(*start)) start++;
    char *end = str + strlen(str) - 1;
    while (end >= start && isspace(*end)) end--;
    end[1] = '\0';
    if (start != str) memmove(str, start, end - start + 2);
}

static bool cli_str_eq(const char *a, const char *b, bool case_sensitive) {
    if (!a || !b) return a == b;
    return case_sensitive ? strcmp(a, b) == 0 : strcasecmp(a, b) == 0;
}

static char** cli_split_string(const char *str, const char *delim, int *count) {
    if (!str || !delim) return NULL;
    
    char *copy = cli_strdup(str);
    if (!copy) return NULL;
    
    *count = 0;
    for (int i = 0; copy[i]; i++) {
        if (strchr(delim, copy[i])) (*count)++;
    }
    (*count)++;
    
    char **result = malloc(sizeof(char*) * (*count));
    if (!result) {
        free(copy);
        return NULL;
    }
    
    int idx = 0;
    char *token = strtok(copy, delim);
    while (token && idx < *count) {
        result[idx++] = cli_strdup(token);
        token = strtok(NULL, delim);
    }
    
    free(copy);
    return result;
}

// ============================================================================
// Core Implementation
// ============================================================================

cli_context_t* cli_init(void) {
    cli_context_t *ctx = malloc(sizeof(cli_context_t));
    if (!ctx) return NULL;

    cli_config_t default_config = (cli_config_t){0};
    default_config.show_help_on_error = true;
    default_config.allow_abbreviations = true;
    default_config.verbosity = 1;

    ctx->config = default_config;
    ctx->commands = NULL;
    ctx->command_count = 0;
    ctx->parsed_args = NULL;
    ctx->parsed_count = 0;
    ctx->current_command = NULL;

    return ctx;
}

void cli_free(cli_context_t *ctx) {
    if (!ctx) return;
    
    for (size_t i = 0; i < ctx->parsed_count; i++) {
        free(ctx->parsed_args[i]);
    }
    free(ctx->parsed_args);
    free(ctx->commands);
    free(ctx);
}

cli_error_t cli_register_command(cli_context_t *ctx, cli_command_t cmd) {
    if (!ctx || !cmd.name) return CLI_ERR_INVALID_ARG;
    
    cli_command_t *new_commands = realloc(ctx->commands, 
        sizeof(cli_command_t) * (ctx->command_count + 1));
    if (!new_commands) return CLI_ERR_PARSE_ERROR;
    
    ctx->commands = new_commands;
    ctx->commands[ctx->command_count++] = cmd;
    
    return CLI_OK;
}

cli_error_t cli_parse(cli_context_t *ctx, int argc, char **argv) {
    if (!ctx || argc < 1) return CLI_ERR_INVALID_ARG;
    
    cli_parser_state_t state = {
        .argc = argc,
        .argv = argv,
        .current_index = 1,
        .last_error = CLI_OK
    };
    
    // Parse global options first (if any)
    while (state.current_index < state.argc) {
        const char *arg = state.argv[state.current_index];
        
        // Check if it's a command
        if (arg[0] != '-') {
            for (size_t i = 0; i < ctx->command_count; i++) {
                if (cli_str_eq(ctx->commands[i].name, arg, 
                              ctx->config.case_sensitive_options)) {
                    ctx->current_command = &ctx->commands[i];
                    state.current_index++;
                    break;
                }
            }
            if (ctx->current_command) break;
        }
        
        state.current_index++;
    }
    
    if (!ctx->current_command && ctx->command_count > 0) {
        ctx->current_command = &ctx->commands[0];
    }
    
    return CLI_OK;
}

int cli_execute(cli_context_t *ctx) {
    if (!ctx || !ctx->current_command) return 1;
    
    return ctx->current_command->execute(ctx->current_command, 
                                         0, NULL);
}

// ============================================================================
// Option Management
// ============================================================================

cli_option_t cli_option_builder(const char *long_name) {
    cli_option_t opt = {0};
    opt.long_name = long_name;
    opt.type = CLI_ARG_STRING;
    return opt;
}

cli_option_t* cli_set_short_name(cli_option_t *opt, const char *name) {
    if (opt) opt->short_name = name;
    return opt;
}

cli_option_t* cli_set_description(cli_option_t *opt, const char *desc) {
    if (opt) opt->description = desc;
    return opt;
}

cli_option_t* cli_set_type(cli_option_t *opt, cli_arg_type_t type) {
    if (opt) opt->type = type;
    return opt;
}

cli_option_t* cli_set_required(cli_option_t *opt, bool required) {
    if (opt) opt->required = required;
    return opt;
}

cli_option_t* cli_set_default(cli_option_t *opt, const char *value) {
    if (opt) opt->default_value = value;
    return opt;
}

cli_option_t* cli_set_validator(cli_option_t *opt, bool (*validator)(const char*)) {
    if (opt) opt->validator = validator;
    return opt;
}

cli_option_t* cli_set_enum_values(cli_option_t *opt, const char **values, int count) {
    if (opt) {
        opt->enum_values = values;
        opt->enum_count = count;
        opt->type = CLI_ARG_ENUM;
    }
    return opt;
}

// ============================================================================
// Value Retrieval
// ============================================================================

const char* cli_get_string(cli_context_t *ctx, const char *option_name) {
    if (!ctx || !ctx->current_command) return NULL;
    
    for (size_t i = 0; i < ctx->current_command->option_count; i++) {
        cli_option_t *opt = &ctx->current_command->options[i];
        if (cli_str_eq(opt->long_name, option_name, true)) {
            return opt->value ? *(const char**)opt->value : opt->default_value;
        }
    }
    return NULL;
}

int cli_get_int(cli_context_t *ctx, const char *option_name) {
    const char *str = cli_get_string(ctx, option_name);
    return str ? atoi(str) : 0;
}

bool cli_get_bool(cli_context_t *ctx, const char *option_name) {
    const char *str = cli_get_string(ctx, option_name);
    return str && (strcmp(str, "true") == 0 || strcmp(str, "1") == 0);
}

double cli_get_float(cli_context_t *ctx, const char *option_name) {
    const char *str = cli_get_string(ctx, option_name);
    return str ? atof(str) : 0.0;
}

char** cli_get_multi(cli_context_t *ctx, const char *option_name, int *count) {
    const char *str = cli_get_string(ctx, option_name);
    if (!str) {
        *count = 0;
        return NULL;
    }
    return cli_split_string(str, ",", count);
}

// ============================================================================
// Validation
// ============================================================================

bool cli_validate_email(const char *value) {
    if (!value || !strchr(value, '@')) return false;
    return strchr(strchr(value, '@') + 1, '.') != NULL;
}

bool cli_validate_url(const char *value) {
    return value && (strncmp(value, "http://", 7) == 0 || 
                     strncmp(value, "https://", 8) == 0 ||
                     strncmp(value, "ftp://", 6) == 0);
}

bool cli_validate_ipv4(const char *value) {
    int a, b, c, d;
    return value && sscanf(value, "%d.%d.%d.%d", &a, &b, &c, &d) == 4 &&
           a >= 0 && a <= 255 && b >= 0 && b <= 255 &&
           c >= 0 && c <= 255 && d >= 0 && d <= 255;
}

bool cli_validate_integer(const char *value) {
    if (!value || !*value) return false;
    char *end;
    strtol(value, &end, 10);
    return *end == '\0';
}

bool cli_validate_float(const char *value) {
    if (!value || !*value) return false;
    char *end;
    strtod(value, &end);
    return *end == '\0';
}

bool cli_validate_file_exists(const char *value) {
    if (!value) return false;
    FILE *f = fopen(value, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

bool cli_validate_dir_exists(const char *value) {
    // Simplified - would need platform-specific code in production
    return value && strlen(value) > 0;
}

bool cli_validate_regex(const char *value, const char *pattern) {
    // Simplified - would use regex library in production
    (void)pattern;
    return value != NULL;
}

// ============================================================================
// Help and Documentation
// ============================================================================

char* cli_generate_help(cli_context_t *ctx, cli_command_t *cmd) {
    if (!ctx || !cmd) return NULL;
    
    char *help = malloc(4096);
    if (!help) return NULL;
    
    int pos = 0;
    pos += sprintf(help + pos, "\n%s - %s\n", cmd->name, cmd->description);
    pos += sprintf(help + pos, "\nUSAGE:\n  %s [OPTIONS]\n", cmd->name);
    
    if (cmd->option_count > 0) {
        pos += sprintf(help + pos, "\nOPTIONS:\n");
        for (size_t i = 0; i < cmd->option_count; i++) {
            cli_option_t *opt = &cmd->options[i];
            if (opt->short_name) {
                pos += sprintf(help + pos, "  %s, %s", opt->short_name, opt->long_name);
            } else {
                pos += sprintf(help + pos, "  %s", opt->long_name);
            }
            pos += sprintf(help + pos, "  %s\n", opt->description);
        }
    }
    
    return help;
}

void cli_print_help(cli_context_t *ctx, cli_command_t *cmd) {
    char *help = cli_generate_help(ctx, cmd);
    if (help) {
        printf("%s", help);
        free(help);
    }
}

void cli_print_usage(cli_context_t *ctx, cli_command_t *cmd) {
    (void)ctx;
    if (cmd) {
        printf("Usage: %s [OPTIONS]\n", cmd->name);
    }
}

void cli_print_version(cli_context_t *ctx) {
    if (ctx) {
        printf("%s version %s\n", ctx->config.app_name, ctx->config.version);
    }
}

// ============================================================================
// Error Handling
// ============================================================================

const char* cli_get_error_message(cli_context_t *ctx) {
    return ctx ? "Error occurred" : NULL;
}

char* cli_format_error(cli_error_t error, const char *detail) {
    char *msg = malloc(512);
    if (!msg) return NULL;
    
    const char *error_text = "";
    switch (error) {
        case CLI_OK: error_text = "OK"; break;
        case CLI_ERR_INVALID_ARG: error_text = "Invalid argument"; break;
        case CLI_ERR_MISSING_ARG: error_text = "Missing argument"; break;
        case CLI_ERR_UNKNOWN_OPTION: error_text = "Unknown option"; break;
        case CLI_ERR_TYPE_MISMATCH: error_text = "Type mismatch"; break;
        case CLI_ERR_FILE_NOT_FOUND: error_text = "File not found"; break;
        case CLI_ERR_INVALID_ENUM: error_text = "Invalid choice"; break;
        case CLI_ERR_RANGE_ERROR: error_text = "Value out of range"; break;
        case CLI_ERR_PARSE_ERROR: error_text = "Parse error"; break;
        case CLI_ERR_USAGE: error_text = "Usage error"; break;
    }
    
    if (detail) {
        snprintf(msg, 512, "Error: %s - %s", error_text, detail);
    } else {
        snprintf(msg, 512, "Error: %s", error_text);
    }
    
    return msg;
}

// ============================================================================
// Logging
// ============================================================================

void cli_log_debug(cli_context_t *ctx, const char *fmt, ...) {
    if (!ctx || ctx->config.verbosity < 3) return;
    printf("[DEBUG] ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void cli_log_info(cli_context_t *ctx, const char *fmt, ...) {
    if (!ctx || ctx->config.verbosity < 2) return;
    printf("[INFO] ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void cli_log_warning(cli_context_t *ctx, const char *fmt, ...) {
    if (!ctx || ctx->config.verbosity < 1) return;
    printf("[WARN] ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void cli_log_error(cli_context_t *ctx, const char *fmt, ...) {
    (void)ctx;
    printf("[ERROR] ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

// ============================================================================
// Utilities
// ============================================================================

void cli_print_colored(cli_color_t color, const char *fmt, ...) {
    const char *color_code = "";
    switch (color) {
        case CLI_COLOR_RED: color_code = "\033[31m"; break;
        case CLI_COLOR_GREEN: color_code = "\033[32m"; break;
        case CLI_COLOR_YELLOW: color_code = "\033[33m"; break;
        case CLI_COLOR_BLUE: color_code = "\033[34m"; break;
        case CLI_COLOR_CYAN: color_code = "\033[36m"; break;
        case CLI_COLOR_RESET: color_code = "\033[0m"; break;
    }
    
    printf("%s", color_code);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\033[0m");
}

void cli_progress_start(const char *label, size_t total) {
    (void)total;
    printf("%s: [", label);
    fflush(stdout);
}

void cli_progress_update(size_t current) {
    (void)current;
    printf(".");
    fflush(stdout);
}

void cli_progress_finish(void) {
    printf("] Done\n");
}

bool cli_confirm(const char *prompt) {
    printf("%s [y/N] ", prompt);
    fflush(stdout);
    
    int c = getchar();
    while (getchar() != '\n');  // Clear input buffer
    
    return c == 'y' || c == 'Y';
}

char* cli_prompt(const char *prompt, bool hidden) {
    (void)hidden;
    printf("%s", prompt);
    fflush(stdout);
    
    char *buffer = malloc(256);
    if (!buffer) return NULL;
    
    if (fgets(buffer, 256, stdin)) {
        cli_trim(buffer);
        return buffer;
    }
    
    free(buffer);
    return NULL;
}
