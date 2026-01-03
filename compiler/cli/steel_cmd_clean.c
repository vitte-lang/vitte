// SPDX-License-Identifier: MIT
// steel_cmd_clean.c
//
// `steel clean` command implementation.
//
// Goals:
//  - Provide a safe, predictable cleanup command for build artifacts and caches.
//  - Default behavior: remove well-known generated directories under the given
//    roots (or `.` when none provided).
//  - Optional: deeper cleanup (object files, binaries) with `--all`.
//  - Optional JSON report.
//
// Notes:
//  - This file is intentionally self-contained (stdlib + minimal filesystem
//    traversal) so it can compile early in bootstrap stages.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdarg.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <sys/stat.h>
  #include <dirent.h>
  #include <unistd.h>
#endif

#ifndef STEEL_CLEAN_MAX_MSG
  #define STEEL_CLEAN_MAX_MSG 512
#endif

#ifndef STEEL_CLEAN_PATH_MAX
  #define STEEL_CLEAN_PATH_MAX 4096
#endif

//------------------------------------------------------------------------------
// Small utilities
//------------------------------------------------------------------------------

static void* steel_xmalloc(size_t n)
{
    void* p = malloc(n);
    if (!p)
    {
        fprintf(stderr, "steel: out of memory\n");
        abort();
    }
    return p;
}

