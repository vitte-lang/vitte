/*
 * tests/unit/pal/t_fs.c
 *
 * Unit tests: PAL filesystem layer.
 *
 * Assumed public API (C):
 *   - #include "pal/fs.h"
 *   - pal_fs_temp_dir(out, cap)
 *   - pal_fs_mkdir_p(path)
 *   - pal_fs_write_file(path, data, len)
 *   - pal_fs_read_file(path, out_buf, cap, &out_len)
 *   - pal_fs_remove(path)
 *   - pal_fs_is_file(path)
 *   - pal_fs_is_dir(path)
 *   - pal_fs_path_join(out, cap, a, b)
 *
 * Adapt names/types if your PAL differs; test logic remains valid.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pal/fs.h"

static int g_fail = 0;

static void t_fail(const char* file, int line, const char* expr) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
    g_fail = 1;
}
#define T_ASSERT(x) do { if (!(x)) t_fail(__FILE__, __LINE__, #x); } while (0)

static void path_join2(char* out, size_t cap, const char* a, const char* b) {
    pal_fs_err e = pal_fs_path_join(out, cap, a, b);
    if (e != PAL_FS_OK) {
        fprintf(stderr, "path_join failed: %d\n", (int)e);
        g_fail = 1;
    }
    T_ASSERT(e == PAL_FS_OK);
}

static void test_temp_dir(void) {
    char tmp[512] = {0};
    pal_fs_err e = pal_fs_temp_dir(tmp, sizeof(tmp));
    T_ASSERT(e == PAL_FS_OK);
    T_ASSERT(tmp[0] != '\0');

    /* Temp dir should exist and be a directory */
    T_ASSERT(pal_fs_is_dir(tmp) == true);
}

static void test_mkdir_write_read_remove(void) {
    char tmp[512] = {0};
    T_ASSERT(pal_fs_temp_dir(tmp, sizeof(tmp)) == PAL_FS_OK);

    /* Create unique subdir */
    char dir[1024] = {0};
    path_join2(dir, sizeof(dir), tmp, "vitte_pal_fs_test");

    /* Ensure clean state */
    (void)pal_fs_remove(dir);

    pal_fs_err e = pal_fs_mkdir_p(dir);
    T_ASSERT(e == PAL_FS_OK);
    T_ASSERT(pal_fs_is_dir(dir) == true);

    /* Write file */
    char file[1024] = {0};
    path_join2(file, sizeof(file), dir, "hello.txt");

    const char* msg = "pal fs test content";
    e = pal_fs_write_file(file, msg, strlen(msg));
    T_ASSERT(e == PAL_FS_OK);
    T_ASSERT(pal_fs_is_file(file) == true);

    /* Read back */
    char buf[256] = {0};
    size_t n = 0;
    e = pal_fs_read_file(file, buf, sizeof(buf), &n);
    T_ASSERT(e == PAL_FS_OK);
    T_ASSERT(n == strlen(msg));
    T_ASSERT(memcmp(buf, msg, n) == 0);

    /* Remove file then dir */
    e = pal_fs_remove(file);
    T_ASSERT(e == PAL_FS_OK);
    T_ASSERT(pal_fs_is_file(file) == false);

    e = pal_fs_remove(dir);
    T_ASSERT(e == PAL_FS_OK);
    T_ASSERT(pal_fs_is_dir(dir) == false);
}

static void test_path_join_normalization(void) {
    char tmp[512] = {0};
    T_ASSERT(pal_fs_temp_dir(tmp, sizeof(tmp)) == PAL_FS_OK);

    char out[1024] = {0};
    T_ASSERT(pal_fs_path_join(out, sizeof(out), tmp, "a/b/c.txt") == PAL_FS_OK);
    T_ASSERT(out[0] != '\0');

    /* Not asserting exact separators (platform dependent), only that it contains segments. */
    T_ASSERT(strstr(out, "a") != NULL);
    T_ASSERT(strstr(out, "b") != NULL);
    T_ASSERT(strstr(out, "c.txt") != NULL);
}

int main(void) {
    test_temp_dir();
    test_mkdir_write_read_remove();
    test_path_join_normalization();

    if (g_fail) return 1;
    printf("OK: pal fs tests\n");
    return 0;
}
