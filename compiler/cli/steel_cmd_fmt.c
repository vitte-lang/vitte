// SPDX-License-Identifier: MIT
// steel_cmd_fmt.c
//
// `steel fmt` command implementation.
//
// Scope:
//  - Early-bootstrap friendly formatter driver.
//  - Without a full parser, this performs conservative whitespace normalization:
//      * Normalize line endings to \n (when rewriting).
//      * Trim trailing whitespace.
//      * Ensure file ends with a newline (POSIX style).
//      * Optionally convert leading tabs to spaces (indent step).
//  - Supports check-only mode suitable for CI.
//  - Supports recursive directory scan.
//
// Notes:
//  - This is not a language-aware formatter; it is a hygiene tool.

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

#ifndef STEEL_FMT_PATH_MAX
  #define STEEL_FMT_PATH_MAX 4096
#endif

#ifndef STEEL_FMT_IO_CHUNK
  #define STEEL_FMT_IO_CHUNK (64u * 1024u)
#endif

//------------------------------------------------------------------------------
// Utilities
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

static bool steel_is_regular_file(const char* path)
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

static bool steel_is_directory(const char* path)
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

#if !defined(_WIN32)
static bool steel_is_symlink(const char* path)
{
    struct stat st;
    if (lstat(path, &st) != 0) return false;
    return S_ISLNK(st.st_mode);
}
#endif

static bool steel_should_skip_dir(const char* name)
{
    return (strcmp(name, ".git") == 0)
        || (strcmp(name, ".hg") == 0)
        || (strcmp(name, ".svn") == 0)
        || (strcmp(name, "node_modules") == 0)
        || (strcmp(name, ".idea") == 0)
        || (strcmp(name, "dist") == 0)
        || (strcmp(name, "build") == 0)
        || (strcmp(name, "out") == 0)
        || (strcmp(name, "target") == 0);
}

static bool steel_should_format_file(const char* path)
{
    // Keep conservative; extend if needed.
    return steel_ends_with(path, ".vitte")
        || steel_ends_with(path, ".vit")
        || steel_ends_with(path, ".vitl")
        || steel_ends_with(path, ".muf")
        || steel_ends_with(path, ".md")
        || steel_ends_with(path, ".toml")
        || steel_ends_with(path, ".json")
        || steel_ends_with(path, ".c")
        || steel_ends_with(path, ".h");
}

//------------------------------------------------------------------------------
// Dynamic vector of strings
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
        v->cap = (v->cap == 0) ? 64 : (v->cap * 2);
        v->items = (char**)steel_xrealloc(v->items, v->cap * sizeof(char*));
    }
    v->items[v->len++] = steel_xstrdup(s);
}

//------------------------------------------------------------------------------
// File collection
//------------------------------------------------------------------------------

static void steel_collect_files_recursive(const char* root, steel_strvec* out)
{
    if (steel_is_regular_file(root))
    {
        if (steel_should_format_file(root)) steel_strvec_push(out, root);
        return;
    }

    if (!steel_is_directory(root)) return;

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
            if (steel_should_skip_dir(name)) continue;
        }

        char child[MAX_PATH];
        steel_path_join(child, sizeof(child), root, name);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
            steel_collect_files_recursive(child, out);
        }
        else
        {
            if (steel_should_format_file(child)) steel_strvec_push(out, child);
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
        if (steel_should_skip_dir(name)) continue;

        char child[STEEL_FMT_PATH_MAX];
        steel_path_join(child, sizeof(child), root, name);

        if (steel_is_symlink(child))
        {
            // Never follow; format the symlink target is a separate concern.
            continue;
        }

        if (steel_is_directory(child))
        {
            steel_collect_files_recursive(child, out);
        }
        else if (steel_is_regular_file(child))
        {
            if (steel_should_format_file(child)) steel_strvec_push(out, child);
        }
    }

    closedir(d);
#endif
}

//------------------------------------------------------------------------------
// Formatting
//------------------------------------------------------------------------------

