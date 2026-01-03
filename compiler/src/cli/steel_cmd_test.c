// SPDX-License-Identifier: MIT
// steel_cmd_test.c
//
// `steel test` command implementation.
//
// Scope:
//  - Early-bootstrap friendly test runner wrapper.
//  - Does not implement a full test framework; instead it discovers and runs
//    external test executables or scripts in a conventional layout.
//
// Conventions (inventory-based):
//  - If the user provides `--cmd <program>`, we execute that program directly.
//  - Otherwise, we attempt to discover runnable test targets under provided
//    roots (default: `tests/` then `.`):
//      * `tests/bin/*` (executables)
//      * `tests/run.*` (run.sh, run.ps1, run.cmd)
//      * `tests/**/*.sh` or `tests/**/*.ps1` when --scripts is enabled
//  - Pass-through args after `--` are forwarded to each test.
//
// Output:
//  - Text summary by default.
//  - JSON summary with --json.
//
// Exit codes:
//  - 0: all tests passed
//  - 1: at least one test failed or runner error
//  - 2: invalid usage

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
  #include <sys/types.h>
  #include <sys/wait.h>
#endif

#ifndef STEEL_TEST_PATH_MAX
  #define STEEL_TEST_PATH_MAX 4096
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

static bool steel_streq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
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
        || (strcmp(name, "dist") == 0)
        || (strcmp(name, "target") == 0);
}

static bool steel_is_probably_executable(const char* path)
{
#if defined(_WIN32)
    return steel_ends_with(path, ".exe") || steel_ends_with(path, ".cmd") || steel_ends_with(path, ".bat");
#else
    // On POSIX, treat any regular file under tests/bin as runnable; permissions may vary.
    (void)path;
    return true;
#endif
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
    if (v->len > 1) qsort(v->items, v->len, sizeof(char*), steel_strcmp_ptr);
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
// Process spawning
//------------------------------------------------------------------------------

#if defined(_WIN32)

static char* steel_quote_arg_win(const char* s)
{
    bool need = false;
    for (const char* p = s; *p; p++)
    {
        if (*p == ' ' || *p == '\t' || *p == '"') { need = true; break; }
    }
    if (!need) return steel_xstrdup(s);

    size_t n = 2;
    for (const char* p = s; *p; p++) n += (*p == '"') ? 2 : 1;

    char* out = (char*)steel_xmalloc(n + 1);
    char* w = out;
    *w++ = '"';
    for (const char* p = s; *p; p++)
    {
        if (*p == '"') { *w++ = '\\'; *w++ = '"'; }
        else { *w++ = *p; }
    }
    *w++ = '"';
    *w = 0;
    return out;
}

static char* steel_build_cmdline_win(const char* program, const steel_strvec* args)
{
    size_t total = 0;

    char* qp = steel_quote_arg_win(program);
    total += strlen(qp) + 1;

    char** qargs = (char**)steel_xmalloc(args->len * sizeof(char*));
    for (size_t i = 0; i < args->len; i++)
    {
        qargs[i] = steel_quote_arg_win(args->items[i]);
        total += strlen(qargs[i]) + 1;
    }

    char* cmd = (char*)steel_xmalloc(total + 1);
    cmd[0] = 0;
    strcat(cmd, qp);
    for (size_t i = 0; i < args->len; i++)
    {
        strcat(cmd, " ");
        strcat(cmd, qargs[i]);
    }

    free(qp);
    for (size_t i = 0; i < args->len; i++) free(qargs[i]);
    free(qargs);

    return cmd;
}

static int steel_spawn_win(const char* program, const steel_strvec* args, const char* cwd)
{
    char* cmdline = steel_build_cmdline_win(program, args);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
        NULL,
        cmdline,
        NULL, NULL,
        TRUE,
        0,
        NULL,
        (cwd && cwd[0]) ? cwd : NULL,
        &si,
        &pi);

    if (!ok)
    {
        DWORD e = GetLastError();
        fprintf(stderr, "steel test: CreateProcess failed (winerr=%lu)\n", (unsigned long)e);
        free(cmdline);
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    free(cmdline);
    return (int)code;
}

#else

static int steel_spawn_posix(const char* program, const steel_strvec* args, const char* cwd)
{
    size_t argc2 = 1 + args->len + 1;
    char** argv2 = (char**)steel_xmalloc(argc2 * sizeof(char*));
    size_t k = 0;
    argv2[k++] = (char*)program;
    for (size_t i = 0; i < args->len; i++) argv2[k++] = args->items[i];
    argv2[k++] = NULL;

    pid_t pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "steel test: fork failed: %s\n", strerror(errno));
        free(argv2);
        return 1;
    }

    if (pid == 0)
    {
        if (cwd && cwd[0])
        {
            if (chdir(cwd) != 0)
            {
                fprintf(stderr, "steel test: chdir '%s' failed: %s\n", cwd, strerror(errno));
                _exit(127);
            }
        }

        execvp(program, argv2);
        fprintf(stderr, "steel test: exec failed: %s\n", strerror(errno));
        _exit(127);
    }

    int st = 0;
    if (waitpid(pid, &st, 0) < 0)
    {
        fprintf(stderr, "steel test: waitpid failed: %s\n", strerror(errno));
        free(argv2);
        return 1;
    }

    free(argv2);

    if (WIFEXITED(st)) return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return 128 + WTERMSIG(st);
    return 1;
}

