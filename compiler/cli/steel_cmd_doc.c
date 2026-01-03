// SPDX-License-Identifier: MIT
// steel_cmd_doc.c
//
// `steel doc` command implementation.
//
// Goals:
//  - Provide a documentation generation command that works in early bootstrap.
//  - Default behavior: scan project roots for Vitte/Steel sources and manifests,
//    then produce an index file (Markdown by default) listing modules/files.
//  - Optional: emit JSON instead.
//
// Notes:
//  - This implementation is intentionally conservative; it does not attempt to
//    parse the language. It provides an inventory that downstream doc tooling
//    can enrich later.

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

#ifndef STEEL_DOC_PATH_MAX
  #define STEEL_DOC_PATH_MAX 4096
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

static const char* steel_ext(const char* path)
{
    const char* b = steel_basename(path);
    const char* dot = strrchr(b, '.');
    return dot ? dot : "";
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
        v->cap = (v->cap == 0) ? 32 : (v->cap * 2);
        v->items = (char**)steel_xrealloc(v->items, v->cap * sizeof(char*));
    }
    v->items[v->len++] = steel_xstrdup(s);
}

static int steel_strcmp_ptr(const void* a, const void* b)
{
    const char* const* pa = (const char* const*)a;
    const char* const* pb = (const char* const*)b;
    return strcmp(*pa, *pb);
}

static void steel_strvec_sort(steel_strvec* v)
{
    if (v->len > 1)
        qsort(v->items, v->len, sizeof(char*), steel_strcmp_ptr);
}

//------------------------------------------------------------------------------
// Filesystem helpers
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

static bool steel_should_doc_file(const char* path)
{
    // Inventory these by default.
    return steel_ends_with(path, ".vitte")
        || steel_ends_with(path, ".vit")
        || steel_ends_with(path, ".vitl")
        || steel_ends_with(path, ".muf")
        || steel_ends_with(path, ".md")
        || steel_ends_with(path, ".toml")
        || steel_ends_with(path, ".json");
}

static void steel_collect_files_recursive(const char* root, steel_strvec* out)
{
    if (steel_is_regular_file(root))
    {
        if (steel_should_doc_file(root)) steel_strvec_push(out, root);
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
            // Skip reparse points to avoid loops.
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
            steel_collect_files_recursive(child, out);
        }
        else
        {
            if (steel_should_doc_file(child)) steel_strvec_push(out, child);
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

        char child[STEEL_DOC_PATH_MAX];
        steel_path_join(child, sizeof(child), root, name);

        // Never follow symlinks.
        struct stat st;
        if (lstat(child, &st) != 0) continue;
        if (S_ISLNK(st.st_mode))
        {
            if (S_ISREG(st.st_mode) && steel_should_doc_file(child)) steel_strvec_push(out, child);
            continue;
        }

        if (S_ISDIR(st.st_mode))
        {
            steel_collect_files_recursive(child, out);
        }
        else if (S_ISREG(st.st_mode))
        {
            if (steel_should_doc_file(child)) steel_strvec_push(out, child);
        }
    }

    closedir(d);
#endif
}

//------------------------------------------------------------------------------
// Output
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
        "Usage: steel doc [options] [roots...]\n"
        "\n"
        "Generates a lightweight documentation index (inventory) for the project.\n"
        "By default, writes docs/index.md.\n"
        "\n"
        "Options:\n"
        "  -h, --help            Show this help\n"
        "  -o, --out <path>      Output file path (default: docs/index.md)\n"
        "      --json            Emit JSON to stdout (ignores --out unless --out is '-')\n"
        "      --stdout          Write Markdown to stdout\n"
        "      --no-sort         Keep discovery order (default: sort)\n"
        "      --title <string>  Markdown title (default: Project Index)\n"
        "\n"
        "Exit codes:\n"
        "  0  Success\n"
        "  1  Error\n"
        "  2  Invalid usage\n");
}