typedef struct steel_fmt_opts
{
    bool check_only;     // do not modify, only report
    bool write;          // rewrite files (default true unless check_only)
    bool quiet;          // minimal output
    bool verbose;        // per-file output
    bool convert_tabs;   // convert leading tabs to spaces
    uint32_t tab_width;  // spaces per leading tab
} steel_fmt_opts;

static void steel_buf_push(uint8_t** buf, size_t* len, size_t* cap, const void* data, size_t n)
{
    if (*len + n > *cap)
    {
        size_t new_cap = (*cap == 0) ? 4096 : *cap;
        while (*len + n > new_cap) new_cap *= 2;
        *buf = (uint8_t*)steel_xrealloc(*buf, new_cap);
        *cap = new_cap;
    }
    memcpy(*buf + *len, data, n);
    *len += n;
}

static bool steel_read_file(const char* path, uint8_t** out_data, size_t* out_len, char* err, size_t err_cap)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        snprintf(err, err_cap, "cannot open: %s", strerror(errno));
        return false;
    }

    uint8_t* data = NULL;
    size_t len = 0;
    size_t cap = 0;

    uint8_t tmp[STEEL_FMT_IO_CHUNK];
    for (;;)
    {
        size_t n = fread(tmp, 1, sizeof(tmp), f);
        if (n == 0)
        {
            if (ferror(f))
            {
                snprintf(err, err_cap, "read error: %s", strerror(errno));
                fclose(f);
                free(data);
                return false;
            }
            break;
        }
        steel_buf_push(&data, &len, &cap, tmp, n);
    }

    fclose(f);
    *out_data = data;
    *out_len = len;
    return true;
}