#endif

//------------------------------------------------------------------------------
// Discovery
//------------------------------------------------------------------------------

typedef struct steel_test_item
{
    char* path;
    bool is_script;
} steel_test_item;

typedef struct steel_test_list
{
    steel_test_item* items;
    size_t len;
    size_t cap;
} steel_test_list;

static void steel_test_list_init(steel_test_list* l)
{
    l->items = NULL;
    l->len = 0;
    l->cap = 0;
}

static void steel_test_list_free(steel_test_list* l)
{
    if (!l) return;
    for (size_t i = 0; i < l->len; i++) free(l->items[i].path);
    free(l->items);
    steel_test_list_init(l);
}

static void steel_test_list_push(steel_test_list* l, const char* path, bool is_script)
{
    if (l->len + 1 > l->cap)
    {
        l->cap = (l->cap == 0) ? 16 : (l->cap * 2);
        l->items = (steel_test_item*)steel_xrealloc(l->items, l->cap * sizeof(steel_test_item));
    }
    l->items[l->len].path = steel_xstrdup(path);
    l->items[l->len].is_script = is_script;
    l->len++;
}

static int steel_test_item_cmp(const void* a, const void* b)
{
    const steel_test_item* A = (const steel_test_item*)a;
    const steel_test_item* B = (const steel_test_item*)b;
    return strcmp(A->path, B->path);
}

static void steel_test_list_sort(steel_test_list* l)
{
    if (l->len > 1) qsort(l->items, l->len, sizeof(steel_test_item), steel_test_item_cmp);
}

static bool steel_is_script_path(const char* path)
{
#if defined(_WIN32)
    return steel_ends_with(path, ".ps1") || steel_ends_with(path, ".cmd") || steel_ends_with(path, ".bat");
#else
    return steel_ends_with(path, ".sh");
#endif
}

static void steel_collect_scripts_recursive(const char* root, steel_test_list* out)
{
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
            steel_collect_scripts_recursive(child, out);
        }
        else
        {
            if (steel_is_script_path(child)) steel_test_list_push(out, child, true);
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

        char child[STEEL_TEST_PATH_MAX];
        steel_path_join(child, sizeof(child), root, name);

        if (steel_is_symlink(child)) continue;

        if (steel_is_directory(child))
        {
            steel_collect_scripts_recursive(child, out);
        }
        else if (steel_is_regular_file(child))
        {
            if (steel_is_script_path(child)) steel_test_list_push(out, child, true);
        }
    }

    closedir(d);
#endif
}

