// Vitte Compiler - Main Entry Point
// Enterprise command-line interface

#include "cli.h"
#include "cli_commands.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define VITTE_VERSION "1.0.0"
#define VITTE_BUILD_DATE "2025-01-15"

// ============================================================================
// Main Application
// ============================================================================

int main(int argc, char *argv[]) {
    // Initialize CLI context
    cli_context_t *ctx = cli_init();
    if (!ctx) {
        fprintf(stderr, "Failed to initialize CLI context\n");
        return 1;
    }
    
    // Configure CLI
    ctx->config.app_name = "vittec";
    ctx->config.version = VITTE_VERSION;
    ctx->config.description = "Vitte Compiler - Enterprise Programming Language";
    ctx->config.author = "Vitte Project";
    ctx->config.verbose = false;
    
    // Register all commands
    setup_all_commands(ctx);
    
    // Parse command line arguments
    int result = cli_parse(ctx, argc, argv);
    if (result != 0) {
        cli_log_error(ctx, "Failed to parse arguments");
        cli_free(ctx);
        return 1;
    }
    
    // Execute command
    result = cli_execute(ctx);
    
    // Cleanup
    cli_free(ctx);
    
    return result;
}

// ============================================================================
// Alternative Entry Point for Library Use
// ============================================================================

cli_context_t* vitte_cli_create(void) {
    cli_context_t *ctx = cli_init();
    if (ctx) {
        ctx->config.app_name = "vittec";
        ctx->config.version = VITTE_VERSION;
        setup_all_commands(ctx);
    }
    return ctx;
}

int vitte_cli_run(cli_context_t *ctx, int argc, char *argv[]) {
    if (!ctx) return 1;
    
    int result = cli_parse(ctx, argc, argv);
    if (result != 0) return 1;
    
    return cli_execute(ctx);
}

void vitte_cli_destroy(cli_context_t *ctx) {
    if (ctx) cli_free(ctx);
}
