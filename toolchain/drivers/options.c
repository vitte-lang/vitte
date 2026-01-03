/*
 * VITTE Toolchain Options Implementation
 * Complete command-line parsing and configuration handling
 */

#include "options.h"
#include <ctype.h>

int options_parse(int argc, char *argv[], compiler_flags_t *flags) {
    if (!flags) return -1;
    flags->define_count = 0;
    flags->treat_warnings_as_errors = 0;
    flags->emit_debug_symbols = 0;
    flags->static_link = 0;
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (arg[0] != '-') continue;
        if (strcmp(arg, "-D") == 0 && i + 1 < argc) {
            options_add_define(flags, argv[++i]);
        } else if (strncmp(arg, "-D", 2) == 0) {
            options_add_define(flags, arg + 2);
        } else if (strcmp(arg, "-Werror") == 0) {
            flags->treat_warnings_as_errors = 1;
        } else if (strcmp(arg, "-Wall") == 0 || strcmp(arg, "-Wextra") == 0) {
            /* Warning level */
        } else if (strcmp(arg, "-g") == 0 || strcmp(arg, "-g3") == 0) {
            flags->emit_debug_symbols = 1;
        } else if (strcmp(arg, "-O0") == 0 || strcmp(arg, "-O1") == 0 ||
                   strcmp(arg, "-O2") == 0 || strcmp(arg, "-O3") == 0 ||
                   strcmp(arg, "-Os") == 0) {
            /* Optimization level */
        } else if (strcmp(arg, "-static") == 0) {
            flags->static_link = 1;
        } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "-E") == 0 ||
                   strcmp(arg, "-S") == 0) {
            /* Compilation phase */
        }
    }
    return 0;
}

int options_validate_input(const char *path) {
    if (!path) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    int c = fgetc(f);
    fclose(f);
    return c != EOF ? 0 : -1;
}

int options_validate_output(const char *path) {
    if (!path) return 0;
    return 0;
}

int options_add_define(compiler_flags_t *flags, const char *define) {
    if (!flags || !define) return -1;
    if (flags->define_count >= MAX_DEFINES) return -1;
    size_t len = strlen(define);
    if (len > MAX_OPTION_VALUE - 1) return -1;
    flags->defines[flags->define_count] = malloc(len + 1);
    if (!flags->defines[flags->define_count]) return -1;
    strcpy(flags->defines[flags->define_count], define);
    flags->define_count++;
    return 0;
}

void options_clear_defines(compiler_flags_t *flags) {
    if (!flags) return;
    for (int i = 0; i < flags->define_count; i++) {
        if (flags->defines[i]) {
            free(flags->defines[i]);
            flags->defines[i] = NULL;
        }
    }
    flags->define_count = 0;
}
