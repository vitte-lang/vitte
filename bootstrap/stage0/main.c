#include "vitte_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.vit> [output.asm]\n", argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *output_file = argc > 2 ? argv[2] : "output.asm";
    
    time_t start = time(NULL);
    printf("[compiler] Vitte Bootstrap Compiler v0.1\n");
    printf("[compiler] Compiling: %s\n", input_file);
    printf("[compiler] Output: %s\n", output_file);
    
    compiler_context_t *ctx = compiler_create(input_file, output_file);
    if (!ctx) {
        fprintf(stderr, "[error] Failed to create compiler context\n");
        return 1;
    }
    
    int result = compiler_compile(ctx);
    
    time_t end = time(NULL);
    double elapsed = difftime(end, start);
    
    printf("[compiler] Compilation %s\n", result == 0 ? "succeeded" : "failed");
    printf("[compiler] Errors: %d\n", ctx->error_count);
    printf("[compiler] Warnings: %d\n", ctx->warning_count);
    printf("[compiler] Time: %.2fs\n", elapsed);
    
    compiler_free(ctx);
    return result;
}