static void steel_discover_tests(const char* root, bool include_scripts, steel_test_list* out)
{
    // 1) tests/run.*
#if defined(_WIN32)
    const char* run_files[] = { "tests\\run.cmd", "tests\\run.bat", "tests\\run.ps1" };
#else
    const char* run_files[] = { "tests/run.sh" };
#endif

    for (size_t i = 0; i < sizeof(run_files) / sizeof(run_files[0]); i++)
    {
        char p[STEEL_TEST_PATH_MAX];
        steel_path_join(p, sizeof(p), root, run_files[i]);
        if (steel_is_regular_file(p))
        {
            steel_test_list_push(out, p, true);
            // Prefer explicit run file, but do not return early; allow additional targets.
        }
    }

    // 2) tests/bin/*
    {
#if defined(_WIN32)
        const char* bin_dir = "tests\\bin";
#else
        const char* bin_dir = "tests/bin";
#endif
        char b[STEEL_TEST_PATH_MAX];
        steel_path_join(b, sizeof(b), root, bin_dir);
        if (steel_is_directory(b))
        {
#if defined(_WIN32)
            char pattern[MAX_PATH];
            snprintf(pattern, sizeof(pattern), "%s\\*", b);
            WIN32_FIND_DATAA ffd;
            HANDLE h = FindFirstFileA(pattern, &ffd);
            if (h != INVALID_HANDLE_VALUE)
            {
                do
                {
                    const char* name = ffd.cFileName;
                    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

                    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

                    char child[MAX_PATH];
                    steel_path_join(child, sizeof(child), b, name);
                    if (steel_is_probably_executable(child)) steel_test_list_push(out, child, false);

                } while (FindNextFileA(h, &ffd));

                FindClose(h);
            }
#else
            DIR* d = opendir(b);
            if (d)
            {
                for (;;)
                {
                    struct dirent* ent = readdir(d);
                    if (!ent) break;
                    const char* name = ent->d_name;
                    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

                    char child[STEEL_TEST_PATH_MAX];
                    steel_path_join(child, sizeof(child), b, name);
                    if (steel_is_regular_file(child)) steel_test_list_push(out, child, false);
                }
                closedir(d);
            }
#endif
        }
    }

    // 3) tests/**/*.sh|ps1 (optional)
    if (include_scripts)
    {
#if defined(_WIN32)
        char t[STEEL_TEST_PATH_MAX];
        steel_path_join(t, sizeof(t), root, "tests");
#else
        char t[STEEL_TEST_PATH_MAX];
        steel_path_join(t, sizeof(t), root, "tests");
#endif
        steel_collect_scripts_recursive(t, out);
    }
}

//------------------------------------------------------------------------------
// Execution of a single test item
//------------------------------------------------------------------------------

typedef struct steel_test_opts
{
    const char* cwd;
    const char* cmd;
    bool include_scripts;
    bool json;
    bool quiet;
    bool verbose;
    bool sort;
    bool fail_fast;
} steel_test_opts;

static int steel_run_test_item(const steel_test_item* it, const steel_strvec* passthru, const steel_test_opts* opt)
{
    steel_strvec args;
    steel_strvec_init(&args);

    const char* program = it->path;

#if defined(_WIN32)
    if (it->is_script)
    {
        if (steel_ends_with(it->path, ".ps1"))
        {
            program = "powershell";
            steel_strvec_push(&args, "-NoProfile");
            steel_strvec_push(&args, "-ExecutionPolicy");
            steel_strvec_push(&args, "Bypass");
            steel_strvec_push(&args, "-File");
            steel_strvec_push(&args, it->path);
        }
        else
        {
            // .cmd/.bat
            program = "cmd";
            steel_strvec_push(&args, "/C");
            steel_strvec_push(&args, it->path);
        }
    }
#else
    if (it->is_script)
    {
        // Prefer /bin/sh
        program = "sh";
        steel_strvec_push(&args, it->path);
    }
#endif

    // Append passthrough args
    for (size_t i = 0; i < passthru->len; i++) steel_strvec_push(&args, passthru->items[i]);

    if (opt->verbose && !opt->quiet)
        printf("RUN  %s\n", it->path);

#if defined(_WIN32)
    int code = steel_spawn_win(program, &args, opt->cwd);
#else
    int code = steel_spawn_posix(program, &args, opt->cwd);
#endif

    steel_strvec_free(&args);
    return code;
}

