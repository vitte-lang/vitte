// Vitte Compiler - CLI Implementation
// Enterprise-grade command line interface

#include "cli.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#define DEFAULT_PROFILE "dev"
#define DEFAULT_PRESET_PATH ".vittec/config"

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

static const char* cli_canonical_name(const char *name) {
    if (!name) return NULL;
    while (*name == '-') name++;
    return name;
}

static bool cli_option_name_matches(const char *candidate, const char *query,
                                    bool case_sensitive) {
    return cli_str_eq(cli_canonical_name(candidate),
                      cli_canonical_name(query),
                      case_sensitive);
}

static cli_option_t* cli_find_option(cli_command_t *cmd, const char *name,
                                     bool case_sensitive) {
    if (!cmd || !name) return NULL;
    for (size_t i = 0; i < cmd->option_count; i++) {
        cli_option_t *opt = &cmd->options[i];
        if ((opt->long_name && cli_option_name_matches(opt->long_name, name, case_sensitive)) ||
            (opt->short_name && cli_option_name_matches(opt->short_name, name, case_sensitive))) {
            return opt;
        }
    }
    return NULL;
}

static void cli_clear_option_values(cli_command_t *cmd) {
    if (!cmd || !cmd->options) return;
    for (size_t i = 0; i < cmd->option_count; i++) {
        if (cmd->options[i].value) {
            *(const char**)cmd->options[i].value = NULL;
        }
    }
}

static void cli_free_parsed_args(cli_context_t *ctx) {
    if (!ctx || !ctx->parsed_args) return;
    for (size_t i = 0; i < ctx->parsed_count; i++) {
        free(ctx->parsed_args[i]);
    }
    free(ctx->parsed_args);
    ctx->parsed_args = NULL;
    ctx->parsed_count = 0;
}

static bool cli_add_positional(cli_context_t *ctx, const char *arg) {
    if (!ctx || !arg) return false;
    char *dup = cli_strdup(arg);
    if (!dup) return false;
    char **new_args = realloc(ctx->parsed_args, sizeof(char*) * (ctx->parsed_count + 1));
    if (!new_args) {
        free(dup);
        return false;
    }
    ctx->parsed_args = new_args;
    ctx->parsed_args[ctx->parsed_count++] = dup;
    return true;
}

static cli_error_t cli_validate_and_set_option(cli_context_t *ctx,
                                               cli_option_t *opt,
                                               const char *value,
                                               const char *arg_name) {
    if (!opt) return CLI_ERR_INVALID_ARG;

    const char *resolved_value = value;
    if ((opt->type == CLI_ARG_BOOL || opt->type == CLI_ARG_NONE) && !resolved_value) {
        resolved_value = "true";
    }

    if (!resolved_value && opt->type != CLI_ARG_BOOL && opt->type != CLI_ARG_NONE) {
        cli_log_error(ctx, "Missing value for option %s", arg_name ? arg_name : "(unknown)");
        return CLI_ERR_MISSING_ARG;
    }

    switch (opt->type) {
        case CLI_ARG_INT: {
            if (!cli_validate_integer(resolved_value)) {
                cli_log_error(ctx, "Expected integer for option %s", arg_name ? arg_name : "(unknown)");
                return CLI_ERR_TYPE_MISMATCH;
            }
            int parsed = atoi(resolved_value);
            if ((opt->min_value || opt->max_value) &&
                (parsed < opt->min_value || parsed > opt->max_value)) {
                cli_log_error(ctx, "Value for %s must be between %d and %d",
                              arg_name ? arg_name : "(unknown)", opt->min_value, opt->max_value);
                return CLI_ERR_RANGE_ERROR;
            }
            break;
        }
        case CLI_ARG_FLOAT:
            if (!cli_validate_float(resolved_value)) {
                cli_log_error(ctx, "Expected float for option %s", arg_name ? arg_name : "(unknown)");
                return CLI_ERR_TYPE_MISMATCH;
            }
            break;
        case CLI_ARG_ENUM: {
            bool match = false;
            for (int i = 0; i < opt->enum_count; i++) {
                if (cli_str_eq(opt->enum_values[i], resolved_value,
                               ctx ? ctx->config.case_sensitive_options : false)) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                cli_log_error(ctx, "Invalid value '%s' for option %s", resolved_value,
                              arg_name ? arg_name : "(unknown)");
                return CLI_ERR_INVALID_ENUM;
            }
            break;
        }
        case CLI_ARG_PATH:
        case CLI_ARG_STRING:
        case CLI_ARG_MULTI:
        case CLI_ARG_LIST:
        case CLI_ARG_BOOL:
        case CLI_ARG_NONE:
            break;
    }

    if (opt->validator && !opt->validator(resolved_value)) {
        cli_log_error(ctx, "Validation failed for option %s", arg_name ? arg_name : "(unknown)");
        return CLI_ERR_INVALID_ARG;
    }

    if (opt->value) {
        *(const char**)opt->value = resolved_value;
    }
    if (opt->on_change) {
        opt->on_change(resolved_value);
    }

    return CLI_OK;
}

