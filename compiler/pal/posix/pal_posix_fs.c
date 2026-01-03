// SPDX-License-Identifier: MIT
// pal_posix_fs.c
//
// POSIX filesystem backend (max).
//
// Provides:
//  - file read/write (binary/text)
//  - existence checks, stat, mkdir -p
//  - directory listing (non-recursive)
//  - path helpers (join, basename/dirname)
//
// Notes:
//  - This module intentionally uses plain POSIX APIs (open/read/write/close,
//    stat, mkdir, opendir/readdir).
//  - For portability between Linux/macOS, avoid GNU-only functions.
//
// Integration:
//  - If `pal_fs.h` exists, this file includes it.
//  - Otherwise, it provides a fallback API that should later be moved into
//    `pal_fs.h`.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("../pal_fs.h")
    #include "../pal_fs.h"
    #define STEEL_HAS_PAL_FS_H 1
  #elif __has_include("pal_fs.h")
    #include "pal_fs.h"
    #define STEEL_HAS_PAL_FS_H 1
  #endif
#endif

#ifndef STEEL_HAS_PAL_FS_H

//------------------------------------------------------------------------------
// Fallback public API (align later with pal_fs.h)
//------------------------------------------------------------------------------

typedef struct pal_fs_stat
{
    bool exists;
    bool is_file;
    bool is_dir;
    uint64_t size;
    uint64_t mtime_sec;
} pal_fs_stat;

typedef struct pal_fs_blob
{
    uint8_t* data; // owned
    size_t len;
} pal_fs_blob;

typedef struct pal_fs_list
{
    char** items;  // owned array of owned strings
    size_t count;
} pal_fs_list;

void pal_fs_blob_dispose(pal_fs_blob* b);
void pal_fs_list_dispose(pal_fs_list* l);

bool pal_fs_exists(const char* path);

bool pal_fs_stat_path(const char* path, pal_fs_stat* out);

bool pal_fs_mkdir_p(const char* path, uint32_t mode);

bool pal_fs_read_all(const char* path, pal_fs_blob* out);
bool pal_fs_write_all(const char* path, const void* data, size_t len, bool atomic);

bool pal_fs_list_dir(const char* path, pal_fs_list* out);

// Path helpers
bool   pal_path_is_abs(const char* path);
size_t pal_path_join(char* out, size_t out_cap, const char* a, const char* b);
size_t pal_path_basename(char* out, size_t out_cap, const char* path);
size_t pal_path_dirname(char* out, size_t out_cap, const char* path);

const char* pal_fs_last_error(void);

#endif // !STEEL_HAS_PAL_FS_H

//------------------------------------------------------------------------------
// Error handling
//------------------------------------------------------------------------------

static _Thread_local char g_last_err_[256];

static void set_err_(const char* msg)
{
    if (!msg) msg = "";
    snprintf(g_last_err_, sizeof(g_last_err_), "%s", msg);
}

static void set_errno_err_(const char* prefix)
{
    const char* e = strerror(errno);
    if (!prefix) prefix = "";
    snprintf(g_last_err_, sizeof(g_last_err_), "%s%s%s", prefix, (prefix[0] ? ": " : ""), e ? e : "");
}

const char* pal_fs_last_error(void)
{
    return g_last_err_;
}

//------------------------------------------------------------------------------
// Memory helpers
//------------------------------------------------------------------------------

