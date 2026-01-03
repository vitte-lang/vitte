// SPDX-License-Identifier: MIT
// fs.c
//
// Minimal filesystem helpers for vitte.
//
// Goals:
//  - Cross-platform (POSIX + Windows) primitives.
//  - Dependency-light.
//  - Provide small building blocks used by CLI/tools/runtime.
//
// This file pairs with fs.h.
// fs.h is expected to declare the functions implemented here.
// If your fs.h differs, adjust one side accordingly.

#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <direct.h>
#else
  #include <sys/stat.h>
  #include <dirent.h>
  #include <unistd.h>
#endif

#ifndef STEEL_FS_PATH_MAX
  #define STEEL_FS_PATH_MAX 4096
#endif

#ifndef STEEL_FS_IO_CHUNK
  #define STEEL_FS_IO_CHUNK (64u * 1024u)
#endif

//------------------------------------------------------------------------------
// Small utilities
//------------------------------------------------------------------------------

static void* steel_xmalloc(size_t n)
{
    void* p = malloc(n);
    if (!p)
    {
        fprintf(stderr, "vitte: out of memory\n");
        abort();
    }
    return p;
}

static void* steel_xrealloc(void* p, size_t n)
{
    void* q = realloc(p, n);
    if (!q)
    {
        fprintf(stderr, "vitte: out of memory\n");
        abort();
    }
    return q;
}

static char* steel_xstrdup(const char* s)
{
    if (!s) return NULL;
    size_t n = strlen(s);
    char* d = (char*)steel_xmalloc(n + 1);
    memcpy(d, s, n + 1);
    return d;
}

static bool steel_starts_with(const char* s, const char* pfx)
{
    size_t m = strlen(pfx);
    return strncmp(s, pfx, m) == 0;
}

static bool steel_ends_with(const char* s, const char* sfx)
{
    size_t n = strlen(s);
    size_t m = strlen(sfx);
    if (m > n) return false;
    return memcmp(s + (n - m), sfx, m) == 0;
}

static void steel_path_join(char* out, size_t out_cap, const char* a, const char* b)
{
    if (!a || !a[0])
    {
        snprintf(out, out_cap, "%s", b ? b : "");
        return;
    }
    if (!b || !b[0])
    {
        snprintf(out, out_cap, "%s", a);
        return;
    }

#if defined(_WIN32)
    const char sep = '\\';
#else
    const char sep = '/';
#endif

    size_t al = strlen(a);
    bool a_has = (a[al - 1] == '/' || a[al - 1] == '\\');
    if (a_has)
        snprintf(out, out_cap, "%s%s", a, b);
    else
        snprintf(out, out_cap, "%s%c%s", a, sep, b);
}

static const char* steel_basename(const char* path)
{
    const char* a = strrchr(path, '/');
    const char* b = strrchr(path, '\\');
    const char* p = a;
    if (b && (!p || b > p)) p = b;
    return p ? p + 1 : path;
}

//------------------------------------------------------------------------------
// Stats / type queries
//------------------------------------------------------------------------------

bool steel_fs_exists(const char* path)
{
#if defined(_WIN32)
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    return access(path, F_OK) == 0;
#endif
}

bool steel_fs_is_file(const char* path)
{
#if defined(_WIN32)
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISREG(st.st_mode);
#endif
}

bool steel_fs_is_dir(const char* path)
{
#if defined(_WIN32)
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
#endif
}

//------------------------------------------------------------------------------
// Directory creation
//------------------------------------------------------------------------------

static bool steel_mkdir_one(const char* path)
{
#if defined(_WIN32)
    if (CreateDirectoryA(path, NULL)) return true;
    DWORD e = GetLastError();
    return (e == ERROR_ALREADY_EXISTS);
#else
    if (mkdir(path, 0755) == 0) return true;
    return (errno == EEXIST);
#endif
}

bool steel_fs_mkdirs(const char* path)
{
    if (!path || !path[0]) return false;

    char tmp[STEEL_FS_PATH_MAX];
    size_t n = strlen(path);
    if (n >= sizeof(tmp)) return false;
    memcpy(tmp, path, n + 1);

    for (size_t i = 0; i < n; i++)
    {
        char c = tmp[i];
        if (c == '/' || c == '\\')
        {
            tmp[i] = 0;
            if (tmp[0]) (void)steel_mkdir_one(tmp);
            tmp[i] = c;
        }
    }

    return steel_mkdir_one(tmp);
}

//------------------------------------------------------------------------------
// Remove
//------------------------------------------------------------------------------

