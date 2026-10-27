#include "filesystem/filesystem.h"
#include "platform/platform.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

static void
test_path_forms(void)
{
    char *normalized = NULL;
    char *joined = NULL;

    assert(vitte_filesystem_path_is_absolute("/tmp/vitte"));
    assert(vitte_filesystem_path_is_absolute("C:\\\\work\\src"));
    assert(vitte_filesystem_path_is_absolute("\\\\server\\share"));
    assert(!vitte_filesystem_path_is_absolute("src\\main.vit"));

    assert(vitte_filesystem_normalize(
        "C:\\\\work\\.\\src\\..\\main.vit",
        &normalized).error == VITTE_FILESYSTEM_ERROR_NONE);
    assert(strcmp(normalized, "C:/work/main.vit") == 0);
    free(normalized);
    normalized = NULL;

    assert(vitte_filesystem_normalize(
        "\\\\server\\share\\folder\\..\\file.vit",
        &normalized).error == VITTE_FILESYSTEM_ERROR_NONE);
    assert(strcmp(normalized, "//server/share/file.vit") == 0);
    free(normalized);
    normalized = NULL;

    assert(vitte_filesystem_join(
        "src\\module",
        "generated\\main.c",
        &joined).error == VITTE_FILESYSTEM_ERROR_NONE);
    assert(strcmp(joined, "src/module/generated/main.c") == 0);
    free(joined);
}

static void
test_unicode_and_symlink(void)
{
    char *cwd = NULL;
    char *directory = NULL;
    char *file = NULL;
    char *link = NULL;
    const char contents[] = "space tests::unicode;\n";
    char *canonical = NULL;
    char *canonical_file = NULL;
    vitte_filesystem_buffer_t buffer = {0};

    assert(vitte_filesystem_current_directory(&cwd).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_join(
        cwd,
        ".vitte-fs-portability",
        &directory).error == VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_join(
        directory,
        "données.vit",
        &file).error == VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_join(
        directory,
        "alias.vit",
        &link).error == VITTE_FILESYSTEM_ERROR_NONE);
    (void)vitte_filesystem_remove_file(link);
    (void)vitte_filesystem_remove_file(file);
    (void)vitte_filesystem_remove_directory(directory);
    assert(vitte_filesystem_create_directories(directory).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_write_file(
        file, contents, sizeof(contents) - 1u).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_read_file(file, 0u, &buffer).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(buffer.length == sizeof(contents) - 1u);
    assert(memcmp(buffer.data, contents, buffer.length) == 0);
    vitte_filesystem_buffer_destroy(&buffer);

#if !defined(_WIN32)
    assert(symlink(file, link) == 0);
    assert(vitte_filesystem_is_symlink(link));
    assert(vitte_filesystem_canonicalize(link, &canonical).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_canonicalize(file, &canonical_file).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(strcmp(canonical, canonical_file) == 0);
    free(canonical);
    free(canonical_file);
#else
    {
        char *target = NULL;
        wchar_t wide_link[MAX_PATH];
        wchar_t wide_target[MAX_PATH];
        int link_size;
        int target_size;

        assert(vitte_filesystem_join(
            directory, "reparse-target", &target).error ==
               VITTE_FILESYSTEM_ERROR_NONE);
        assert(vitte_filesystem_create_directory(target).error ==
               VITTE_FILESYSTEM_ERROR_NONE);
        link_size = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, link, -1,
            wide_link, MAX_PATH);
        target_size = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, target, -1,
            wide_target, MAX_PATH);
        assert(link_size > 0 && target_size > 0);
        /* Developer Mode or the SeCreateSymbolicLink privilege is required
         * for this optional check.  The rest of the Windows portability job
         * remains valid when the host denies link creation. */
        if (CreateSymbolicLinkW(
                wide_link, wide_target, SYMBOLIC_LINK_FLAG_DIRECTORY) != 0) {
            assert(vitte_filesystem_is_symlink(link));
            assert(vitte_filesystem_remove_file(link).error ==
                   VITTE_FILESYSTEM_ERROR_NONE);
            assert(vitte_filesystem_is_directory(target));
        }
        assert(vitte_filesystem_remove_directory(target).error ==
               VITTE_FILESYSTEM_ERROR_NONE);
        free(target);
    }
#endif

    assert(vitte_filesystem_remove_file(link).error ==
           VITTE_FILESYSTEM_ERROR_NONE ||
           !vitte_filesystem_exists(link));
    assert(vitte_filesystem_remove_file(file).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_remove_directory(directory).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    free(link);
    free(file);
    free(directory);
    free(cwd);
}

static void
test_process_boundary(void)
{
#if defined(_WIN32)
    const char *arguments[] = {"cmd", "/C", "exit", "0", NULL};
#else
    const char *arguments[] = {"true", NULL};
#endif
    assert(vitte_platform_run(arguments, true));
    assert(!vitte_platform_run(NULL, true));
}

int
main(void)
{
    test_path_forms();
    test_unicode_and_symlink();
    test_process_boundary();
    return 0;
}