static cli_error_t cli_validate_required_options(cli_context_t *ctx,
                                                 cli_command_t *cmd) {
    if (!ctx || !cmd) return CLI_ERR_INVALID_ARG;
    for (size_t i = 0; i < cmd->option_count; i++) {
        cli_option_t *opt = &cmd->options[i];
        const char *current = opt->value ? *(const char**)opt->value : NULL;
        if (opt->required && !current && !opt->default_value) {
            const char *name = opt->long_name ? cli_canonical_name(opt->long_name)
                                              : (opt->short_name ? cli_canonical_name(opt->short_name) : "unknown");
            cli_log_error(ctx, "Missing required option --%s", name);
            return CLI_ERR_MISSING_ARG;
        }
    }
    return CLI_OK;
}

static cli_error_t cli_parse_option(cli_context_t *ctx,
                                    cli_parser_state_t *state,
                                    cli_command_t *cmd) {
    if (!ctx || !state || !cmd) return CLI_ERR_INVALID_ARG;
    const char *arg = state->argv[state->current_index];
    bool is_long = strncmp(arg, "--", 2) == 0;
    bool is_short = !is_long && arg[0] == '-' && arg[1] != '\0';

    if (!is_long && !is_short) {
        return CLI_ERR_INVALID_ARG;
    }

    cli_error_t result = CLI_OK;
    const char *value = NULL;
    cli_option_t *opt = NULL;

    if (is_long) {
        const char *name = arg + 2;
        const char *eq = strchr(name, '=');
        char *name_copy = NULL;
        if (eq) {
            size_t len = (size_t)(eq - name);
            name_copy = malloc(len + 1);
            if (!name_copy) return CLI_ERR_PARSE_ERROR;
            memcpy(name_copy, name, len);
            name_copy[len] = '\0';
            name = name_copy;
            value = eq + 1;
        }

        opt = cli_find_option(cmd, name, ctx->config.case_sensitive_options);
        if (name_copy) free(name_copy);
        if (!opt) {
            cli_log_error(ctx, "Unknown option '%s'", arg);
            return CLI_ERR_UNKNOWN_OPTION;
        }

        if (!value && opt->type != CLI_ARG_BOOL && opt->type != CLI_ARG_NONE) {
            if (state->current_index + 1 >= state->argc) {
                cli_log_error(ctx, "Missing value for option '%s'", arg);
                return CLI_ERR_MISSING_ARG;
            }
            value = state->argv[++state->current_index];
        }

        result = cli_validate_and_set_option(ctx, opt, value, arg);
        state->current_index++;
        return result;
    }

    const char *name_part = arg + 1;
    char short_name[2] = {0};
    short_name[0] = name_part[0];
    opt = cli_find_option(cmd, short_name, ctx->config.case_sensitive_options);
    if (!opt) {
        cli_log_error(ctx, "Unknown option '%s'", arg);
        return CLI_ERR_UNKNOWN_OPTION;
    }

    if (strlen(name_part) > 1) {
        value = name_part + 1;
        if (opt->type == CLI_ARG_BOOL || opt->type == CLI_ARG_NONE) {
            cli_log_error(ctx, "Option '-%s' does not accept a value", short_name);
            return CLI_ERR_USAGE;
        }
    } else if (opt->type != CLI_ARG_BOOL && opt->type != CLI_ARG_NONE) {
        if (state->current_index + 1 >= state->argc) {
            cli_log_error(ctx, "Missing value for option '%s'", arg);
            return CLI_ERR_MISSING_ARG;
        }
        value = state->argv[++state->current_index];
    }

    result = cli_validate_and_set_option(ctx, opt, value, arg);
    state->current_index++;
    return result;
}