static void* steel_xrealloc(void* p, size_t n)
{
    void* q = realloc(p, n);
    if (!q)
    {
        fprintf(stderr, "steel: out of memory\n");
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
// Dynamic arrays
//------------------------------------------------------------------------------

typedef struct steel_strvec
{
    char** items;
    size_t len;
    size_t cap;
} steel_strvec;

static void steel_strvec_init(steel_strvec* v)
{
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

static void steel_strvec_free(steel_strvec* v)
{
    if (!v) return;
    for (size_t i = 0; i < v->len; i++) free(v->items[i]);
    free(v->items);
    steel_strvec_init(v);
}

static void steel_strvec_push(steel_strvec* v, const char* s)
{
    if (v->len + 1 > v->cap)
    {
        v->cap = (v->cap == 0) ? 16 : (v->cap * 2);
        v->items = (char**)steel_xrealloc(v->items, v->cap * sizeof(char*));
    }
    v->items[v->len++] = steel_xstrdup(s);
}

//------------------------------------------------------------------------------
// Operation report
//------------------------------------------------------------------------------

typedef enum steel_clean_action
{
    STEEL_CLEAN_REMOVE_FILE = 1,
    STEEL_CLEAN_REMOVE_DIR = 2
} steel_clean_action;

typedef struct steel_clean_op
{
    steel_clean_action action;
    char* path;
    bool ok;
    char msg[STEEL_CLEAN_MAX_MSG];
} steel_clean_op;

typedef struct steel_clean_opvec
{
    steel_clean_op* items;
    size_t len;
    size_t cap;
} steel_clean_opvec;

static void steel_clean_opvec_init(steel_clean_opvec* v)
{
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

static void steel_clean_opvec_free(steel_clean_opvec* v)
{
    if (!v) return;
    for (size_t i = 0; i < v->len; i++) free(v->items[i].path);
    free(v->items);
    steel_clean_opvec_init(v);
}

static void steel_clean_opvec_add(steel_clean_opvec* v, steel_clean_action act, const char* path, bool ok, const char* fmt, ...)
{
    if (v->len + 1 > v->cap)
    {
        v->cap = (v->cap == 0) ? 16 : (v->cap * 2);
        v->items = (steel_clean_op*)steel_xrealloc(v->items, v->cap * sizeof(steel_clean_op));
    }

    steel_clean_op* o = &v->items[v->len++];
    memset(o, 0, sizeof(*o));
    o->action = act;
    o->path = steel_xstrdup(path);
    o->ok = ok;

    if (fmt && fmt[0])
    {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(o->msg, sizeof(o->msg), fmt, ap);
        va_end(ap);
    }
    else
    {
        o->msg[0] = 0;
    }
}

//------------------------------------------------------------------------------
// Filesystem helpers
//------------------------------------------------------------------------------

static bool steel_is_directory(const char* path)
{
#if defined(_WIN32)
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    if (lstat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
#endif
}

static bool steel_path_exists(const char* path)
{
#if defined(_WIN32)
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return lstat(path, &st) == 0;
#endif
}

static bool steel_is_symlink(const char* path)
{
#if defined(_WIN32)
    // We treat Windows reparse points conservatively as non-directories for recursion.
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    struct stat st;
    if (lstat(path, &st) != 0) return false;
    return S_ISLNK(st.st_mode);
#endif
}

#if defined(_WIN32)
static void steel_win_make_writable(const char* path)
{
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return;
    if (attrs & FILE_ATTRIBUTE_READONLY)
    {
        attrs &= ~FILE_ATTRIBUTE_READONLY;
        (void)SetFileAttributesA(path, attrs);
    }
}
#endif

static bool steel_remove_file(const char* path, char* err, size_t err_cap)
{
#if defined(_WIN32)
    steel_win_make_writable(path);
    if (DeleteFileA(path)) return true;
    DWORD e = GetLastError();
    snprintf(err, err_cap, "DeleteFile failed (winerr=%lu)", (unsigned long)e);
    return false;
#else
    if (unlink(path) == 0) return true;
    snprintf(err, err_cap, "unlink failed: %s", strerror(errno));
    return false;
#endif
}

static bool steel_remove_empty_dir(const char* path, char* err, size_t err_cap)
{
#if defined(_WIN32)
    steel_win_make_writable(path);
    if (RemoveDirectoryA(path)) return true;
    DWORD e = GetLastError();
    snprintf(err, err_cap, "RemoveDirectory failed (winerr=%lu)", (unsigned long)e);
    return false;
#else
    if (rmdir(path) == 0) return true;
    snprintf(err, err_cap, "rmdir failed: %s", strerror(errno));
    return false;
#endif
}

static bool steel_remove_tree(const char* path, bool dry_run, steel_clean_opvec* ops, size_t* removed_files, size_t* removed_dirs, size_t* errors)
{
    if (!steel_path_exists(path)) return true;

    // Never recurse into symlinks.
    if (steel_is_symlink(path) && !steel_is_directory(path))
    {
        char err[STEEL_CLEAN_MAX_MSG];
        if (dry_run)
        {
            steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, path, true, "dry-run");
            (*removed_files)++;
            return true;
        }
        bool ok = steel_remove_file(path, err, sizeof(err));
        steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, path, ok, ok ? "" : "%s", ok ? "" : err);
        if (ok) (*removed_files)++; else (*errors)++;
        return ok;
    }

    if (!steel_is_directory(path))
    {
        char err[STEEL_CLEAN_MAX_MSG];
        if (dry_run)
        {
            steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, path, true, "dry-run");
            (*removed_files)++;
            return true;
        }
        bool ok = steel_remove_file(path, err, sizeof(err));
        steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, path, ok, ok ? "" : "%s", ok ? "" : err);
        if (ok) (*removed_files)++; else (*errors)++;
        return ok;
    }

#if defined(_WIN32)
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);

    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    if (h == INVALID_HANDLE_VALUE)
    {
        // Might be empty or inaccessible.
        char err[STEEL_CLEAN_MAX_MSG];
        if (dry_run)
        {
            steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, path, true, "dry-run");
            (*removed_dirs)++;
            return true;
        }
        bool ok = steel_remove_empty_dir(path, err, sizeof(err));
        steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, path, ok, ok ? "" : "%s", ok ? "" : err);
        if (ok) (*removed_dirs)++; else (*errors)++;
        return ok;
    }

    do
    {
        const char* name = ffd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char child[MAX_PATH];
        steel_path_join(child, sizeof(child), path, name);

        // Avoid recursing into reparse points (symlink/junction)
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // treat as directory to remove as-is
                char err[STEEL_CLEAN_MAX_MSG];
                if (dry_run)
                {
                    steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, child, true, "dry-run");
                    (*removed_dirs)++;
                }
                else
                {
                    bool ok = steel_remove_empty_dir(child, err, sizeof(err));
                    steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, child, ok, ok ? "" : "%s", ok ? "" : err);
                    if (ok) (*removed_dirs)++; else (*errors)++;
                }
            }
            else
            {
                char err[STEEL_CLEAN_MAX_MSG];
                if (dry_run)
                {
                    steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, child, true, "dry-run");
                    (*removed_files)++;
                }
                else
                {
                    bool ok = steel_remove_file(child, err, sizeof(err));
                    steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, child, ok, ok ? "" : "%s", ok ? "" : err);
                    if (ok) (*removed_files)++; else (*errors)++;
                }
            }
            continue;
        }

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            (void)steel_remove_tree(child, dry_run, ops, removed_files, removed_dirs, errors);
        }
        else
        {
            char err[STEEL_CLEAN_MAX_MSG];
            if (dry_run)
            {
                steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, child, true, "dry-run");
                (*removed_files)++;
            }
            else
            {
                bool ok = steel_remove_file(child, err, sizeof(err));
                steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, child, ok, ok ? "" : "%s", ok ? "" : err);
                if (ok) (*removed_files)++; else (*errors)++;
            }
        }

    } while (FindNextFileA(h, &ffd));

    FindClose(h);

    {
        char err[STEEL_CLEAN_MAX_MSG];
        if (dry_run)
        {
            steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, path, true, "dry-run");
            (*removed_dirs)++;
            return true;
        }
        bool ok = steel_remove_empty_dir(path, err, sizeof(err));
        steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, path, ok, ok ? "" : "%s", ok ? "" : err);
        if (ok) (*removed_dirs)++; else (*errors)++;
        return ok;
    }
