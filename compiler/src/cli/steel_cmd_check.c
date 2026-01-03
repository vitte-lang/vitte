// SPDX-License-Identifier: MIT
// steel_cmd_check.c
//
// `steel check` command implementation.
//
// Scope:
//  - Provide a robust CLI subcommand that can be wired into the Steel/Vitte CLI.
//  - Perform lightweight static checks on source/manifest text files (readability,
//    NUL bytes, UTF-8 validity, trailing whitespace, tabs, long lines).
//  - Optionally scan directories recursively.
//  - Emit either human-readable text or JSON.
//
// NOTE: This file is intentionally self-contained (stdlib + minimal platform
// traversal) so it can compile early in bootstrap stages.

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
#else
  #include <sys/stat.h>
  #include <dirent.h>
  #include <unistd.h>
#endif

#ifndef STEEL_CHECK_MAX_MSG
  #define STEEL_CHECK_MAX_MSG 512
#endif

#ifndef STEEL_CHECK_MAX_LINE
  #define STEEL_CHECK_MAX_LINE 240
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

static bool steel_starts_with(const char* s, const char* pfx)
{
    size_t a = strlen(pfx);
    return strncmp(s, pfx, a) == 0;
}

static bool steel_ends_with(const char* s, const char* sfx)
{
    size_t n = strlen(s);
    size_t m = strlen(sfx);
    if (m > n) return false;
    return memcmp(s + (n - m), sfx, m) == 0;
}