bool steel_fs_remove_file(const char* path)
{
#if defined(_WIN32)
    return DeleteFileA(path) != 0;
#else
    return unlink(path) == 0;
#endif
}

bool steel_fs_remove_empty_dir(const char* path)
{
#if defined(_WIN32)
    return RemoveDirectoryA(path) != 0;
#else
    return rmdir(path) == 0;
#endif
}

//------------------------------------------------------------------------------
// Current working directory
//------------------------------------------------------------------------------

bool steel_fs_get_cwd(char* out, size_t out_cap)
{
#if defined(_WIN32)
    DWORD n = GetCurrentDirectoryA((DWORD)out_cap, out);
    return (n > 0 && n < (DWORD)out_cap);
#else
    return getcwd(out, out_cap) != NULL;
#endif
}

bool steel_fs_set_cwd(const char* path)
{
#if defined(_WIN32)
    return SetCurrentDirectoryA(path) != 0;
#else
    return chdir(path) == 0;
#endif
}

//------------------------------------------------------------------------------
// Read / write whole files
//------------------------------------------------------------------------------

typedef struct steel_file_buf
{
    uint8_t* data;
    size_t len;
} steel_file_buf;

// If fs.h already defines these types, this internal typedef is unused.

bool steel_fs_read_all(const char* path, uint8_t** out_data, size_t* out_len)
{
    if (!out_data || !out_len) return false;
    *out_data = NULL;
    *out_len = 0;

    FILE* f = fopen(path, "rb");
    if (!f) return false;

    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return false;
    }

    long sz = ftell(f);
    if (sz < 0)
    {
        fclose(f);
        return false;
    }

    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return false;
    }

    size_t n = (size_t)sz;
    uint8_t* buf = (uint8_t*)malloc(n ? n : 1);
    if (!buf)
    {
        fclose(f);
        return false;
    }

    size_t got = fread(buf, 1, n, f);
    if (got != n)
    {
        free(buf);
        fclose(f);
        return false;
    }

    fclose(f);
    *out_data = buf;
    *out_len = n;
    return true;
}

bool steel_fs_write_all(const char* path, const void* data, size_t len)
{
    // Ensure parent directory exists.
    {
        char dir[STEEL_FS_PATH_MAX];
        strncpy(dir, path, sizeof(dir) - 1);
        dir[sizeof(dir) - 1] = 0;

        char* slash1 = strrchr(dir, '/');
        char* slash2 = strrchr(dir, '\\');
        char* slash = slash1;
        if (slash2 && (!slash || slash2 > slash)) slash = slash2;
        if (slash)
        {
            *slash = 0;
            if (dir[0]) (void)steel_fs_mkdirs(dir);
        }
    }

    FILE* f = fopen(path, "wb");
    if (!f) return false;

    if (len)
    {
        if (fwrite(data, 1, len, f) != len)
        {
            fclose(f);
            return false;
        }
    }

    if (fclose(f) != 0) return false;
    return true;
}

//------------------------------------------------------------------------------
// Copy / move
//------------------------------------------------------------------------------

bool steel_fs_copy_file(const char* src, const char* dst)
{
#if defined(_WIN32)
    // CopyFileA fails if dst exists unless bFailIfExists=FALSE.
    if (CopyFileA(src, dst, FALSE)) return true;
    // Ensure parent directory for dst.
    {
        char dir[STEEL_FS_PATH_MAX];
        strncpy(dir, dst, sizeof(dir) - 1);
        dir[sizeof(dir) - 1] = 0;
        char* slash1 = strrchr(dir, '/');
        char* slash2 = strrchr(dir, '\\');
        char* slash = slash1;
        if (slash2 && (!slash || slash2 > slash)) slash = slash2;
        if (slash)
        {
            *slash = 0;
            if (dir[0]) (void)steel_fs_mkdirs(dir);
        }
    }
    return CopyFileA(src, dst, FALSE) != 0;
#else
    FILE* in = fopen(src, "rb");
    if (!in) return false;

    // Ensure destination directory exists.
    {
        char dir[STEEL_FS_PATH_MAX];
        strncpy(dir, dst, sizeof(dir) - 1);
        dir[sizeof(dir) - 1] = 0;
        char* slash = strrchr(dir, '/');
        if (slash)
        {
            *slash = 0;
            if (dir[0]) (void)steel_fs_mkdirs(dir);
        }
    }

    FILE* out = fopen(dst, "wb");
    if (!out)
    {
        fclose(in);
        return false;
    }

    uint8_t buf[STEEL_FS_IO_CHUNK];
    for (;;)
    {
        size_t n = fread(buf, 1, sizeof(buf), in);
        if (n == 0)
        {
            if (ferror(in))
            {
                fclose(in);
                fclose(out);
                return false;
            }
            break;
        }

        if (fwrite(buf, 1, n, out) != n)
        {
            fclose(in);
            fclose(out);
            return false;
        }
    }

    fclose(in);
    if (fclose(out) != 0) return false;
    return true;
#endif
}