#else
    DIR* d = opendir(path);
    if (!d)
    {
        char err[STEEL_CLEAN_MAX_MSG];
        if (dry_run)
        {
            steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, path, true, "dry-run");
            (*removed_dirs)++;
            return true;
        }
        bool ok = steel_remove_empty_dir(path, err, sizeof(err));
        steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, path, ok, ok ? "" : "%s", ok ? "" : err);
        if (ok) (*removed_dirs)++; else (*errors)++;
        return ok;
    }

    for (;;)
    {
        struct dirent* ent = readdir(d);
        if (!ent) break;

        const char* name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char child[STEEL_CLEAN_PATH_MAX];
        steel_path_join(child, sizeof(child), path, name);

        // Never recurse into symlinked directories.
        if (steel_is_symlink(child))
        {
            // unlink symlink (as file)
            char err[STEEL_CLEAN_MAX_MSG];
            if (dry_run)
            {
                steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, child, true, "dry-run");
                (*removed_files)++;
            }
            else
            {
                bool ok = steel_remove_file(child, err, sizeof(err));
                steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, child, ok, ok ? "" : "%s", ok ? "" : err);
                if (ok) (*removed_files)++; else (*errors)++;
            }
            continue;
        }

        if (steel_is_directory(child))
        {
            (void)steel_remove_tree(child, dry_run, ops, removed_files, removed_dirs, errors);
        }
        else
        {
            char err[STEEL_CLEAN_MAX_MSG];
            if (dry_run)
            {
                steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, child, true, "dry-run");
                (*removed_files)++;
            }
            else
            {
                bool ok = steel_remove_file(child, err, sizeof(err));
                steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_FILE, child, ok, ok ? "" : "%s", ok ? "" : err);
                if (ok) (*removed_files)++; else (*errors)++;
            }
        }
    }

    closedir(d);

    {
        char err[STEEL_CLEAN_MAX_MSG];
        if (dry_run)
        {
            steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, path, true, "dry-run");
            (*removed_dirs)++;
            return true;
        }
        bool ok = steel_remove_empty_dir(path, err, sizeof(err));
        steel_clean_opvec_add(ops, STEEL_CLEAN_REMOVE_DIR, path, ok, ok ? "" : "%s", ok ? "" : err);
        if (ok) (*removed_dirs)++; else (*errors)++;
        return ok;
    }