//------------------------------------------------------------------------------
// CLI
//------------------------------------------------------------------------------

static void steel_print_help(FILE* out)
{
    fprintf(out,
        "Usage: steel test [options] [roots...] [-- <args...>]\n"
        "\n"
        "Runs tests by discovering conventional test targets or by executing --cmd.\n"
        "\n"
        "Options:\n"
        "  -h, --help            Show this help\n"
        "      --cmd <program>   Run a single command instead of discovery\n"
        "      --cwd <dir>       Working directory (default: none)\n"
        "      --scripts         Include tests/**/*.sh or tests/**/*.ps1 in discovery\n"
        "      --no-scripts      Disable script discovery (default)\n"
        "      --json            Emit JSON summary to stdout\n"
        "  -q, --quiet           Reduce output\n"
        "  -v, --verbose         Verbose per-test output\n"
        "      --no-sort         Do not sort discovered test list\n"
        "      --fail-fast       Stop on first failure\n"
        "\n"
        "Exit codes:\n"
        "  0  All tests passed\n"
        "  1  Failures or runner errors\n"
        "  2  Invalid usage\n");
}

static int steel_cmd_test_impl(int argc, char** argv)
{
    steel_test_opts opt;
    opt.cwd = NULL;
    opt.cmd = NULL;
    opt.include_scripts = false;
    opt.json = false;
    opt.quiet = false;
    opt.verbose = false;
    opt.sort = true;
    opt.fail_fast = false;

    steel_strvec roots;
    steel_strvec_init(&roots);

    steel_strvec passthru;
    steel_strvec_init(&passthru);

    bool passthrough = false;

    for (int i = 1; i < argc; i++)
    {
        const char* a = argv[i];
        if (!a) continue;

        if (!passthrough && steel_streq(a, "--"))
        {
            passthrough = true;
            continue;
        }

        if (passthrough)
        {
            steel_strvec_push(&passthru, a);
            continue;
        }

        if (steel_streq(a, "-h") || steel_streq(a, "--help"))
        {
            steel_print_help(stdout);
            steel_strvec_free(&roots);
            steel_strvec_free(&passthru);
            return 0;
        }

        if (steel_streq(a, "--cmd"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel test: missing value after --cmd\n");
                steel_print_help(stderr);
                steel_strvec_free(&roots);
                steel_strvec_free(&passthru);
                return 2;
            }
            opt.cmd = argv[++i];
            continue;
        }

        if (steel_streq(a, "--cwd"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel test: missing value after --cwd\n");
                steel_print_help(stderr);
                steel_strvec_free(&roots);
                steel_strvec_free(&passthru);
                return 2;
            }
            opt.cwd = argv[++i];
            continue;
        }

        if (steel_streq(a, "--scripts"))
        {
            opt.include_scripts = true;
            continue;
        }

        if (steel_streq(a, "--no-scripts"))
        {
            opt.include_scripts = false;
            continue;
        }

        if (steel_streq(a, "--json"))
        {
            opt.json = true;
            continue;
        }

        if (steel_streq(a, "-q") || steel_streq(a, "--quiet"))
        {
            opt.quiet = true;
            continue;
        }

        if (steel_streq(a, "-v") || steel_streq(a, "--verbose"))
        {
            opt.verbose = true;
            continue;
        }

        if (steel_streq(a, "--no-sort"))
        {
            opt.sort = false;
            continue;
        }

        if (steel_streq(a, "--fail-fast"))
        {
            opt.fail_fast = true;
            continue;
        }

        if (a[0] == '-')
        {
            fprintf(stderr, "steel test: unknown option: %s\n", a);
            steel_print_help(stderr);
            steel_strvec_free(&roots);
            steel_strvec_free(&passthru);
            return 2;
        }

        steel_strvec_push(&roots, a);
    }

    if (roots.len == 0)
    {
        // Prefer tests/ if exists, else fallback to .
        if (steel_is_directory("tests"))
            steel_strvec_push(&roots, ".");
        else
            steel_strvec_push(&roots, ".");
    }

    size_t total = 0;
    size_t passed = 0;
    size_t failed = 0;
    size_t errors = 0;

    steel_test_list tests;
    steel_test_list_init(&tests);

    if (opt.cmd && opt.cmd[0])
    {
        // Run single command.
        steel_test_item it;
        it.path = (char*)opt.cmd;
        it.is_script = steel_is_script_path(opt.cmd);

        total = 1;

        int code = steel_run_test_item(&it, &passthru, &opt);
        if (code == 0) passed++; else failed++;

        if (!opt.quiet)
            printf("%s: %s (%d)\n", (code == 0) ? "PASS" : "FAIL", opt.cmd, code);

        if (opt.json)
        {
            FILE* out = stdout;
            fputs("{\n", out);
            fputs("  \"command\": \"test\",\n", out);
            fputs("  \"mode\": \"cmd\",\n", out);
            fputs("  \"cmd\": ", out);
            steel_json_escape(out, opt.cmd);
            fputs(",\n", out);
            fprintf(out, "  \"total\": %zu,\n", total);
            fprintf(out, "  \"passed\": %zu,\n", passed);
            fprintf(out, "  \"failed\": %zu,\n", failed);
            fprintf(out, "  \"errors\": %zu\n", errors);
            fputs("}\n", out);
        }

        steel_test_list_free(&tests);
        steel_strvec_free(&roots);
        steel_strvec_free(&passthru);

        return (failed || errors) ? 1 : 0;
    }

    // Discovery mode
    for (size_t i = 0; i < roots.len; i++)
        steel_discover_tests(roots.items[i], opt.include_scripts, &tests);

    if (opt.sort) steel_test_list_sort(&tests);

    if (tests.len == 0)
    {
        if (!opt.quiet) fprintf(stderr, "steel test: no tests discovered\n");
        steel_test_list_free(&tests);
        steel_strvec_free(&roots);
        steel_strvec_free(&passthru);
        return 1;
    }

    for (size_t i = 0; i < tests.len; i++)
    {
        total++;
        int code = steel_run_test_item(&tests.items[i], &passthru, &opt);
        if (code == 0)
        {
            passed++;
            if (!opt.quiet) printf("PASS %s\n", tests.items[i].path);
        }
        else
        {
            failed++;
            if (!opt.quiet) printf("FAIL %s (%d)\n", tests.items[i].path, code);
            if (opt.fail_fast) break;
        }
    }

    if (!opt.quiet)
        printf("Summary: total=%zu passed=%zu failed=%zu errors=%zu\n", total, passed, failed, errors);

    if (opt.json)
    {
        FILE* out = stdout;
        fputs("{\n", out);
        fputs("  \"command\": \"test\",\n", out);
        fputs("  \"mode\": \"discovery\",\n", out);
        fprintf(out, "  \"tests\": %zu,\n", tests.len);
        fprintf(out, "  \"total\": %zu,\n", total);
        fprintf(out, "  \"passed\": %zu,\n", passed);
        fprintf(out, "  \"failed\": %zu,\n", failed);
        fprintf(out, "  \"errors\": %zu,\n", errors);
        fputs("  \"items\": [\n", out);
        for (size_t i = 0; i < tests.len; i++)
        {
            fputs("    ", out);
            steel_json_escape(out, tests.items[i].path);
            if (i + 1 < tests.len) fputs(",", out);
            fputs("\n", out);
        }
        fputs("  ]\n", out);
        fputs("}\n", out);
    }

    steel_test_list_free(&tests);
    steel_strvec_free(&roots);
    steel_strvec_free(&passthru);

    return (failed || errors) ? 1 : 0;
}

// Public entry points
int steel_cmd_test(int argc, char** argv)
{
    return steel_cmd_test_impl(argc, argv);
}

int steel_cmd_test_main(int argc, char** argv)
{
    return steel_cmd_test_impl(argc, argv);
}