static const char* steel_basename(const char* path)
{
    const char* a = strrchr(path, '/');
    const char* b = strrchr(path, '\\');
    const char* p = a;
    if (b && (!p || b > p)) p = b;
    return p ? p + 1 : path;
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
// Diagnostics
//------------------------------------------------------------------------------

typedef enum steel_diag_level
{
    STEEL_DIAG_WARNING = 1,
    STEEL_DIAG_ERROR = 2
} steel_diag_level;

typedef struct steel_diag
{
    steel_diag_level level;
    char msg[STEEL_CHECK_MAX_MSG];
} steel_diag;

typedef struct steel_diagvec
{
    steel_diag* items;
    size_t len;
    size_t cap;
} steel_diagvec;

static void steel_diagvec_init(steel_diagvec* v)
{
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

static void steel_diagvec_free(steel_diagvec* v)
{
    free(v->items);
    steel_diagvec_init(v);
}

static void steel_diagvec_add(steel_diagvec* v, steel_diag_level lvl, const char* fmt, ...)
{
    if (v->len + 1 > v->cap)
    {
        v->cap = (v->cap == 0) ? 16 : (v->cap * 2);
        v->items = (steel_diag*)steel_xrealloc(v->items, v->cap * sizeof(steel_diag));
    }

    steel_diag* d = &v->items[v->len++];
    d->level = lvl;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(d->msg, sizeof(d->msg), fmt, ap);
    va_end(ap);
}

//------------------------------------------------------------------------------
// File & directory helpers
//------------------------------------------------------------------------------

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

static bool steel_should_check_file(const char* path)
{
    // Conservative list: source + manifests.
    // Extend locally if your workspace uses additional extensions.
    return steel_ends_with(path, ".vitte")
        || steel_ends_with(path, ".vit")
        || steel_ends_with(path, ".vitl")
        || steel_ends_with(path, ".muf")
        || steel_ends_with(path, ".toml")
        || steel_ends_with(path, ".json")
        || steel_ends_with(path, ".md");
}

static void steel_collect_files_recursive(const char* root, steel_strvec* out)
{
    if (steel_is_regular_file(root))
    {
        if (steel_should_check_file(root)) steel_strvec_push(out, root);
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

        char child[MAX_PATH];
        steel_path_join(child, sizeof(child), root, name);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            steel_collect_files_recursive(child, out);
        }
        else
        {
            if (steel_should_check_file(child)) steel_strvec_push(out, child);
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

        char child[4096];
        steel_path_join(child, sizeof(child), root, name);

        // Use d_type if available, but fall back to stat.
        bool is_dir = false;
        bool is_reg = false;

    #if defined(DT_DIR) && defined(DT_REG)
        if (ent->d_type == DT_DIR) is_dir = true;
        else if (ent->d_type == DT_REG) is_reg = true;
        else
        {
            is_dir = steel_is_directory(child);
            is_reg = steel_is_regular_file(child);
        }
    #else
        is_dir = steel_is_directory(child);
        is_reg = steel_is_regular_file(child);
    #endif

        if (is_dir)
        {
            steel_collect_files_recursive(child, out);
        }
        else if (is_reg)
        {
            if (steel_should_check_file(child)) steel_strvec_push(out, child);
        }
    }

    closedir(d);
#endif
}

//------------------------------------------------------------------------------
// UTF-8 validation (strict)
//------------------------------------------------------------------------------

static bool steel_utf8_is_valid(const uint8_t* s, size_t n)
{
    size_t i = 0;
    while (i < n)
    {
        uint8_t c = s[i];
        if (c <= 0x7F)
        {
            i++;
            continue;
        }

        // 2-byte
        if (c >= 0xC2 && c <= 0xDF)
        {
            if (i + 1 >= n) return false;
            uint8_t c1 = s[i + 1];
            if ((c1 & 0xC0) != 0x80) return false;
            i += 2;
            continue;
        }

        // 3-byte
        if (c >= 0xE0 && c <= 0xEF)
        {
            if (i + 2 >= n) return false;
            uint8_t c1 = s[i + 1];
            uint8_t c2 = s[i + 2];
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return false;

            // Prevent overlongs and surrogates.
            if (c == 0xE0 && c1 < 0xA0) return false;
            if (c == 0xED && c1 >= 0xA0) return false;

            i += 3;
            continue;
        }

        // 4-byte
        if (c >= 0xF0 && c <= 0xF4)
        {
            if (i + 3 >= n) return false;
            uint8_t c1 = s[i + 1];
            uint8_t c2 = s[i + 2];
            uint8_t c3 = s[i + 3];
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return false;

            // Prevent overlongs and > U+10FFFF
            if (c == 0xF0 && c1 < 0x90) return false;
            if (c == 0xF4 && c1 > 0x8F) return false;

            i += 4;
            continue;
        }

        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// Text checks
//------------------------------------------------------------------------------

typedef struct steel_file_report
{
    char* path;
    steel_diagvec diags;
    size_t bytes;
    size_t lines;
} steel_file_report;

typedef struct steel_filerepvec
{
    steel_file_report* items;
    size_t len;
    size_t cap;
} steel_filerepvec;

static void steel_filerepvec_init(steel_filerepvec* v)
{
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

static void steel_filerep_free(steel_file_report* r)
{
    if (!r) return;
    free(r->path);
    steel_diagvec_free(&r->diags);
    r->path = NULL;
    r->bytes = 0;
    r->lines = 0;
}

static void steel_filerepvec_free(steel_filerepvec* v)
{
    if (!v) return;
    for (size_t i = 0; i < v->len; i++) steel_filerep_free(&v->items[i]);
    free(v->items);
    steel_filerepvec_init(v);
}

static steel_file_report* steel_filerepvec_push(steel_filerepvec* v, const char* path)
{
    if (v->len + 1 > v->cap)
    {
        v->cap = (v->cap == 0) ? 16 : (v->cap * 2);
        v->items = (steel_file_report*)steel_xrealloc(v->items, v->cap * sizeof(steel_file_report));
    }

    steel_file_report* r = &v->items[v->len++];
    memset(r, 0, sizeof(*r));
    r->path = steel_xstrdup(path);
    steel_diagvec_init(&r->diags);
    return r;
}

static void steel_check_text_file(const char* path, bool warn_tabs, bool warn_trailing_ws, bool warn_long_lines, steel_file_report* out)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        steel_diagvec_add(&out->diags, STEEL_DIAG_ERROR, "cannot open: %s", strerror(errno));
        return;
    }

    uint8_t buf[64 * 1024];
    size_t total = 0;

    // Keep a small tail buffer for line-based checks.
    // We process lines in a streaming manner.
    char linebuf[STEEL_CHECK_MAX_LINE * 4];
    size_t line_len = 0;
    size_t line_no = 1;

    bool seen_non_utf8 = false;
    bool saw_nul = false;

    for (;;)
    {
        size_t n = fread(buf, 1, sizeof(buf), f);
        if (n == 0)
        {
            if (ferror(f))
            {
                steel_diagvec_add(&out->diags, STEEL_DIAG_ERROR, "read error: %s", strerror(errno));
            }
            break;
        }

        total += n;

        // NUL bytes check
        for (size_t i = 0; i < n; i++)
        {
            if (buf[i] == 0)
            {
                saw_nul = true;
                break;
            }
        }

        // UTF-8 check (best-effort streaming: validate each chunk, but also
        // tolerate boundary splits by validating the concatenation of tail+chunk)
        if (!seen_non_utf8)
        {
            // Build a small window that includes up to 4 bytes from previous partial.
            uint8_t win[8];
            size_t win_len = 0;

            // Use last up to 4 bytes of current line buffer (not perfect, but good enough).
            // We just need a few prior bytes to avoid false negatives at boundaries.
            size_t take = (line_len < 4) ? line_len : 4;
            for (size_t i = 0; i < take; i++)
            {
                win[win_len++] = (uint8_t)linebuf[line_len - take + i];
            }

            size_t cpy = (n < (sizeof(win) - win_len)) ? n : (sizeof(win) - win_len);
            for (size_t i = 0; i < cpy; i++) win[win_len++] = buf[i];

            if (!steel_utf8_is_valid(win, win_len))
            {
                // This can still be a false positive if boundary happened before our sample,
                // but in practice it catches binary files.
                seen_non_utf8 = true;
            }
        }

        // Line-based checks
        for (size_t i = 0; i < n; i++)
        {
            uint8_t c = buf[i];

            if (c == '\n')
            {
                // evaluate line [0..line_len)
                if (warn_tabs)
                {
                    for (size_t k = 0; k < line_len; k++)
                    {
                        if (linebuf[k] == '\t')
                        {
                            steel_diagvec_add(&out->diags, STEEL_DIAG_WARNING, "line %zu: contains tab character", line_no);
                            break;
                        }
                    }
                }

                if (warn_trailing_ws)
                {
                    // ignore CR
                    size_t end = line_len;
                    while (end > 0 && (linebuf[end - 1] == '\r')) end--;
                    size_t t = end;
                    while (t > 0 && (linebuf[t - 1] == ' ' || linebuf[t - 1] == '\t')) t--;
                    if (t != end && end > 0)
                    {
                        steel_diagvec_add(&out->diags, STEEL_DIAG_WARNING, "line %zu: trailing whitespace", line_no);
                    }
                }

                if (warn_long_lines)
                {
                    size_t logical = line_len;
                    if (logical > 0 && linebuf[logical - 1] == '\r') logical--;
                    if (logical > STEEL_CHECK_MAX_LINE)
                    {
                        steel_diagvec_add(&out->diags, STEEL_DIAG_WARNING, "line %zu: long line (%zu > %d)", line_no, logical, (int)STEEL_CHECK_MAX_LINE);
                    }
                }

                line_no++;
                line_len = 0;
                continue;
            }

            if (line_len + 1 < sizeof(linebuf))
            {
                linebuf[line_len++] = (char)c;
            }
            else
            {
                // keep consuming but cap buffer; long line warning will still trigger.
                line_len++;
            }
        }
    }

    fclose(f);

    out->bytes = total;
    out->lines = (line_no == 0) ? 0 : (line_no - 1);

    if (saw_nul)
    {
        steel_diagvec_add(&out->diags, STEEL_DIAG_ERROR, "contains NUL byte(s) (binary file?)");
    }

    if (seen_non_utf8)
    {
        steel_diagvec_add(&out->diags, STEEL_DIAG_WARNING, "content does not look like valid UTF-8");
    }
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
        "Usage: steel check [options] [paths...]\n"
        "\n"
        "Performs lightweight static checks on Vitte/Steel sources and manifests.\n"
        "If a path is a directory, it is scanned recursively.\n"
        "\n"
        "Options:\n"
        "  -h, --help           Show this help\n"
        "  -q, --quiet          Only print errors (no per-file OK)\n"
        "  -v, --verbose        Print additional per-file details\n"
        "      --json           Emit JSON report to stdout\n"
        "      --no-tabs        Do not warn on tab characters\n"
        "      --no-trailing-ws Do not warn on trailing whitespace\n"
        "      --no-long-lines  Do not warn on long lines\n"
        "\n"
        "Exit codes:\n"
        "  0  Success (no errors)\n"
        "  1  Errors found\n"
        "  2  Invalid usage\n");
}

//------------------------------------------------------------------------------
// Command entry point
//------------------------------------------------------------------------------

typedef struct steel_check_opts
{
    bool verbose;
    bool quiet;
    bool json;
    bool warn_tabs;
    bool warn_trailing_ws;
    bool warn_long_lines;
} steel_check_opts;

static int steel_cmd_check_impl(int argc, char** argv)
{
    steel_check_opts opt;
    opt.verbose = false;
    opt.quiet = false;
    opt.json = false;
    opt.warn_tabs = true;
    opt.warn_trailing_ws = true;
    opt.warn_long_lines = true;

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

        if (strcmp(a, "--no-tabs") == 0)
        {
            opt.warn_tabs = false;
            continue;
        }

        if (strcmp(a, "--no-trailing-ws") == 0)
        {
            opt.warn_trailing_ws = false;
            continue;
        }

        if (strcmp(a, "--no-long-lines") == 0)
        {
            opt.warn_long_lines = false;
            continue;
        }

        if (a[0] == '-')
        {
            fprintf(stderr, "steel check: unknown option: %s\n", a);
            steel_print_help(stderr);
            steel_strvec_free(&roots);
            return 2;
        }

        steel_strvec_push(&roots, a);
    }

    if (roots.len == 0)
    {
        steel_strvec_push(&roots, ".");
    }

    steel_strvec files;
    steel_strvec_init(&files);

    for (size_t i = 0; i < roots.len; i++)
    {
        const char* root = roots.items[i];
        steel_collect_files_recursive(root, &files);
    }

    steel_filerepvec reports;
    steel_filerepvec_init(&reports);

    size_t errors = 0;
    size_t warnings = 0;

    for (size_t i = 0; i < files.len; i++)
    {
        steel_file_report* r = steel_filerepvec_push(&reports, files.items[i]);
        steel_check_text_file(files.items[i], opt.warn_tabs, opt.warn_trailing_ws, opt.warn_long_lines, r);

        for (size_t k = 0; k < r->diags.len; k++)
        {
            if (r->diags.items[k].level == STEEL_DIAG_ERROR) errors++;
            else warnings++;
        }
    }

    // Output
    if (opt.json)
    {
        FILE* out = stdout;
        fputs("{\n", out);
        fputs("  \"command\": \"check\",\n", out);
        fputs("  \"summary\": {\n", out);
        fprintf(out, "    \"files\": %zu,\n", reports.len);
        fprintf(out, "    \"warnings\": %zu,\n", warnings);
        fprintf(out, "    \"errors\": %zu\n", errors);
        fputs("  },\n", out);
        fputs("  \"files\": [\n", out);

        for (size_t i = 0; i < reports.len; i++)
        {
            const steel_file_report* r = &reports.items[i];
            bool ok = true;
            for (size_t k = 0; k < r->diags.len; k++)
            {
                if (r->diags.items[k].level == STEEL_DIAG_ERROR) { ok = false; break; }
            }

            fputs("    {\n", out);
            fputs("      \"path\": ", out);
            steel_json_escape(out, r->path);
            fputs(",\n", out);
            fprintf(out, "      \"bytes\": %zu,\n", r->bytes);
            fprintf(out, "      \"lines\": %zu,\n", r->lines);
            fprintf(out, "      \"ok\": %s,\n", ok ? "true" : "false");
            fputs("      \"diagnostics\": [\n", out);

            for (size_t k = 0; k < r->diags.len; k++)
            {
                const steel_diag* d = &r->diags.items[k];
                fputs("        { \"level\": ", out);
                steel_json_escape(out, (d->level == STEEL_DIAG_ERROR) ? "error" : "warning");
                fputs(", \"message\": ", out);
                steel_json_escape(out, d->msg);
                fputs(" }", out);
                if (k + 1 < r->diags.len) fputs(",", out);
                fputs("\n", out);
            }

            fputs("      ]\n", out);
            fputs("    }", out);
            if (i + 1 < reports.len) fputs(",", out);
            fputs("\n", out);
        }

        fputs("  ]\n", out);
        fputs("}\n", out);
    }
    else
    {
        for (size_t i = 0; i < reports.len; i++)
        {
            const steel_file_report* r = &reports.items[i];

            bool has_err = false;
            bool has_warn = false;
            for (size_t k = 0; k < r->diags.len; k++)
            {
                if (r->diags.items[k].level == STEEL_DIAG_ERROR) has_err = true;
                else has_warn = true;
            }

            if (opt.quiet && !has_err) continue;

            if (!has_err && !has_warn)
            {
                if (!opt.quiet)
                    printf("OK   %s\n", r->path);
                continue;
            }

            if (has_err) printf("FAIL %s\n", r->path);
            else printf("WARN %s\n", r->path);

            for (size_t k = 0; k < r->diags.len; k++)
            {
                const steel_diag* d = &r->diags.items[k];
                const char* lvl = (d->level == STEEL_DIAG_ERROR) ? "error" : "warning";
                printf("  - %s: %s\n", lvl, d->msg);
            }

            if (opt.verbose)
            {
                printf("    bytes=%zu lines=%zu\n", r->bytes, r->lines);
            }
        }

        if (!opt.quiet)
        {
            printf("\nSummary: files=%zu warnings=%zu errors=%zu\n", reports.len, warnings, errors);
        }
    }

    steel_filerepvec_free(&reports);
    steel_strvec_free(&files);
    steel_strvec_free(&roots);

    return (errors == 0) ? 0 : 1;
}

// Public symbol(s).
//
// In most CLI designs, the command dispatcher will call a function with the
// signature `int <cmd>(int argc, char** argv)`. If your project uses another
// signature, keep this wrapper and add another one (forwarding to
// `steel_cmd_check_impl`) in a project-specific header/c file.

int steel_cmd_check(int argc, char** argv)
{
    return steel_cmd_check_impl(argc, argv);
}

// Optional alias used by some dispatchers.
int steel_cmd_check_main(int argc, char** argv)
{
    return steel_cmd_check_impl(argc, argv);
}