static char* strdup_(const char* s)
{
    if (!s) s = "";
    size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static size_t snprint_(char* out, size_t cap, const char* s)
{
    if (!out || cap == 0) return 0;
    if (!s) s = "";
    size_t n = strlen(s);
    if (n >= cap) n = cap - 1;
    memcpy(out, s, n);
    out[n] = 0;
    return n;
}

//------------------------------------------------------------------------------
// Dispose
//------------------------------------------------------------------------------

void pal_fs_blob_dispose(pal_fs_blob* b)
{
    if (!b) return;
    free(b->data);
    b->data = NULL;
    b->len = 0;
}

void pal_fs_list_dispose(pal_fs_list* l)
{
    if (!l) return;
    for (size_t i = 0; i < l->count; i++)
        free(l->items[i]);
    free(l->items);
    l->items = NULL;
    l->count = 0;
}

//------------------------------------------------------------------------------
// Stat / exists
//------------------------------------------------------------------------------

bool pal_fs_exists(const char* path)
{
    if (!path || !path[0]) return false;
    struct stat st;
    return stat(path, &st) == 0;
}

bool pal_fs_stat_path(const char* path, pal_fs_stat* out)
{
    if (out) memset(out, 0, sizeof(*out));
    if (!path || !path[0])
    {
        set_err_("empty path");
        return false;
    }

    struct stat st;
    if (stat(path, &st) != 0)
    {
        if (errno == ENOENT)
        {
            if (out) out->exists = false;
            return true;
        }
        set_errno_err_("stat");
        return false;
    }

    if (out)
    {
        out->exists = true;
        out->is_dir = S_ISDIR(st.st_mode);
        out->is_file = S_ISREG(st.st_mode);
        out->size = (uint64_t)st.st_size;

#if defined(__APPLE__)
        out->mtime_sec = (uint64_t)st.st_mtimespec.tv_sec;
#else
        out->mtime_sec = (uint64_t)st.st_mtim.tv_sec;
#endif
    }

    return true;
}

//------------------------------------------------------------------------------
// mkdir -p
//------------------------------------------------------------------------------

static bool mkdir_one_(const char* p, uint32_t mode)
{
    if (mkdir(p, (mode_t)mode) == 0)
        return true;

    if (errno == EEXIST)
    {
        struct stat st;
        if (stat(p, &st) == 0 && S_ISDIR(st.st_mode))
            return true;
    }

    return false;
}

bool pal_fs_mkdir_p(const char* path, uint32_t mode)
{
    if (!path || !path[0])
    {
        set_err_("empty path");
        return false;
    }

    // Copy to mutable buffer.
    size_t n = strlen(path);
    char* tmp = (char*)malloc(n + 1);
    if (!tmp)
    {
        set_err_("out of memory");
        return false;
    }

    memcpy(tmp, path, n + 1);

    // Remove trailing '/' (except root).
    while (n > 1 && tmp[n - 1] == '/')
    {
        tmp[n - 1] = 0;
        n--;
    }

    for (size_t i = 1; tmp[i]; i++)
    {
        if (tmp[i] == '/')
        {
            tmp[i] = 0;
            if (!mkdir_one_(tmp, mode))
            {
                set_errno_err_("mkdir");
                free(tmp);
                return false;
            }
            tmp[i] = '/';
        }
    }

    if (!mkdir_one_(tmp, mode))
    {
        set_errno_err_("mkdir");
        free(tmp);
        return false;
    }

    free(tmp);
    return true;
}

//------------------------------------------------------------------------------
// Read all
//------------------------------------------------------------------------------

static bool read_all_fd_(int fd, pal_fs_blob* out)
{
    if (out) memset(out, 0, sizeof(*out));

    struct stat st;
    if (fstat(fd, &st) != 0)
    {
        set_errno_err_("fstat");
        return false;
    }

    size_t cap = (size_t)st.st_size;
    if (cap == 0) cap = 64;

    uint8_t* buf = (uint8_t*)malloc(cap + 1);
    if (!buf)
    {
        set_err_("out of memory");
        return false;
    }

    size_t len = 0;
    for (;;)
    {
        if (len == cap)
        {
            size_t ncap = cap * 2;
            uint8_t* nb = (uint8_t*)realloc(buf, ncap + 1);
            if (!nb)
            {
                free(buf);
                set_err_("out of memory");
                return false;
            }
            buf = nb;
            cap = ncap;
        }

        ssize_t r = read(fd, buf + len, cap - len);
        if (r == 0) break;
        if (r < 0)
        {
            if (errno == EINTR) continue;
            free(buf);
            set_errno_err_("read");
            return false;
        }

        len += (size_t)r;
    }

    buf[len] = 0; // NUL terminate for convenience

    if (out)
    {
        out->data = buf;
        out->len = len;
    }
    else
    {
        free(buf);
    }

    return true;
}

bool pal_fs_read_all(const char* path, pal_fs_blob* out)
{
    if (out) memset(out, 0, sizeof(*out));

    if (!path || !path[0])
    {
        set_err_("empty path");
        return false;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        set_errno_err_("open");
        return false;
    }

    bool ok = read_all_fd_(fd, out);

    close(fd);
    return ok;
}

//------------------------------------------------------------------------------
// Write all (optionally atomic)
//------------------------------------------------------------------------------

static bool write_all_fd_(int fd, const uint8_t* data, size_t len)
{
    size_t off = 0;
    while (off < len)
    {
        ssize_t w = write(fd, data + off, len - off);
        if (w < 0)
        {
            if (errno == EINTR) continue;
            set_errno_err_("write");
            return false;
        }
        off += (size_t)w;
    }

    if (fsync(fd) != 0)
    {
        set_errno_err_("fsync");
        return false;
    }

    return true;
}

static bool make_tmp_path_(char* out, size_t out_cap, const char* path)
{
    // tmp = path + ".tmp.<pid>"
    pid_t pid = getpid();
    int n = snprintf(out, out_cap, "%s.tmp.%ld", path, (long)pid);
    if (n < 0 || (size_t)n >= out_cap)
        return false;
    return true;
}

bool pal_fs_write_all(const char* path, const void* data, size_t len, bool atomic)
{
    if (!path || !path[0])
    {
        set_err_("empty path");
        return false;
    }

    if (!data && len != 0)
    {
        set_err_("null data");
        return false;
    }

    const uint8_t* bytes = (const uint8_t*)data;

    if (!atomic)
    {
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            set_errno_err_("open");
            return false;
        }

        bool ok = write_all_fd_(fd, bytes, len);
        close(fd);
        return ok;
    }

    // atomic: write to tmp then rename
    char tmp[4096];
    if (!make_tmp_path_(tmp, sizeof(tmp), path))
    {
        set_err_("tmp path too long");
        return false;
    }

    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        set_errno_err_("open(tmp)");
        return false;
    }

    bool ok = write_all_fd_(fd, bytes, len);
    close(fd);

    if (!ok)
    {
        (void)unlink(tmp);
        return false;
    }

    if (rename(tmp, path) != 0)
    {
        set_errno_err_("rename");
        (void)unlink(tmp);
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// Directory listing (non-recursive)
//------------------------------------------------------------------------------

bool pal_fs_list_dir(const char* path, pal_fs_list* out)
{
    if (out) memset(out, 0, sizeof(*out));

    if (!path || !path[0])
    {
        set_err_("empty path");
        return false;
    }

    DIR* d = opendir(path);
    if (!d)
    {
        set_errno_err_("opendir");
        return false;
    }

    size_t cap = 16;
    char** items = (char**)calloc(cap, sizeof(char*));
    if (!items)
    {
        closedir(d);
        set_err_("out of memory");
        return false;
    }

    size_t count = 0;

    for (;;)
    {
        errno = 0;
        struct dirent* ent = readdir(d);
        if (!ent)
        {
            if (errno != 0)
                set_errno_err_("readdir");
            break;
        }

        const char* name = ent->d_name;
        if (!name) continue;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        if (count == cap)
        {
            size_t ncap = cap * 2;
            char** nitems = (char**)realloc(items, ncap * sizeof(char*));
            if (!nitems)
            {
                set_err_("out of memory");
                break;
            }
            memset(nitems + cap, 0, (ncap - cap) * sizeof(char*));
            items = nitems;
            cap = ncap;
        }

        char* copy = strdup_(name);
        if (!copy)
        {
            set_err_("out of memory");
            break;
        }

        items[count++] = copy;
    }

    closedir(d);

    // If errno set during readdir, treat as failure.
    if (errno != 0)
    {
        for (size_t i = 0; i < count; i++) free(items[i]);
        free(items);
        return false;
    }

    if (out)
    {
        out->items = items;
        out->count = count;
    }
    else
    {
        for (size_t i = 0; i < count; i++) free(items[i]);
        free(items);
    }

    return true;
}

//------------------------------------------------------------------------------
// Path helpers
//------------------------------------------------------------------------------

bool pal_path_is_abs(const char* path)
{
    if (!path || !path[0]) return false;
    return path[0] == '/';
}

size_t pal_path_join(char* out, size_t out_cap, const char* a, const char* b)
{
    if (!out || out_cap == 0) return 0;
    if (!a) a = "";
    if (!b) b = "";

    // If b is absolute, return b
    if (pal_path_is_abs(b))
        return snprint_(out, out_cap, b);

    size_t al = strlen(a);
    size_t bl = strlen(b);

    // Trim trailing slashes on a
    while (al > 0 && a[al - 1] == '/') al--;

    // Trim leading slashes on b
    size_t bi = 0;
    while (b[bi] == '/') bi++;
    bl = strlen(b + bi);

    // Special-case empty a
    if (al == 0)
        return snprint_(out, out_cap, b + bi);

    size_t need = al + 1 + bl;
    if (need + 1 > out_cap)
    {
        // best-effort truncation
        // write a
        size_t used = 0;
        size_t copy_a = (al < out_cap - 1) ? al : (out_cap - 1);
        memcpy(out, a, copy_a);
        used += copy_a;
        if (used < out_cap - 1) out[used++] = '/';
        if (used < out_cap - 1)
        {
            size_t copy_b = (bl < (out_cap - 1 - used)) ? bl : (out_cap - 1 - used);
            memcpy(out + used, b + bi, copy_b);
            used += copy_b;
        }
        out[used] = 0;
        return used;
    }

    memcpy(out, a, al);
    out[al] = '/';
    memcpy(out + al + 1, b + bi, bl);
    out[al + 1 + bl] = 0;

    return al + 1 + bl;
}

size_t pal_path_basename(char* out, size_t out_cap, const char* path)
{
    if (!out || out_cap == 0) return 0;
    if (!path) path = "";

    size_t n = strlen(path);
    while (n > 1 && path[n - 1] == '/') n--;

    const char* end = path + n;
    const char* p = end;
    while (p > path && *(p - 1) != '/') p--;

    return snprint_(out, out_cap, p);
}

size_t pal_path_dirname(char* out, size_t out_cap, const char* path)
{
    if (!out || out_cap == 0) return 0;
    if (!path) path = "";

    size_t n = strlen(path);
    if (n == 0) return snprint_(out, out_cap, ".");

    while (n > 1 && path[n - 1] == '/') n--;

    // Find last '/'
    ssize_t i = (ssize_t)n - 1;
    while (i >= 0 && path[i] != '/') i--;

    if (i < 0)
        return snprint_(out, out_cap, ".");

    if (i == 0)
        return snprint_(out, out_cap, "/");

    // Copy up to i
    size_t copy = (size_t)i;
    if (copy >= out_cap) copy = out_cap - 1;
    memcpy(out, path, copy);
    out[copy] = 0;
    return copy;
}