bool steel_fs_move(const char* src, const char* dst)
{
#if defined(_WIN32)
    // MOVEFILE_REPLACE_EXISTING for overwrite.
    if (MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING)) return true;
    // Ensure destination directory exists then retry.
    {
        char dir[STEEL_FS_PATH_MAX];
        strncpy(dir, dst, sizeof(dir) - 1);
        dir[sizeof(dir) - 1] = 0;
        char* slash1 = strrchr(dir, '/');
        char* slash2 = strrchr(dir, '\\');
        char* slash = slash1;
        if (slash2 && (!slash || slash2 > slash)) slash = slash2;
        if (slash)
        {
            *slash = 0;
            if (dir[0]) (void)steel_fs_mkdirs(dir);
        }
    }
    return MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    // rename() is atomic on same filesystem.
    if (rename(src, dst) == 0) return true;

    // Try copy+unlink as fallback.
    if (!steel_fs_copy_file(src, dst)) return false;
    return unlink(src) == 0;
#endif
}

//------------------------------------------------------------------------------
// Directory listing (simple)
//------------------------------------------------------------------------------

typedef struct steel_fs_list
{
    char** items;
    size_t len;
    size_t cap;
} steel_fs_list;

static void steel_list_push(steel_fs_list* l, const char* s)
{
    if (l->len + 1 > l->cap)
    {
        l->cap = (l->cap == 0) ? 64 : (l->cap * 2);
        l->items = (char**)steel_xrealloc(l->items, l->cap * sizeof(char*));
    }
    l->items[l->len++] = steel_xstrdup(s);
}

static void steel_list_free(steel_fs_list* l)
{
    for (size_t i = 0; i < l->len; i++) free(l->items[i]);
    free(l->items);
    l->items = NULL;
    l->len = 0;
    l->cap = 0;
}

bool steel_fs_list_dir(const char* path, char*** out_items, size_t* out_len)
{
    if (!out_items || !out_len) return false;
    *out_items = NULL;
    *out_len = 0;

    if (!steel_fs_is_dir(path)) return false;

    steel_fs_list list;
    list.items = NULL;
    list.len = 0;
    list.cap = 0;

#if defined(_WIN32)
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);

    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    if (h == INVALID_HANDLE_VALUE)
    {
        steel_list_free(&list);
        return false;
    }

    do
    {
        const char* name = ffd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        steel_list_push(&list, name);
    } while (FindNextFileA(h, &ffd));

    FindClose(h);
#else
    DIR* d = opendir(path);
    if (!d)
    {
        steel_list_free(&list);
        return false;
    }

    for (;;)
    {
        struct dirent* ent = readdir(d);
        if (!ent) break;
        const char* name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        steel_list_push(&list, name);
    }

    closedir(d);
#endif

    // Transfer ownership
    *out_items = list.items;
    *out_len = list.len;
    return true;
}

void steel_fs_list_free(char** items, size_t len)
{
    if (!items) return;
    for (size_t i = 0; i < len; i++) free(items[i]);
    free(items);
}

//------------------------------------------------------------------------------
// Temp file path helper
//------------------------------------------------------------------------------

bool steel_fs_temp_path(char* out, size_t out_cap, const char* prefix)
{
    if (!out || out_cap == 0) return false;
    if (!prefix) prefix = "tmp";

#if defined(_WIN32)
    char dir[MAX_PATH];
    DWORD n = GetTempPathA((DWORD)sizeof(dir), dir);
    if (n == 0 || n >= sizeof(dir)) return false;

    // Use PID + tick count for uniqueness.
    DWORD pid = GetCurrentProcessId();
    DWORD tick = GetTickCount();

    snprintf(out, out_cap, "%s%s_%lu_%lu.tmp", dir, prefix, (unsigned long)pid, (unsigned long)tick);
    return true;
#else
    const char* tmp = getenv("TMPDIR");
    if (!tmp || !tmp[0]) tmp = "/tmp";

    // Use PID for uniqueness; mkstemp is better but we keep minimal.
    int pid = (int)getpid();
    snprintf(out, out_cap, "%s/%s_%d.tmp", tmp, prefix, pid);
    return true;
#endif
}
