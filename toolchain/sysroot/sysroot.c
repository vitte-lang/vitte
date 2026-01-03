/*
 * VITTE Toolchain Sysroot Implementation
 * Complete system root configuration and path management
 */

#include "sysroot.h"

sysroot_t *sysroot_create(const char *root) {
    sysroot_t *sysroot = malloc(sizeof(sysroot_t));
    if (!sysroot) return NULL;
    sysroot->root_path = root ? strdup(root) : strdup("/usr");
    if (!sysroot->root_path) {
        free(sysroot);
        return NULL;
    }
    sysroot->include_count = 0;
    sysroot->lib_count = 0;
    sysroot->target_triple = "x86_64-unknown-linux-gnu";
    memset(sysroot->include_paths, 0, sizeof(sysroot->include_paths));
    memset(sysroot->lib_paths, 0, sizeof(sysroot->lib_paths));
    fprintf(stderr, "[sysroot] ✓ Created sysroot at: %s\n", sysroot->root_path);
    return sysroot;
}

void sysroot_free(sysroot_t *sysroot) {
    if (!sysroot) return;
    if (sysroot->root_path) {
        free(sysroot->root_path);
        sysroot->root_path = NULL;
    }
    for (int i = 0; i < sysroot->include_count; i++) {
        if (sysroot->include_paths[i]) free(sysroot->include_paths[i]);
    }
    for (int i = 0; i < sysroot->lib_count; i++) {
        if (sysroot->lib_paths[i]) free(sysroot->lib_paths[i]);
    }
    free(sysroot);
}

int sysroot_add_include_path(sysroot_t *sysroot, const char *path) {
    if (!sysroot || !path) return -1;
    if (sysroot->include_count >= MAX_PATHS) return -1;
    sysroot->include_paths[sysroot->include_count] = strdup(path);
    if (!sysroot->include_paths[sysroot->include_count]) return -1;
    fprintf(stderr, "[sysroot] ✓ Added include path: %s\n", path);
    sysroot->include_count++;
    return 0;
}

int sysroot_add_lib_path(sysroot_t *sysroot, const char *path) {
    if (!sysroot || !path) return -1;
    if (sysroot->lib_count >= MAX_PATHS) return -1;
    sysroot->lib_paths[sysroot->lib_count] = strdup(path);
    if (!sysroot->lib_paths[sysroot->lib_count]) return -1;
    fprintf(stderr, "[sysroot] ✓ Added lib path: %s\n", path);
    sysroot->lib_count++;
    return 0;
}

const char **sysroot_get_include_paths(sysroot_t *sysroot, int *count) {
    if (!sysroot || !count) return NULL;
    *count = sysroot->include_count;
    return (const char **)sysroot->include_paths;
}

const char **sysroot_get_lib_paths(sysroot_t *sysroot, int *count) {
    if (!sysroot || !count) return NULL;
    *count = sysroot->lib_count;
    return (const char **)sysroot->lib_paths;
}

int sysroot_detect_default(sysroot_t *sysroot) {
    if (!sysroot) return -1;
    sysroot_add_include_path(sysroot, "/usr/include");
    sysroot_add_include_path(sysroot, "/usr/local/include");
    sysroot_add_include_path(sysroot, "/usr/include/x86_64-linux-gnu");
    sysroot_add_lib_path(sysroot, "/usr/lib");
    sysroot_add_lib_path(sysroot, "/usr/local/lib");
    sysroot_add_lib_path(sysroot, "/lib/x86_64-linux-gnu");
    fprintf(stderr, "[sysroot] ✓ Detected default paths for %s\n", sysroot->target_triple);
    fprintf(stderr, "[sysroot]   Include paths: %d, Lib paths: %d\n", sysroot->include_count, sysroot->lib_count);
    return 0;
}

int sysroot_verify(sysroot_t *sysroot) {
    if (!sysroot) return -1;
    struct stat sb;
    if (stat(sysroot->root_path, &sb) != 0) {
        fprintf(stderr, "[sysroot] ⚠ Warning: root path %s does not exist\n", sysroot->root_path);
        return -1;
    }
    if (!S_ISDIR(sb.st_mode)) {
        fprintf(stderr, "[sysroot] ✗ Error: %s is not a directory\n", sysroot->root_path);
        return -1;
    }
    fprintf(stderr, "[sysroot] ✓ Verified sysroot at %s\n", sysroot->root_path);
    return 0;
}