static int steel_write_markdown_index(FILE* out, const char* title, const steel_strvec* files)
{
    fprintf(out, "# %s\n\n", title ? title : "Project Index");
    fprintf(out, "Generated by `steel doc`.\n\n");

    // Group by extension for readability.
    const char* current = NULL;
    for (size_t i = 0; i < files->len; i++)
    {
        const char* p = files->items[i];
        const char* ext = steel_ext(p);
        if (!current || strcmp(current, ext) != 0)
        {
            current = ext;
            fprintf(out, "## %s\n\n", (ext && ext[0]) ? ext : "(no extension)");
        }
        fprintf(out, "- %s\n", p);
    }

    return 0;
}

static int steel_write_json_index(FILE* out, const steel_strvec* files)
{
    fputs("{\n", out);
    fputs("  \"command\": \"doc\",\n", out);
    fprintf(out, "  \"files\": %zu,\n", files->len);
    fputs("  \"items\": [\n", out);

    for (size_t i = 0; i < files->len; i++)
    {
        fputs("    ", out);
        steel_json_escape(out, files->items[i]);
        if (i + 1 < files->len) fputs(",", out);
        fputs("\n", out);
    }

    fputs("  ]\n", out);
    fputs("}\n", out);
    return 0;
}

//------------------------------------------------------------------------------
// Command implementation
//------------------------------------------------------------------------------

typedef struct steel_doc_opts
{
    bool json;
    bool to_stdout;
    bool sort;
    const char* out_path;
    const char* title;
} steel_doc_opts;

static int steel_cmd_doc_impl(int argc, char** argv)
{
    steel_doc_opts opt;
    opt.json = false;
    opt.to_stdout = false;
    opt.sort = true;
    opt.out_path = "docs/index.md";
    opt.title = "Project Index";

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

        if (strcmp(a, "--json") == 0)
        {
            opt.json = true;
            continue;
        }

        if (strcmp(a, "--stdout") == 0)
        {
            opt.to_stdout = true;
            continue;
        }

        if (strcmp(a, "--no-sort") == 0)
        {
            opt.sort = false;
            continue;
        }

        if (strcmp(a, "-o") == 0 || strcmp(a, "--out") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel doc: missing value after %s\n", a);
                steel_print_help(stderr);
                steel_strvec_free(&roots);
                return 2;
            }
            opt.out_path = argv[++i];
            continue;
        }

        if (strcmp(a, "--title") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel doc: missing value after --title\n");
                steel_print_help(stderr);
                steel_strvec_free(&roots);
                return 2;
            }
            opt.title = argv[++i];
            continue;
        }

        if (a[0] == '-')
        {
            fprintf(stderr, "steel doc: unknown option: %s\n", a);
            steel_print_help(stderr);
            steel_strvec_free(&roots);
            return 2;
        }

        steel_strvec_push(&roots, a);
    }

    if (roots.len == 0) steel_strvec_push(&roots, ".");

    steel_strvec files;
    steel_strvec_init(&files);

    for (size_t i = 0; i < roots.len; i++)
        steel_collect_files_recursive(roots.items[i], &files);

    if (opt.sort) steel_strvec_sort(&files);

    FILE* out = NULL;

    if (opt.json)
    {
        // JSON defaults to stdout.
        out = stdout;
        // Allow explicit `--out -` to force stdout (already stdout).
        (void)opt;
        int rc = steel_write_json_index(out, &files);
        steel_strvec_free(&files);
        steel_strvec_free(&roots);
        return rc;
    }

    if (opt.to_stdout || (opt.out_path && strcmp(opt.out_path, "-") == 0))
    {
        out = stdout;
    }
    else
    {
        out = fopen(opt.out_path, "wb");
        if (!out)
        {
            fprintf(stderr, "steel doc: cannot open output '%s': %s\n", opt.out_path, strerror(errno));
            steel_strvec_free(&files);
            steel_strvec_free(&roots);
            return 1;
        }
    }

    int rc = steel_write_markdown_index(out, opt.title, &files);

    if (out != stdout) fclose(out);

    steel_strvec_free(&files);
    steel_strvec_free(&roots);

    return rc;
}

// Public entry points
int steel_cmd_doc(int argc, char** argv)
{
    return steel_cmd_doc_impl(argc, argv);
}

int steel_cmd_doc_main(int argc, char** argv)
{
    return steel_cmd_doc_impl(argc, argv);
}
