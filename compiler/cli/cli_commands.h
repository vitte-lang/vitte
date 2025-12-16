// Vitte Compiler - Main CLI Commands
// Enterprise commands for the Vitte compiler

#ifndef VITTE_CLI_COMMANDS_H
#define VITTE_CLI_COMMANDS_H

#include "cli.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Command Definitions
// ============================================================================

typedef struct {
    const char *input_file;
    const char *output_file;
    const char *target;
    int optimization_level;
    bool emit_ir;
    bool emit_asm;
    bool emit_c;
    bool emit_ast;
    int verbosity;
    bool debug_symbols;
    bool warnings_as_errors;
} compile_options_t;

typedef struct {
    const char *test_pattern;
    bool run_unit_tests;
    bool run_integration_tests;
    bool generate_coverage;
    int timeout_seconds;
    bool verbose_output;
} test_options_t;

typedef struct {
    const char *input_file;
    bool show_metrics;
    bool show_dependencies;
    bool show_symbols;
} analyze_options_t;

// ============================================================================
// Command Executors
// ============================================================================

int cmd_compile(cli_command_t *cmd, int argc, char **argv);
int cmd_test(cli_command_t *cmd, int argc, char **argv);
int cmd_analyze(cli_command_t *cmd, int argc, char **argv);
int cmd_format(cli_command_t *cmd, int argc, char **argv);
int cmd_lint(cli_command_t *cmd, int argc, char **argv);
int cmd_build(cli_command_t *cmd, int argc, char **argv);
int cmd_install(cli_command_t *cmd, int argc, char **argv);
int cmd_clean(cli_command_t *cmd, int argc, char **argv);
int cmd_doc(cli_command_t *cmd, int argc, char **argv);
int cmd_version(cli_command_t *cmd, int argc, char **argv);
int cmd_help(cli_command_t *cmd, int argc, char **argv);

// ============================================================================
// Command Setup Functions
// ============================================================================

cli_command_t cli_setup_compile_command(void);
cli_command_t cli_setup_test_command(void);
cli_command_t cli_setup_analyze_command(void);
cli_command_t cli_setup_format_command(void);
cli_command_t cli_setup_lint_command(void);
cli_command_t cli_setup_build_command(void);
cli_command_t cli_setup_install_command(void);
cli_command_t cli_setup_clean_command(void);
cli_command_t cli_setup_doc_command(void);
cli_command_t cli_setup_version_command(void);
cli_command_t cli_setup_help_command(void);

// ============================================================================
// Utilities
// ============================================================================

void setup_all_commands(cli_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif  // VITTE_CLI_COMMANDS_H