#endif
}

static bool steel_should_skip_dir_name(const char* name)
{
    // Avoid scanning huge/vendor dirs for `--all` file cleanup.
    return (strcmp(name, ".git") == 0)
        || (strcmp(name, ".hg") == 0)
        || (strcmp(name, ".svn") == 0)
        || (strcmp(name, "node_modules") == 0)
        || (strcmp(name, ".idea") == 0);
}

static bool steel_is_cleanable_ext(const char* path)
{
    // Extensions removed when --all is used.
    return steel_ends_with(path, ".o")
        || steel_ends_with(path, ".obj")
        || steel_ends_with(path, ".a")
        || steel_ends_with(path, ".lib")
        || steel_ends_with(path, ".so")
        || steel_ends_with(path, ".dylib")
        || steel_ends_with(path, ".dll")
        || steel_ends_with(path, ".exe")
        || steel_ends_with(path, ".pdb")
        || steel_ends_with(path, ".ilk")
        || steel_ends_with(path, ".d")
        || steel_ends_with(path, ".tmp")
        || steel_ends_with(path, ".log");
}

static void steel_collect_all_files_recursive(const char* root, steel_strvec* out)
{
    if (!steel_path_exists(root)) return;

    if (!steel_is_directory(root))
    {
        if (steel_is_cleanable_ext(root)) steel_strvec_push(out, root);
        return;
    }

#if defined(_WIN32)
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", root);

    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    if (h == INVALID_HANDLE_VALUE) return;

    do
    {
        const char* name = ffd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (steel_should_skip_dir_name(name)) continue;
        }

        char child[MAX_PATH];
        steel_path_join(child, sizeof(child), root, name);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // avoid junction loops
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
            steel_collect_all_files_recursive(child, out);
        }
        else
        {
            if (steel_is_cleanable_ext(child)) steel_strvec_push(out, child);
        }

    } while (FindNextFileA(h, &ffd));

    FindClose(h);
#else
    DIR* d = opendir(root);
    if (!d) return;

    for (;;)
    {
        struct dirent* ent = readdir(d);
        if (!ent) break;

        const char* name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        if (steel_should_skip_dir_name(name)) continue;

        char child[STEEL_CLEAN_PATH_MAX];
        steel_path_join(child, sizeof(child), root, name);

        if (steel_is_symlink(child))
        {
            // Don't follow.
            if (!steel_is_directory(child) && steel_is_cleanable_ext(child)) steel_strvec_push(out, child);
            continue;
        }

        if (steel_is_directory(child))
        {
            steel_collect_all_files_recursive(child, out);
        }
        else
        {
            if (steel_is_cleanable_ext(child)) steel_strvec_push(out, child);
        }
    }

    closedir(d);
#endif
}

//------------------------------------------------------------------------------
// Output formatting
//------------------------------------------------------------------------------

