// Vitte Compiler - CLI Usage Examples
// Demonstrates how to use the CLI framework programmatically

#include <stdio.h>
#include <stdlib.h>
#include "cli.h"
#include "cli_commands.h"
#include "config.h"

// ============================================================================
// Example 1: Basic Usage
// ============================================================================

void example_basic_usage(void) {
    printf("=== Example 1: Basic Usage ===\n\n");
    
    // Create CLI context
    cli_context_t *ctx = cli_init();
    if (!ctx) {
        fprintf(stderr, "Failed to create CLI context\n");
        return;
    }
    
    // Register commands
    setup_all_commands(ctx);
    
    // Simulate: vittec compile main.vitte
    const char *argv[] = { "vittec", "compile", "main.vitte" };
    int argc = 3;
    
    // Parse arguments
    if (cli_parse(ctx, argc, (char**)argv) == 0) {
        // Execute command
        int result = cli_execute(ctx);
        printf("\nCommand result: %d\n", result);
    }
    
    cli_free(ctx);
}

// ============================================================================
// Example 2: Advanced Option Handling
// ============================================================================

void example_advanced_options(void) {
    printf("\n=== Example 2: Advanced Options ===\n\n");
    
    cli_context_t *ctx = cli_init();
    if (!ctx) return;
    
    // Register commands
    setup_all_commands(ctx);
    
    // Simulate: vittec compile -O3 -g --emit-ir main.vitte
    const char *argv[] = {
        "vittec",
        "compile",
        "-O3",
        "-g",
        "--emit-ir",
        "main.vitte"
    };
    int argc = 6;
    
    if (cli_parse(ctx, argc, (char**)argv) == 0) {
        printf("Parsed command: %s\n", ctx->current_command ? ctx->current_command->name : "(none)");
        printf("CLI option parsing can be wired to real compiler flags here.\n");

        int result = cli_execute(ctx);
        printf("Execution result: %d\n", result);
    }
    
    cli_free(ctx);
}

// ============================================================================
// Example 3: Configuration Management
// ============================================================================

void example_configuration(void) {
    printf("\n=== Example 3: Configuration Management ===\n\n");
    
    // Create configuration
    vitte_config_t *config = vitte_config_create();
    if (!config) {
        fprintf(stderr, "Failed to create config\n");
        return;
    }
    
    // Set default values
    vitte_config_set_defaults(config);
    
    // Override with custom values
    int opt_level = 3;
    vitte_config_set(config, "compile.optimization", &opt_level, VITTE_CONFIG_INT);
    
    const char *cache_dir = ".vitte/cache";
    vitte_config_set(config, "cache.directory", cache_dir, VITTE_CONFIG_PATH);
    
    // Load from environment
    vitte_config_load_env(config);
    
    // Print configuration
    printf("Current Configuration:\n");
    vitte_config_print(config);
    
    // Save to file
    if (vitte_config_save(config, ".vitteconfig") == 0) {
        printf("\nConfiguration saved to .vitteconfig\n");
    }
    
    vitte_config_free(config);
}

// ============================================================================
// Example 4: Error Handling
// ============================================================================

void example_error_handling(void) {
    printf("\n=== Example 4: Error Handling ===\n\n");
    
    cli_context_t *ctx = cli_init();
    if (!ctx) return;
    
    // Simulate invalid command
    const char *argv[] = { "vittec", "invalid_command" };
    int argc = 2;
    
    if (cli_parse(ctx, argc, (char**)argv) != 0) {
        printf("Parsing failed - this is expected for invalid commands\n");
    }
    
    cli_free(ctx);
}

// ============================================================================
// Example 5: Logging
// ============================================================================

void example_logging(void) {
    printf("\n=== Example 5: Logging ===\n\n");
    
    cli_context_t *ctx = cli_init();
    if (!ctx) return;
    
    // Set verbosity
    ctx->config.verbosity = 3;
    
    // Different log levels
    cli_log_debug(ctx, "This is a debug message (only shown in verbose mode)");
    cli_log_info(ctx, "This is an info message");
    cli_log_warning(ctx, "This is a warning message");
    cli_log_error(ctx, "This is an error message");
    
    cli_free(ctx);
}

// ============================================================================
// Example 6: Interactive Features
// ============================================================================

void example_interactive(void) {
    printf("\n=== Example 6: Interactive Features ===\n\n");
    
    // Color output example
    printf("Colored output:\n");
    cli_print_colored(CLI_COLOR_GREEN, "  Success: Operation completed");
    printf("\n");
    cli_print_colored(CLI_COLOR_RED, "  Error: Something went wrong");
    printf("\n");
    cli_print_colored(CLI_COLOR_YELLOW, "  Warning: Check this carefully");
    printf("\n\n");
    
    // Progress bar example
    printf("Progress bar:\n");
    cli_progress_start("Processing", 20);
    for (int i = 0; i < 20; i++) {
        cli_progress_update(i);
    }
    cli_progress_finish();
    printf("\n");
}

// ============================================================================
// Example 7: Custom Command
// ============================================================================

int custom_cmd_example(cli_context_t *ctx, cli_command_t *cmd, int argc, char **argv) {
    (void)cmd;
    printf("Custom command executed (profile: %s)!\n", ctx && ctx->profile ? ctx->profile : "dev");

    printf("Arguments: ");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}

void example_custom_command(void) {
    printf("\n=== Example 7: Custom Command ===\n\n");
    
    cli_context_t *ctx = cli_init();
    if (!ctx) return;
    
    // Create and register custom command
    cli_command_t custom = {
        .name = "custom",
        .description = "Custom example command",
        .long_description = "This is a custom command example",
        .execute = custom_cmd_example,
        .subcommands = NULL,
        .subcommand_count = 0
    };
    
    cli_register_command(ctx, custom);
    
    // Execute custom command
    const char *argv[] = { "vittec", "custom", "arg1", "arg2" };
    int argc = 4;
    
    if (cli_parse(ctx, argc, (char**)argv) == 0) {
        cli_execute(ctx);
    }
    
    cli_free(ctx);
}

// ============================================================================
// Example 8: Pipeline Simulation
// ============================================================================

void example_pipeline(void) {
    printf("\n=== Example 8: Build Pipeline Simulation ===\n\n");
    
    cli_context_t *ctx = cli_init();
    if (!ctx) return;
    
    setup_all_commands(ctx);
    
    // Simulate: vittec build --release
    const char *argv[] = { "vittec", "build", "--release" };
    int argc = 3;
    
    printf("Simulating build pipeline...\n\n");
    
    if (cli_parse(ctx, argc, (char**)argv) == 0) {
        cli_execute(ctx);
    }
    
    printf("\n✓ Pipeline complete\n");
    
    cli_free(ctx);
}

// ============================================================================
// Main Function
// ============================================================================

int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║   Vitte Compiler CLI - Usage Examples                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Run all examples
    example_basic_usage();
    example_advanced_options();
    example_configuration();
    example_error_handling();
    example_logging();
    example_interactive();
    example_custom_command();
    example_pipeline();
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║   All examples completed!                                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

// ============================================================================
// Compilation Instructions:
//
// gcc -std=c17 -Wall -Wextra \
//     cli.c cli_commands.c config.c cli_examples.c \
//     -o cli_examples
//
// ./cli_examples
// ============================================================================