static bool cli_is_profile_valid(const char *profile) {
    if (!profile) return false;
    const char *valid[] = {"dev", "release", "debug-asm"};
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); i++) {
        if (strcmp(profile, valid[i]) == 0) return true;
    }
    return false;
}

static void cli_set_profile(cli_context_t *ctx, const char *profile) {
    if (!ctx || !profile) return;
    if (!cli_is_profile_valid(profile)) {
        cli_log_warning(ctx, "Unknown profile '%s', falling back to '%s'",
                        profile, DEFAULT_PROFILE);
        profile = DEFAULT_PROFILE;
    }
    free(ctx->profile);
    ctx->profile = cli_strdup(profile);
}

static bool cli_file_exists(const char *path) {
    if (!path) return false;
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

static void cli_reset_config_store(cli_context_t *ctx) {
    if (!ctx) return;
    if (ctx->config_store) {
        vitte_config_free(ctx->config_store);
    }
    ctx->config_store = vitte_config_create();
    if (ctx->config_store) {
        vitte_config_set_defaults(ctx->config_store);
    }
}

static void cli_print_preset_summary(cli_context_t *ctx) {
    if (!ctx || !ctx->config_store) return;
    size_t count = ctx->config_store->count;
    size_t limit = count < 8 ? count : 8;

    printf("Preset loaded from %s (%zu entr%s):\n",
           ctx->preset_path ? ctx->preset_path : DEFAULT_PRESET_PATH,
           count,
           count == 1 ? "y" : "ies");

    for (size_t i = 0; i < limit; i++) {
        vitte_config_entry_t *entry = &ctx->config_store->entries[i];
        printf("  %s = ", entry->key);
        switch (entry->type) {
            case VITTE_CONFIG_STRING:
            case VITTE_CONFIG_PATH:
                printf("%s", (char*)entry->value);
                break;
            case VITTE_CONFIG_INT:
                printf("%d", *(int*)entry->value);
                break;
            case VITTE_CONFIG_BOOL:
                printf("%s", *(int*)entry->value ? "true" : "false");
                break;
            case VITTE_CONFIG_FLOAT:
                printf("%f", *(float*)entry->value);
                break;
        }
        printf("\n");
    }

    if (count > limit) {
        printf("  ... %zu more entr%s\n", count - limit, (count - limit) == 1 ? "y" : "ies");
    }
}

static bool cli_load_preset(cli_context_t *ctx, const char *path, bool quiet) {
    if (!ctx || !path) return false;
    
    ctx->preset_loaded = false;
    free(ctx->preset_path);
    ctx->preset_path = NULL;

    if (!cli_file_exists(path)) {
        if (!quiet) {
            cli_log_warning(ctx, "Preset file not found: %s", path);
        }
        return false;
    }
    
    cli_reset_config_store(ctx);
    if (!ctx->config_store) {
        return false;
    }

    int load_result = vitte_config_load_file(ctx->config_store, path);
    if (load_result != 0) {
        cli_log_error(ctx, "Failed to load preset '%s'%s",
                      path, load_result == -2 ? " (unknown key)" : "");
        return false;
    }

    ctx->preset_path = cli_strdup(path);
    ctx->preset_loaded = true;
    if (!quiet) {
        cli_log_info(ctx, "Loaded preset from %s", path);
    }
    cli_print_preset_summary(ctx);
    return true;
}

static cli_error_t cli_try_load_default_preset(cli_context_t *ctx) {
    if (!ctx || ctx->preset_loaded) return CLI_OK;
    if (cli_file_exists(DEFAULT_PRESET_PATH)) {
        return cli_load_preset(ctx, DEFAULT_PRESET_PATH, true) ? CLI_OK : CLI_ERR_PARSE_ERROR;
    }
    return CLI_OK;
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
    ctx->profile = cli_strdup(DEFAULT_PROFILE);
    ctx->preset_path = NULL;
    ctx->preset_loaded = false;
    ctx->config_store = vitte_config_create();
    if (ctx->config_store) {
        vitte_config_set_defaults(ctx->config_store);
    }

    return ctx;
}

void cli_free(cli_context_t *ctx) {
    if (!ctx) return;
    
    for (size_t i = 0; i < ctx->parsed_count; i++) {
        free(ctx->parsed_args[i]);
    }
    free(ctx->parsed_args);
    free(ctx->commands);
    free(ctx->profile);
    free(ctx->preset_path);
    if (ctx->config_store) {
        vitte_config_free(ctx->config_store);
    }
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
    
    cli_free_parsed_args(ctx);
    ctx->current_command = NULL;

    cli_error_t preset_status = cli_try_load_default_preset(ctx);
    if (preset_status != CLI_OK) {
        return preset_status;
    }
    
    cli_parser_state_t state = {
        .argc = argc,
        .argv = argv,
        .current_index = 1,
        .last_error = CLI_OK
    };
    
    // Parse global options first (if any) and find the command token
    while (state.current_index < state.argc) {
        const char *arg = state.argv[state.current_index];

        if (strcmp(arg, "--profile") == 0) {
            if (state.current_index + 1 >= state.argc) {
                cli_log_error(ctx, "Missing value for --profile");
                return CLI_ERR_MISSING_ARG;
            }
            cli_set_profile(ctx, state.argv[state.current_index + 1]);
            state.current_index += 2;
            continue;
        }
        if (strncmp(arg, "--profile=", 10) == 0) {
            cli_set_profile(ctx, arg + 10);
            state.current_index++;
            continue;
        }
        if ((strcmp(arg, "--config") == 0 || strcmp(arg, "--preset") == 0)) {
            if (state.current_index + 1 >= state.argc) {
                cli_log_error(ctx, "Missing value for %s", arg);
                return CLI_ERR_MISSING_ARG;
            }
            if (!cli_load_preset(ctx, state.argv[state.current_index + 1], false)) {
                return CLI_ERR_PARSE_ERROR;
            }
            state.current_index += 2;
            continue;
        }
        if (strncmp(arg, "--config=", 9) == 0) {
            if (!cli_load_preset(ctx, arg + 9, false)) {
                return CLI_ERR_PARSE_ERROR;
            }
            state.current_index++;
            continue;
        }
        
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
        if (arg[0] == '-') {
            cli_log_error(ctx, "Unknown global option '%s'", arg);
            return CLI_ERR_UNKNOWN_OPTION;
        }
        
        state.current_index++;
    }
    
    if (!ctx->current_command && ctx->command_count > 0) {
        ctx->current_command = &ctx->commands[0];
    }
    if (!ctx->current_command) {
        cli_log_error(ctx, "No command specified");
        return CLI_ERR_USAGE;
    }

    cli_clear_option_values(ctx->current_command);

    // Parse command-specific options and positional arguments
    while (state.current_index < state.argc) {
        const char *arg = state.argv[state.current_index];
        if (strcmp(arg, "--") == 0) {
            state.current_index++;
            while (state.current_index < state.argc) {
                if (!cli_add_positional(ctx, state.argv[state.current_index++])) {
                    return CLI_ERR_PARSE_ERROR;
                }
            }
            break;
        }

        if (arg[0] == '-' && arg[1] != '\0') {
            cli_error_t opt_err = cli_parse_option(ctx, &state, ctx->current_command);
            if (opt_err != CLI_OK) {
                return opt_err;
            }
            continue;
        }

        if (!cli_add_positional(ctx, arg)) {
            return CLI_ERR_PARSE_ERROR;
        }
        state.current_index++;
    }

    cli_error_t required_err = cli_validate_required_options(ctx, ctx->current_command);
    if (required_err != CLI_OK) {
        return required_err;
    }

    return CLI_OK;
}

int cli_execute(cli_context_t *ctx) {
    if (!ctx || !ctx->current_command) return 1;
    
    return ctx->current_command->execute(ctx, ctx->current_command, 
                                         (int)ctx->parsed_count, ctx->parsed_args);
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
        if (cli_option_name_matches(opt->long_name, option_name, ctx->config.case_sensitive_options) ||
            (opt->short_name && cli_option_name_matches(opt->short_name, option_name, ctx->config.case_sensitive_options))) {
            const char *stored = opt->value ? *(const char**)opt->value : NULL;
            return stored ? stored : opt->default_value;
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
        if (count) *count = 0;
        return NULL;
    }
    if (!count) {
        int tmp = 0;
        return cli_split_string(str, ",", &tmp);
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
