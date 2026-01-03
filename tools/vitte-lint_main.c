#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * vitte-lint - Static Code Analyzer for Vitte Language
 * 
 * Detects potential bugs, style issues, and suspicious patterns.
 */

typedef struct {
    int strict_mode;
    int fix_mode;
    int verbose;
    int warnings_count;
    int errors_count;
} lint_options_t;

typedef struct {
    int line;
    const char *type;
    const char *message;
    int severity; // 0=info, 1=warning, 2=error
} lint_issue_t;

const char *severity_name(int sev) {
    switch(sev) {
        case 0: return "INFO";
        case 1: return "WARNING";
        case 2: return "ERROR";
        default: return "UNKNOWN";
    }
}

const char *severity_color(int sev) {
    switch(sev) {
        case 0: return "\x1b[36m"; // cyan
        case 1: return "\x1b[33m"; // yellow
        case 2: return "\x1b[31m"; // red
        default: return "\x1b[0m";
    }
}

void print_issue(const char *filename, lint_issue_t issue) {
    const char *color = severity_color(issue.severity);
    const char *reset = "\x1b[0m";
    
    printf("%s%s%s: %s:%d: %s: %s\n",
           color, severity_name(issue.severity), reset,
           filename, issue.line, issue.type, issue.message);
}

int lint_file(const char *filename, lint_options_t opts) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }
    
    printf("Analyzing: %s\n", filename);
    
    char line[512];
    int line_num = 0;
    int issues_found = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') len--;
        
        // Check 1: Unused variables (simple heuristic)
        if (strstr(line, "let ") && !strstr(line, " = ") && opts.strict_mode) {
            lint_issue_t issue = {
                .line = line_num,
                .type = "unused-variable",
                .message = "Variable may be unused",
                .severity = 1
            };
            print_issue(filename, issue);
            issues_found++;
            opts.warnings_count++;
        }
        
        // Check 2: Missing semicolons
        if (len > 0 && line[len-1] != ';' && line[len-1] != '{' && 
            line[len-1] != '}' && line[len-1] != ':') {
            if (strstr(line, "let ") || strstr(line, "fn ") || 
                strstr(line, "return ")) {
                lint_issue_t issue = {
                    .line = line_num,
                    .type = "missing-semicolon",
                    .message = "Statement may be missing semicolon",
                    .severity = opts.strict_mode ? 2 : 1
                };
                print_issue(filename, issue);
                issues_found++;
                if (opts.strict_mode) opts.errors_count++;
                else opts.warnings_count++;
            }
        }
        
        // Check 3: Inconsistent spacing
        if (strstr(line, "fn  ") || strstr(line, "let  ")) {
            lint_issue_t issue = {
                .line = line_num,
                .type = "spacing",
                .message = "Multiple spaces in keyword",
                .severity = 0
            };
            if (opts.verbose) print_issue(filename, issue);
        }
        
        // Check 4: Line too long
        if (len > 100) {
            lint_issue_t issue = {
                .line = line_num,
                .type = "line-too-long",
                .message = "Line exceeds 100 characters",
                .severity = 0
            };
            if (opts.verbose) print_issue(filename, issue);
        }
    }
    
    fclose(fp);
    printf("Found %d issues\n\n", issues_found);
    return issues_found;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: vitte-lint [OPTIONS] <file.vitte> [files...]\n");
        fprintf(stderr, "Try 'vitte-lint --help' for more information.\n");
        return 1;
    }
    
    lint_options_t opts = {
        .strict_mode = 0,
        .fix_mode = 0,
        .verbose = 0,
        .warnings_count = 0,
        .errors_count = 0
    };
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--strict") == 0) {
            opts.strict_mode = 1;
        } else if (strcmp(argv[i], "--fix") == 0) {
            opts.fix_mode = 1;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            opts.verbose = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: vitte-lint [OPTIONS] <file.vitte> [files...]\n\n");
            printf("Options:\n");
            printf("  --strict       Enable strict checking\n");
            printf("  --fix          Attempt to auto-fix issues\n");
            printf("  --verbose, -v  Verbose output\n");
            printf("  --help         Show this help message\n");
            return 0;
        }
    }
    
    printf("Vitte Static Code Analyzer\n");
    printf("Mode: %s\n\n", opts.strict_mode ? "STRICT" : "NORMAL");
    
    // Process files
    int total_files = 0;
    int total_issues = 0;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            int issues = lint_file(argv[i], opts);
            if (issues >= 0) {
                total_files++;
                total_issues += issues;
            }
        }
    }
    
    // Summary
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Lint Results\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Files analyzed:     %d\n", total_files);
    printf("Total issues:       %d\n", total_issues);
    printf("Errors:             %d\n", opts.errors_count);
    printf("Warnings:           %d\n", opts.warnings_count);
    printf("═══════════════════════════════════════════════════════════\n");
    
    return opts.errors_count > 0 ? 1 : 0;
}