static bool steel_write_file_atomic(const char* path, const uint8_t* data, size_t len, char* err, size_t err_cap)
{
    // Write to temp in same directory then rename.
    // Best-effort atomic on POSIX; on Windows, MoveFileEx is used.
    char tmp_path[STEEL_FMT_PATH_MAX];
    snprintf(tmp_path, sizeof(tmp_path), "%s.steel_tmp", path);

    FILE* f = fopen(tmp_path, "wb");
    if (!f)
    {
        snprintf(err, err_cap, "cannot write temp: %s", strerror(errno));
        return false;
    }

    if (len > 0)
    {
        if (fwrite(data, 1, len, f) != len)
        {
            snprintf(err, err_cap, "write error: %s", strerror(errno));
            fclose(f);
            (void)remove(tmp_path);
            return false;
        }
    }

    if (fclose(f) != 0)
    {
        snprintf(err, err_cap, "flush error: %s", strerror(errno));
        (void)remove(tmp_path);
        return false;
    }

#if defined(_WIN32)
    if (!MoveFileExA(tmp_path, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DWORD e = GetLastError();
        snprintf(err, err_cap, "rename failed (winerr=%lu)", (unsigned long)e);
        (void)remove(tmp_path);
        return false;
    }
#else
    if (rename(tmp_path, path) != 0)
    {
        snprintf(err, err_cap, "rename failed: %s", strerror(errno));
        (void)remove(tmp_path);
        return false;
    }
#endif

    return true;
}

static void steel_format_buffer(const steel_fmt_opts* opt, const uint8_t* in, size_t in_len, uint8_t** out, size_t* out_len)
{
    // Streaming-like transform, but easiest to just build an output buffer.
    uint8_t* buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    bool at_line_start = true;
    bool pending_cr = false;

    // Track trailing whitespace for current line: keep indices of last non-ws.
    // We'll flush line content without trailing ws at newline.
    uint8_t* line = NULL;
    size_t line_len = 0;
    size_t line_cap = 0;

    // Lambda-like helpers (C99 style)
    #define PUSH_LINE(data, n) do { \
        if (line_len + (n) > line_cap) { \
            size_t nc = (line_cap == 0) ? 256 : line_cap; \
            while (line_len + (n) > nc) nc *= 2; \
            line = (uint8_t*)steel_xrealloc(line, nc); \
            line_cap = nc; \
        } \
        memcpy(line + line_len, (data), (n)); \
        line_len += (n); \
    } while (0)

    #define FLUSH_LINE_NO_TRAILING_WS() do { \
        size_t end = line_len; \
        while (end > 0 && (line[end - 1] == ' ' || line[end - 1] == '\t')) end--; \
        if (end > 0) steel_buf_push(&buf, &len, &cap, line, end); \
        line_len = 0; \
    } while (0)

    for (size_t i = 0; i < in_len; i++)
    {
        uint8_t c = in[i];

        // Normalize CRLF/CR to LF
        if (pending_cr)
        {
            pending_cr = false;
            if (c == '\n')
            {
                // already handled as newline; skip this LF
                continue;
            }
            // CR not followed by LF: treat CR as newline and continue processing current char
            FLUSH_LINE_NO_TRAILING_WS();
            uint8_t nl = '\n';
            steel_buf_push(&buf, &len, &cap, &nl, 1);
            at_line_start = true;
        }

        if (c == '\r')
        {
            pending_cr = true;
            continue;
        }

        if (c == '\n')
        {
            FLUSH_LINE_NO_TRAILING_WS();
            uint8_t nl = '\n';
            steel_buf_push(&buf, &len, &cap, &nl, 1);
            at_line_start = true;
            continue;
        }

        if (at_line_start && opt->convert_tabs)
        {
            // Convert leading tabs only (indentation)
            if (c == '\t')
            {
                for (uint32_t k = 0; k < opt->tab_width; k++)
                {
                    uint8_t sp = ' ';
                    PUSH_LINE(&sp, 1);
                }
                continue;
            }
            else if (c == ' ')
            {
                PUSH_LINE(&c, 1);
                continue;
            }
            else
            {
                at_line_start = false;
            }
        }
        else if (at_line_start)
        {
            if (c != ' ' && c != '\t') at_line_start = false;
        }

        PUSH_LINE(&c, 1);
    }

    if (pending_cr)
    {
        // file ended with CR
        FLUSH_LINE_NO_TRAILING_WS();
        uint8_t nl = '\n';
        steel_buf_push(&buf, &len, &cap, &nl, 1);
        at_line_start = true;
    }

    // Flush last line (trim trailing ws) without forcing newline yet
    if (line_len > 0)
    {
        FLUSH_LINE_NO_TRAILING_WS();
    }

    free(line);

    // Ensure file ends with exactly one newline (unless empty)
    if (len > 0)
    {
        if (buf[len - 1] != '\n')
        {
            uint8_t nl = '\n';
            steel_buf_push(&buf, &len, &cap, &nl, 1);
        }
    }

    *out = buf;
    *out_len = len;

    #undef PUSH_LINE
    #undef FLUSH_LINE_NO_TRAILING_WS
}

static bool steel_buffers_equal(const uint8_t* a, size_t an, const uint8_t* b, size_t bn)
{
    if (an != bn) return false;
    if (an == 0) return true;
    return memcmp(a, b, an) == 0;
}

static int steel_format_file(const char* path, const steel_fmt_opts* opt, size_t* changed, size_t* errors)
{
    char err[256];
    uint8_t* in = NULL;
    size_t in_len = 0;

    if (!steel_read_file(path, &in, &in_len, err, sizeof(err)))
    {
        if (!opt->quiet) fprintf(stderr, "steel fmt: %s: %s\n", path, err);
        (*errors)++;
        return 1;
    }

    uint8_t* out = NULL;
    size_t out_len = 0;
    steel_format_buffer(opt, in, in_len, &out, &out_len);

    bool same = steel_buffers_equal(in, in_len, out, out_len);

    if (!same)
    {
        (*changed)++;
        if (opt->check_only)
        {
            if (opt->verbose && !opt->quiet)
                printf("DIFF %s\n", path);
        }
        else
        {
            if (!steel_write_file_atomic(path, out, out_len, err, sizeof(err)))
            {
                if (!opt->quiet) fprintf(stderr, "steel fmt: %s: %s\n", path, err);
                (*errors)++;
                free(in);
                free(out);
                return 1;
            }
            if (opt->verbose && !opt->quiet)
                printf("FMT  %s\n", path);
        }
    }
    else
    {
        if (opt->verbose && !opt->quiet)
            printf("OK   %s\n", path);
    }

    free(in);
    free(out);
    return 0;
}

