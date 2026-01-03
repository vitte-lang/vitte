// SPDX-License-Identifier: MIT
// steel_cmd_pkg.c
//
// `steel pkg` command implementation.
//
// Scope:
//  - Provide a lightweight packaging command suitable for early bootstrap.
//  - Create a simple "package directory" (default: dist/pkg) containing:
//      * A manifest (steel_pkg.json) describing inputs/options.
//      * A copy of selected project files (sources/manifests/docs).
//  - Optionally produce a single-file tar-like bundle is intentionally NOT
//    implemented here to avoid pulling in compression/archiving dependencies.
//
// Notes:
//  - This is designed as an inventory-and-copy step. A later stage can turn the
//    directory into .tar.gz / .zip using external tooling.

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

#ifndef STEEL_PKG_PATH_MAX
  #define STEEL_PKG_PATH_MAX 4096
#endif

#ifndef STEEL_PKG_IO_CHUNK
  #define STEEL_PKG_IO_CHUNK (64u * 1024u)
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

static bool steel_starts_with(const char* s, const char* pfx)
{
    size_t m = strlen(pfx);
    return strncmp(s, pfx, m) == 0;
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

static bool steel_ensure_dir(const char* path)
{
    // Create parent directories (simple, iterative).
    char tmp[STEEL_PKG_PATH_MAX];
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
        || (strcmp(name, "build") == 0)
        || (strcmp(name, "out") == 0)
        || (strcmp(name, "target") == 0)
        || (strcmp(name, "dist") == 0);
}

static bool steel_should_pkg_file(const char* path)
{
    // Keep conservative; exclude binaries by default.
    return steel_ends_with(path, ".vitte")
        || steel_ends_with(path, ".vit")
        || steel_ends_with(path, ".vitl")
        || steel_ends_with(path, ".muf")
        || steel_ends_with(path, ".md")
        || steel_ends_with(path, ".toml")
        || steel_ends_with(path, ".json")
        || steel_ends_with(path, ".c")
        || steel_ends_with(path, ".h")
        || steel_ends_with(path, "LICENSE")
        || steel_ends_with(path, "LICENSE.txt")
        || steel_ends_with(path, "LICENSE.md")
        || steel_ends_with(path, "README")
        || steel_ends_with(path, "README.md")
        || steel_ends_with(path, "CHANGELOG.md");
}

//------------------------------------------------------------------------------
// String vector
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

static int steel_strcmp_ptr(const void* a, const void* b)
{
    const char* const* pa = (const char* const*)a;
    const char* const* pb = (const char* const*)b;
    return strcmp(*pa, *pb);
}

static void steel_strvec_sort(steel_strvec* v)
{
    if (v->len > 1) qsort(v->items, v->len, sizeof(char*), steel_strcmp_ptr);
}

//------------------------------------------------------------------------------
// File discovery
//------------------------------------------------------------------------------

static void steel_collect_files_recursive(const char* root, steel_strvec* out)
{
    if (steel_is_regular_file(root))
    {
        if (steel_should_pkg_file(root)) steel_strvec_push(out, root);
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
            if (steel_should_pkg_file(child)) steel_strvec_push(out, child);
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

        char child[STEEL_PKG_PATH_MAX];
        steel_path_join(child, sizeof(child), root, name);

        if (steel_is_symlink(child))
        {
            // Never follow symlinks.
            continue;
        }

        if (steel_is_directory(child))
        {
            steel_collect_files_recursive(child, out);
        }
        else if (steel_is_regular_file(child))
        {
            if (steel_should_pkg_file(child)) steel_strvec_push(out, child);
        }
    }

    closedir(d);
#endif
}

//------------------------------------------------------------------------------
// Copy
//------------------------------------------------------------------------------

static bool steel_copy_file(const char* src, const char* dst, char* err, size_t err_cap)
{
    FILE* in = fopen(src, "rb");
    if (!in)
    {
        snprintf(err, err_cap, "open src failed: %s", strerror(errno));
        return false;
    }

    // Ensure destination directory exists.
    {
        char dir[STEEL_PKG_PATH_MAX];
        strncpy(dir, dst, sizeof(dir) - 1);
        dir[sizeof(dir) - 1] = 0;

        char* slash1 = strrchr(dir, '/');
        char* slash2 = strrchr(dir, '\\');
        char* slash = slash1;
        if (slash2 && (!slash || slash2 > slash)) slash = slash2;
        if (slash)
        {
            *slash = 0;
            (void)steel_ensure_dir(dir);
        }
    }

    FILE* out = fopen(dst, "wb");
    if (!out)
    {
        snprintf(err, err_cap, "open dst failed: %s", strerror(errno));
        fclose(in);
        return false;
    }

    uint8_t buf[STEEL_PKG_IO_CHUNK];
    for (;;)
    {
        size_t n = fread(buf, 1, sizeof(buf), in);
        if (n == 0)
        {
            if (ferror(in))
            {
                snprintf(err, err_cap, "read failed: %s", strerror(errno));
                fclose(in);
                fclose(out);
                return false;
            }
            break;
        }

        if (fwrite(buf, 1, n, out) != n)
        {
            snprintf(err, err_cap, "write failed: %s", strerror(errno));
            fclose(in);
            fclose(out);
            return false;
        }
    }

    fclose(in);
    if (fclose(out) != 0)
    {
        snprintf(err, err_cap, "flush failed: %s", strerror(errno));
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// JSON helpers
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

//------------------------------------------------------------------------------
// CLI
//------------------------------------------------------------------------------

typedef struct steel_pkg_opts
{
    const char* out_dir;   // package directory
    const char* prefix;    // optional subfolder inside out_dir (e.g. project name)
    bool dry_run;
    bool json;
    bool quiet;
    bool verbose;
    bool sort;
} steel_pkg_opts;

static void steel_print_help(FILE* out)
{
    fprintf(out,
        "Usage: steel pkg [options] [roots...]\n"
        "\n"
        "Creates a package directory with a manifest and copied source files.\n"
        "This is intended to feed external archivers (tar/zip) in later stages.\n"
        "\n"
        "Options:\n"
        "  -h, --help            Show this help\n"
        "  -o, --out <dir>       Output directory (default: dist/pkg)\n"
        "      --prefix <name>   Place contents under <dir>/<name>/\n"
        "  -n, --dry-run         Do not copy, only report\n"
        "      --json            Emit JSON report to stdout\n"
        "  -q, --quiet           Reduce output\n"
        "  -v, --verbose         Verbose per-file output\n"
        "      --no-sort         Do not sort file list (default: sort)\n"
        "\n"
        "Exit codes:\n"
        "  0  Success\n"
        "  1  Errors occurred\n"
        "  2  Invalid usage\n");
}

static int steel_write_pkg_manifest(const char* out_dir, const char* prefix, const steel_strvec* files, const steel_pkg_opts* opt)
{
    char manifest_path[STEEL_PKG_PATH_MAX];
    if (prefix && prefix[0])
    {
        char base[STEEL_PKG_PATH_MAX];
        steel_path_join(base, sizeof(base), out_dir, prefix);
        steel_path_join(manifest_path, sizeof(manifest_path), base, "steel_pkg.json");
    }
    else
    {
        steel_path_join(manifest_path, sizeof(manifest_path), out_dir, "steel_pkg.json");
    }

    (void)steel_ensure_dir((prefix && prefix[0]) ? (char[STEEL_PKG_PATH_MAX]){0} : out_dir);

    // Ensure target folder exists.
    if (prefix && prefix[0])
    {
        char base[STEEL_PKG_PATH_MAX];
        steel_path_join(base, sizeof(base), out_dir, prefix);
        if (!steel_ensure_dir(base))
        {
            if (!opt->quiet) fprintf(stderr, "steel pkg: cannot create dir: %s\n", base);
            return 1;
        }
    }
    else
    {
        if (!steel_ensure_dir(out_dir))
        {
            if (!opt->quiet) fprintf(stderr, "steel pkg: cannot create dir: %s\n", out_dir);
            return 1;
        }
    }

    if (opt->dry_run) return 0;

    FILE* f = fopen(manifest_path, "wb");
    if (!f)
    {
        if (!opt->quiet) fprintf(stderr, "steel pkg: cannot write manifest '%s': %s\n", manifest_path, strerror(errno));
        return 1;
    }

    fputs("{\n", f);
    fputs("  \"tool\": \"steel pkg\",\n", f);
    fputs("  \"version\": 1,\n", f);
    fputs("  \"out_dir\": ", f);
    steel_json_escape(f, out_dir);
    fputs(",\n", f);
    fputs("  \"prefix\": ", f);
    steel_json_escape(f, (prefix && prefix[0]) ? prefix : "");
    fputs(",\n", f);
    fprintf(f, "  \"file_count\": %zu,\n", files->len);
    fputs("  \"files\": [\n", f);

    for (size_t i = 0; i < files->len; i++)
    {
        fputs("    ", f);
        steel_json_escape(f, files->items[i]);
        if (i + 1 < files->len) fputs(",", f);
        fputs("\n", f);
    }

    fputs("  ]\n", f);
    fputs("}\n", f);

    fclose(f);
    return 0;
}

static int steel_cmd_pkg_impl(int argc, char** argv)
{
    steel_pkg_opts opt;
    opt.out_dir = "dist/pkg";
    opt.prefix = "";
    opt.dry_run = false;
    opt.json = false;
    opt.quiet = false;
    opt.verbose = false;
    opt.sort = true;

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

        if (strcmp(a, "-o") == 0 || strcmp(a, "--out") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel pkg: missing value after %s\n", a);
                steel_print_help(stderr);
                steel_strvec_free(&roots);
                return 2;
            }
            opt.out_dir = argv[++i];
            continue;
        }

        if (strcmp(a, "--prefix") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel pkg: missing value after --prefix\n");
                steel_print_help(stderr);
                steel_strvec_free(&roots);
                return 2;
            }
            opt.prefix = argv[++i];
            continue;
        }

        if (strcmp(a, "-n") == 0 || strcmp(a, "--dry-run") == 0)
        {
            opt.dry_run = true;
            continue;
        }

        if (strcmp(a, "--json") == 0)
        {
            opt.json = true;
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

        if (strcmp(a, "--no-sort") == 0)
        {
            opt.sort = false;
            continue;
        }

        if (a[0] == '-')
        {
            fprintf(stderr, "steel pkg: unknown option: %s\n", a);
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

    // Prepare output base directory
    if (!opt.dry_run)
    {
        if (!steel_ensure_dir(opt.out_dir))
        {
            if (!opt.quiet) fprintf(stderr, "steel pkg: cannot create out dir: %s\n", opt.out_dir);
            steel_strvec_free(&files);
            steel_strvec_free(&roots);
            return 1;
        }

        if (opt.prefix && opt.prefix[0])
        {
            char base[STEEL_PKG_PATH_MAX];
            steel_path_join(base, sizeof(base), opt.out_dir, opt.prefix);
            if (!steel_ensure_dir(base))
            {
                if (!opt.quiet) fprintf(stderr, "steel pkg: cannot create prefix dir: %s\n", base);
                steel_strvec_free(&files);
                steel_strvec_free(&roots);
                return 1;
            }
        }
    }

    // Write manifest (json file inside package dir)
    if (steel_write_pkg_manifest(opt.out_dir, opt.prefix, &files, &opt) != 0)
    {
        steel_strvec_free(&files);
        steel_strvec_free(&roots);
        return 1;
    }

    // Copy files preserving relative paths when possible.
    size_t copied = 0;
    size_t errors = 0;

    for (size_t i = 0; i < files.len; i++)
    {
        const char* src = files.items[i];

        // Compute destination path.
        // Heuristic: if src begins with "./" strip it.
        const char* rel = src;
        if (steel_starts_with(rel, "./")) rel += 2;
#if defined(_WIN32)
        if (steel_starts_with(rel, ".\\")) rel += 2;
#endif

        char dst_base[STEEL_PKG_PATH_MAX];
        if (opt.prefix && opt.prefix[0])
        {
            char t[STEEL_PKG_PATH_MAX];
            steel_path_join(t, sizeof(t), opt.out_dir, opt.prefix);
            steel_path_join(dst_base, sizeof(dst_base), t, rel);
        }
        else
        {
            steel_path_join(dst_base, sizeof(dst_base), opt.out_dir, rel);
        }

        if (opt.dry_run)
        {
            copied++;
            if (opt.verbose && !opt.quiet) printf("COPY %s -> %s (dry-run)\n", src, dst_base);
            continue;
        }

        char err[256];
        if (!steel_copy_file(src, dst_base, err, sizeof(err)))
        {
            errors++;
            if (!opt.quiet) fprintf(stderr, "steel pkg: copy failed: %s -> %s: %s\n", src, dst_base, err);
        }
        else
        {
            copied++;
            if (opt.verbose && !opt.quiet) printf("COPY %s -> %s\n", src, dst_base);
        }
    }

    if (opt.json)
    {
        FILE* out = stdout;
        fputs("{\n", out);
        fputs("  \"command\": \"pkg\",\n", out);
        fputs("  \"out_dir\": ", out);
        steel_json_escape(out, opt.out_dir);
        fputs(",\n", out);
        fputs("  \"prefix\": ", out);
        steel_json_escape(out, (opt.prefix && opt.prefix[0]) ? opt.prefix : "");
        fputs(",\n", out);
        fprintf(out, "  \"roots\": %zu,\n", roots.len);
        fprintf(out, "  \"files_discovered\": %zu,\n", files.len);
        fprintf(out, "  \"files_copied\": %zu,\n", copied);
        fprintf(out, "  \"errors\": %zu\n", errors);
        fputs("}\n", out);
    }
    else
    {
        if (!opt.quiet)
        {
            printf("Summary: discovered=%zu copied=%zu errors=%zu\n", files.len, copied, errors);
            if (!opt.dry_run)
            {
                char manifest_path[STEEL_PKG_PATH_MAX];
                if (opt.prefix && opt.prefix[0])
                {
                    char base[STEEL_PKG_PATH_MAX];
                    steel_path_join(base, sizeof(base), opt.out_dir, opt.prefix);
                    steel_path_join(manifest_path, sizeof(manifest_path), base, "steel_pkg.json");
                }
                else
                {
                    steel_path_join(manifest_path, sizeof(manifest_path), opt.out_dir, "steel_pkg.json");
                }
                printf("Manifest: %s\n", manifest_path);
            }
        }
    }

    steel_strvec_free(&files);
    steel_strvec_free(&roots);

    return errors ? 1 : 0;
}

// Public entry points
int steel_cmd_pkg(int argc, char** argv)
{
    return steel_cmd_pkg_impl(argc, argv);
}

int steel_cmd_pkg_main(int argc, char** argv)
{
    return steel_cmd_pkg_impl(argc, argv);
}