static void steel_json_escape(FILE* out, const char* s)
{
    fputc('"', out);
    for (const unsigned char* p = (const unsigned char*)s; *p; p++)
    {
        unsigned char c = *p;
        switch (c)
        {
            case '"': fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\b': fputs("\\b", out); break;
            case '\f': fputs("\\f", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if (c < 0x20)
                    fprintf(out, "\\u%04x", (unsigned)c);
                else
                    fputc((int)c, out);
                break;
        }
    }
    fputc('"', out);
}

static void steel_print_help(FILE* out)
{
    fprintf(out,
        "Usage: steel clean [options] [roots...]\n"
        "\n"
        "Removes common generated directories (build/cache/tmp/dist) under roots.\n"
        "If no root is provided, defaults to current directory (.).\n"
        "\n"
        "Options:\n"
        "  -h, --help        Show this help\n"
        "  -n, --dry-run     Do not delete, only report\n"
        "  -q, --quiet       Only print errors (text mode)\n"
        "  -v, --verbose     Print each removed path (text mode)\n"
        "      --json        Emit JSON report to stdout\n"
        "      --force       Continue on errors (still returns non-zero)\n"
        "      --build       Only remove build/output directories\n"
        "      --cache       Only remove cache directories\n"
        "      --dist        Only remove dist/release directories\n"
        "      --tmp         Only remove temp directories\n"
        "      --all         Additionally remove common object/binary/temp files recursively\n"
        "\n"
        "Exit codes:\n"
        "  0  Success\n"
        "  1  Errors occurred\n"
        "  2  Invalid usage\n");
}

//------------------------------------------------------------------------------
// Command implementation
//------------------------------------------------------------------------------

typedef struct steel_clean_opts
{
    bool dry_run;
    bool verbose;
    bool quiet;
    bool json;
    bool force;
    bool all;

    bool sel_build;
    bool sel_cache;
    bool sel_dist;
    bool sel_tmp;
} steel_clean_opts;

static bool steel_any_selection(const steel_clean_opts* o)
{
    return o->sel_build || o->sel_cache || o->sel_dist || o->sel_tmp;
}

static void steel_collect_known_dirs(const char* root, const steel_clean_opts* opt, steel_strvec* targets)
{
    // Build-related
    static const char* build_dirs[] = {
        "build", "out", "_build", "cmake-build-debug", "cmake-build-release", "target", NULL
    };

    // Cache-related
    static const char* cache_dirs[] = {
        ".cache", "cache", ".muffin-cache", ".vitte-cache", ".steel-cache", NULL
    };

    // Dist/release
    static const char* dist_dirs[] = {
        "dist", "release", "pkg", "packages", NULL
    };

    // Temp
    static const char* tmp_dirs[] = {
        "tmp", ".tmp", "temp", ".temp", NULL
    };

    bool any = steel_any_selection(opt);
    bool do_build = any ? opt->sel_build : true;
    bool do_cache = any ? opt->sel_cache : true;
    bool do_dist  = any ? opt->sel_dist  : true;
    bool do_tmp   = any ? opt->sel_tmp   : true;

    if (steel_path_exists(root))
    {
        if (!steel_is_directory(root))
        {
            // Root is a file: only consider it if --all and extension matches.
            if (opt->all && steel_is_cleanable_ext(root)) steel_strvec_push(targets, root);
            return;
        }

        // Root itself may be one of these dirs; if the user points at it explicitly, allow removal.
        const char* bn = steel_basename(root);
        if (bn && bn[0])
        {
            for (const char** p = build_dirs; do_build && *p; p++) if (strcmp(bn, *p) == 0) { steel_strvec_push(targets, root); return; }
            for (const char** p = cache_dirs; do_cache && *p; p++) if (strcmp(bn, *p) == 0) { steel_strvec_push(targets, root); return; }
            for (const char** p = dist_dirs;  do_dist  && *p; p++) if (strcmp(bn, *p) == 0) { steel_strvec_push(targets, root); return; }
            for (const char** p = tmp_dirs;   do_tmp   && *p; p++) if (strcmp(bn, *p) == 0) { steel_strvec_push(targets, root); return; }
        }

        char buf[STEEL_CLEAN_PATH_MAX];

        if (do_build)
        {
            for (const char** p = build_dirs; *p; p++)
            {
                steel_path_join(buf, sizeof(buf), root, *p);
                if (steel_is_directory(buf)) steel_strvec_push(targets, buf);
            }
        }

        if (do_cache)
        {
            for (const char** p = cache_dirs; *p; p++)
            {
                steel_path_join(buf, sizeof(buf), root, *p);
                if (steel_is_directory(buf)) steel_strvec_push(targets, buf);
            }
        }

        if (do_dist)
        {
            for (const char** p = dist_dirs; *p; p++)
            {
                steel_path_join(buf, sizeof(buf), root, *p);
                if (steel_is_directory(buf)) steel_strvec_push(targets, buf);
            }
        }

        if (do_tmp)
        {
            for (const char** p = tmp_dirs; *p; p++)
            {
                steel_path_join(buf, sizeof(buf), root, *p);
                if (steel_is_directory(buf)) steel_strvec_push(targets, buf);
            }
        }
    }
}

static int steel_cmd_clean_impl(int argc, char** argv)
{
    steel_clean_opts opt;
    memset(&opt, 0, sizeof(opt));

    steel_strvec roots;
    steel_strvec_init(&roots);

    // Parse args
    for (int i = 1; i < argc; i++)
    {
        const char* a = argv[i];
        if (!a) continue;

        if (strcmp(a, "--") == 0)
        {
            for (int j = i + 1; j < argc; j++) steel_strvec_push(&roots, argv[j]);
            break;
        }

        if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0)
        {
            steel_print_help(stdout);
            steel_strvec_free(&roots);
            return 0;
        }

        if (strcmp(a, "-n") == 0 || strcmp(a, "--dry-run") == 0)
        {
            opt.dry_run = true;
            continue;
        }

        if (strcmp(a, "-q") == 0 || strcmp(a, "--quiet") == 0)
        {
            opt.quiet = true;
            continue;
        }

        if (strcmp(a, "-v") == 0 || strcmp(a, "--verbose") == 0)
        {
            opt.verbose = true;
            continue;
        }

        if (strcmp(a, "--json") == 0)
        {
            opt.json = true;
            continue;
        }

        if (strcmp(a, "--force") == 0)
        {
            opt.force = true;
            continue;
        }

        if (strcmp(a, "--all") == 0)
        {
            opt.all = true;
            continue;
        }

        if (strcmp(a, "--build") == 0)
        {
            opt.sel_build = true;
            continue;
        }

        if (strcmp(a, "--cache") == 0)
        {
            opt.sel_cache = true;
            continue;
        }

        if (strcmp(a, "--dist") == 0)
        {
            opt.sel_dist = true;
            continue;
        }

        if (strcmp(a, "--tmp") == 0)
        {
            opt.sel_tmp = true;
            continue;
        }

        if (a[0] == '-')
        {
            fprintf(stderr, "steel clean: unknown option: %s\n", a);
            steel_print_help(stderr);
            steel_strvec_free(&roots);
            return 2;
        }

        steel_strvec_push(&roots, a);
    }

    if (roots.len == 0) steel_strvec_push(&roots, ".");

    // Collect targets
    steel_strvec targets;
    steel_strvec_init(&targets);

    for (size_t i = 0; i < roots.len; i++)
    {
        steel_collect_known_dirs(roots.items[i], &opt, &targets);
    }

    // Optional recursive file cleanup
    steel_strvec extra_files;
    steel_strvec_init(&extra_files);
    if (opt.all)
    {
        for (size_t i = 0; i < roots.len; i++)
        {
            steel_collect_all_files_recursive(roots.items[i], &extra_files);
        }
    }

    steel_clean_opvec ops;
    steel_clean_opvec_init(&ops);

    size_t removed_files = 0;
    size_t removed_dirs = 0;
    size_t errors = 0;

    // Remove directories first (tree delete)
    for (size_t i = 0; i < targets.len; i++)
    {
        (void)steel_remove_tree(targets.items[i], opt.dry_run, &ops, &removed_files, &removed_dirs, &errors);
        if (errors && !opt.force) break;
    }

    // Then remove extra files
    if ((!errors || opt.force) && opt.all)
    {
        for (size_t i = 0; i < extra_files.len; i++)
        {
            const char* p = extra_files.items[i];
            if (!steel_path_exists(p)) continue;

            char err[STEEL_CLEAN_MAX_MSG];
            if (opt.dry_run)
            {
                steel_clean_opvec_add(&ops, STEEL_CLEAN_REMOVE_FILE, p, true, "dry-run");
                removed_files++;
                continue;
            }

            bool ok = steel_remove_file(p, err, sizeof(err));
            steel_clean_opvec_add(&ops, STEEL_CLEAN_REMOVE_FILE, p, ok, ok ? "" : "%s", ok ? "" : err);
            if (ok) removed_files++; else errors++;
            if (errors && !opt.force) break;
        }
    }

    // Output
    if (opt.json)
    {
        FILE* out = stdout;
        fputs("{\n", out);
        fputs("  \"command\": \"clean\",\n", out);
        fputs("  \"options\": {\n", out);
        fprintf(out, "    \"dry_run\": %s,\n", opt.dry_run ? "true" : "false");
        fprintf(out, "    \"force\": %s,\n", opt.force ? "true" : "false");
        fprintf(out, "    \"all\": %s\n", opt.all ? "true" : "false");
        fputs("  },\n", out);
        fputs("  \"summary\": {\n", out);
        fprintf(out, "    \"removed_files\": %zu,\n", removed_files);
        fprintf(out, "    \"removed_dirs\": %zu,\n", removed_dirs);
        fprintf(out, "    \"errors\": %zu\n", errors);
        fputs("  },\n", out);
        fputs("  \"operations\": [\n", out);

        for (size_t i = 0; i < ops.len; i++)
        {
            const steel_clean_op* o = &ops.items[i];
            fputs("    { ", out);
            fputs("\"action\": ", out);
            steel_json_escape(out, (o->action == STEEL_CLEAN_REMOVE_DIR) ? "remove_dir" : "remove_file");
            fputs(", \"path\": ", out);
            steel_json_escape(out, o->path);
            fputs(", \"ok\": ", out);
            fputs(o->ok ? "true" : "false", out);
            if (o->msg[0])
            {
                fputs(", \"message\": ", out);
                steel_json_escape(out, o->msg);
            }
            fputs(" }", out);
            if (i + 1 < ops.len) fputs(",", out);
            fputs("\n", out);
        }

        fputs("  ]\n", out);
        fputs("}\n", out);
    }
    else
    {
        if (!opt.quiet)
        {
            if (opt.dry_run) printf("DRY-RUN (no changes)\n\n");
        }

        for (size_t i = 0; i < ops.len; i++)
        {
            const steel_clean_op* o = &ops.items[i];
            if (opt.quiet && o->ok) continue;

            if (opt.verbose || !o->ok)
            {
                const char* act = (o->action == STEEL_CLEAN_REMOVE_DIR) ? "DIR " : "FILE";
                const char* st  = o->ok ? "OK" : "FAIL";
                if (o->msg[0])
                    printf("%s %s %s (%s)\n", st, act, o->path, o->msg);
                else
                    printf("%s %s %s\n", st, act, o->path);
            }
        }

        if (!opt.quiet)
        {
            printf("\nSummary: removed_files=%zu removed_dirs=%zu errors=%zu\n", removed_files, removed_dirs, errors);
        }
    }

    steel_clean_opvec_free(&ops);
    steel_strvec_free(&extra_files);
    steel_strvec_free(&targets);
    steel_strvec_free(&roots);

    return errors ? 1 : 0;
}

// Public entry points
int steel_cmd_clean(int argc, char** argv)
{
    return steel_cmd_clean_impl(argc, argv);
}

int steel_cmd_clean_main(int argc, char** argv)
{
    return steel_cmd_clean_impl(argc, argv);
}

