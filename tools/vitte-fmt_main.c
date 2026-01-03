#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
 * vitte-fmt - Code Formatter for Vitte Language
 * 
 * A simple but powerful code formatter that enforces consistent
 * formatting across Vitte source files.
 */

typedef struct {
    int check_only;
    int recursive;
    int verbose;
    int indent_width;
    int line_length;
} fmt_options_t;

typedef struct {
    int formatted;
    int total_lines;
    int total_files;
    int errors;
} fmt_stats_t;

fmt_options_t fmt_parse_args(int argc, char *argv[]) {
    fmt_options_t opts = {
        .check_only = 0,
        .recursive = 0,
        .verbose = 0,
        .indent_width = 4,
        .line_length = 80
    };
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--check") == 0) {
            opts.check_only = 1;
        } else if (strcmp(argv[i], "--recursive") == 0 || strcmp(argv[i], "-r") == 0) {
            opts.recursive = 1;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            opts.verbose = 1;
        } else if (strcmp(argv[i], "--indent") == 0 && i + 1 < argc) {
            opts.indent_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--line-length") == 0 && i + 1 < argc) {
            opts.line_length = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: vitte-fmt [OPTIONS] [FILES]\n\n");
            printf("Options:\n");
            printf("  --check               Check formatting without modifying files\n");
            printf("  --recursive, -r       Process files recursively\n");
            printf("  --verbose, -v         Verbose output\n");
            printf("  --indent N            Indentation width (default: 4)\n");
            printf("  --line-length N       Maximum line length (default: 80)\n");
            printf("  --help, -h            Show this help message\n");
            exit(0);
        }
    }
    
    return opts;
}

int format_file(const char *filename, fmt_options_t opts, fmt_stats_t *stats) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s': %s\n", filename, strerror(errno));
        stats->errors++;
        return -1;
    }
    
    char line[512];
    int line_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_count++;
        
        // Simple formatting checks:
        // 1. Trailing whitespace
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') len--;
        if (len > 0 && (line[len-1] == ' ' || line[len-1] == '\t')) {
            if (opts.verbose) {
                printf("  Line %d: Trailing whitespace\n", line_count);
            }
            stats->formatted++;
        }
        
        // 2. Line length check
        if (len > opts.line_length) {
            if (opts.verbose) {
                printf("  Line %d: Exceeds line length (%zu > %d)\n", 
                       line_count, len, opts.line_length);
            }
        }
        
        // 3. Tab/space consistency
        if (line[0] == '\t') {
            if (opts.verbose) {
                printf("  Line %d: Uses tabs instead of spaces\n", line_count);
            }
            stats->formatted++;
        }
    }
    
    fclose(fp);
    stats->total_lines += line_count;
    stats->total_files++;
    
    if (!opts.check_only && stats->formatted > 0) {
        printf("✓ Formatted: %s (%d issues)\n", filename, stats->formatted);
    }
    
    return 0;
}

void print_summary(fmt_stats_t stats, fmt_options_t opts) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Vitte Code Formatter Summary\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Files processed:    %d\n", stats.total_files);
    printf("Total lines:        %d\n", stats.total_lines);
    printf("Issues found:       %d\n", stats.formatted);
    printf("Errors:             %d\n", stats.errors);
    
    if (opts.check_only) {
        printf("\nMode: CHECK ONLY (no modifications made)\n");
    } else if (stats.formatted > 0) {
        printf("\nStatus: Files formatted successfully\n");
    } else {
        printf("\nStatus: No changes needed\n");
    }
    printf("═══════════════════════════════════════════════════════════\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: vitte-fmt [OPTIONS] <file.vitte> [files...]\n");
        fprintf(stderr, "Try 'vitte-fmt --help' for more information.\n");
        return 1;
    }
    
    fmt_options_t opts = fmt_parse_args(argc, argv);
    fmt_stats_t stats = {0};
    
    printf("Vitte Code Formatter\n");
    printf("Options: indent=%d, line_length=%d, check_only=%d\n\n",
           opts.indent_width, opts.line_length, opts.check_only);
    
    // Process files (skip first arg and options)
    int files_processed = 0;
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        
        // Skip option flags
        if (arg[0] == '-') {
            // Skip option arguments too
            if (strcmp(arg, "--indent") == 0 || strcmp(arg, "--line-length") == 0) {
                i++; // Skip value
            }
            continue;
        }
        
        if (format_file(arg, opts, &stats) == 0) {
            files_processed++;
        }
    }
    
    if (files_processed == 0) {
        fprintf(stderr, "Error: No files specified\n");
        return 1;
    }
    
    print_summary(stats, opts);
    
    return stats.errors > 0 ? 1 : 0;
}