//------------------------------------------------------------------------------
// CLI
//------------------------------------------------------------------------------

static void steel_print_help(FILE* out)
{
    fprintf(out,
        "Usage: steel fmt [options] [paths...]\n"
        "\n"
        "Performs conservative whitespace formatting (hygiene) on sources/manifests.\n"
        "If a path is a directory, it is scanned recursively.\n"
        "\n"
        "Options:\n"
        "  -h, --help          Show this help\n"
        "  -c, --check         Check only; do not modify files (CI mode)\n"
        "  -q, --quiet         Reduce output (errors only)\n"
        "  -v, --verbose       Print per-file status\n"
        "      --tabs <n>      Convert leading tabs to spaces (n spaces, default 4)\n"
        "      --no-tabs       Do not convert tabs (default)\n"
        "\n"
        "Exit codes:\n"
        "  0  Success (or no diffs in --check)\n"
        "  1  Diffs found in --check or errors in write mode\n"
        "  2  Invalid usage\n");
}

static int steel_cmd_fmt_impl(int argc, char** argv)
{
    steel_fmt_opts opt;
    opt.check_only = false;
    opt.write = true;
    opt.quiet = false;
    opt.verbose = false;
    opt.convert_tabs = false;
    opt.tab_width = 4;

    steel_strvec roots;
    steel_strvec_init(&roots);

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

        if (strcmp(a, "-c") == 0 || strcmp(a, "--check") == 0)
        {
            opt.check_only = true;
            opt.write = false;
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

        if (strcmp(a, "--no-tabs") == 0)
        {
            opt.convert_tabs = false;
            continue;
        }

        if (strcmp(a, "--tabs") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel fmt: missing value after --tabs\n");
                steel_print_help(stderr);
                steel_strvec_free(&roots);
                return 2;
            }
            opt.convert_tabs = true;
            opt.tab_width = (uint32_t)strtoul(argv[++i], NULL, 10);
            if (opt.tab_width == 0) opt.tab_width = 4;
            continue;
        }

        if (a[0] == '-')
        {
            fprintf(stderr, "steel fmt: unknown option: %s\n", a);
            steel_print_help(stderr);
            steel_strvec_free(&roots);
            return 2;
        }

        steel_strvec_push(&roots, a);
    }

    if (roots.len == 0) steel_strvec_push(&roots, ".");

    // Collect files
    steel_strvec files;
    steel_strvec_init(&files);

    for (size_t i = 0; i < roots.len; i++)
        steel_collect_files_recursive(roots.items[i], &files);

    size_t changed = 0;
    size_t errors = 0;

    for (size_t i = 0; i < files.len; i++)
        (void)steel_format_file(files.items[i], &opt, &changed, &errors);

    if (!opt.quiet)
    {
        if (opt.check_only)
            printf("Summary: files=%zu diffs=%zu errors=%zu\n", files.len, changed, errors);
        else
            printf("Summary: files=%zu formatted=%zu errors=%zu\n", files.len, changed, errors);
    }

    steel_strvec_free(&files);
    steel_strvec_free(&roots);

    if (errors) return 1;
    if (opt.check_only && changed) return 1;
    return 0;
}

// Public entry points
int steel_cmd_fmt(int argc, char** argv)
{
    return steel_cmd_fmt_impl(argc, argv);
}

int steel_cmd_fmt_main(int argc, char** argv)
{
    return steel_cmd_fmt_impl(argc, argv);
}

