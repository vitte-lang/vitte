// SPDX-License-Identifier: MIT
// steel_cmd_run.c
//
// `steel run` command implementation.
//
// Scope:
//  - Early-bootstrap friendly runner that delegates to an external command.
//  - Provides a stable UX wrapper for invoking tools (compiler, VM, tests, etc.).
//  - Supports selecting a working directory, environment overrides, and
//    pass-through arguments after `--`.
//
// Design:
//  - We intentionally do not try to interpret Muffin manifests here.
//    This command is a thin process launcher.
//  - Default command is `steel build` then run an output binary is NOT assumed.
//    Instead the user specifies `--cmd <program>`.
//
// Examples:
//  - steel run --cmd vittec -- --help
//  - steel run --cwd examples/hello --cmd vittevm -- hello.vm
//  - steel run --env FOO=bar --cmd sh -- -lc "echo $FOO"

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
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif

#ifndef STEEL_RUN_MAX_ENV
  #define STEEL_RUN_MAX_ENV 128
#endif

#ifndef STEEL_RUN_PATH_MAX
  #define STEEL_RUN_PATH_MAX 4096
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
        v->cap = (v->cap == 0) ? 16 : (v->cap * 2);
        v->items = (char**)steel_xrealloc(v->items, v->cap * sizeof(char*));
    }
    v->items[v->len++] = steel_xstrdup(s);
}

//------------------------------------------------------------------------------
// Help
//------------------------------------------------------------------------------

static void steel_print_help(FILE* out)
{
    fprintf(out,
        "Usage: steel run [options] --cmd <program> [-- <args...>]\n"
        "\n"
        "Runs an external command in a controlled way. This is a thin process launcher.\n"
        "\n"
        "Options:\n"
        "  -h, --help            Show this help\n"
        "      --cmd <program>   Program to execute (required)\n"
        "      --cwd <dir>       Working directory for the command\n"
        "      --env KEY=VALUE   Add/override environment variable (repeatable)\n"
        "      --inherit-env     Inherit the current environment (default)\n"
        "      --clean-env       Start from an empty environment, then apply --env\n"
        "      --               All remaining args are passed to the program\n"
        "\n"
        "Exit codes:\n"
        "  0  Success\n"
        "  1  Failed to launch or command returned non-zero\n"
        "  2  Invalid usage\n");
}

//------------------------------------------------------------------------------
// Environment handling
//------------------------------------------------------------------------------

typedef struct steel_env
{
    bool inherit;
    steel_strvec kvs; // KEY=VALUE
} steel_env;

static void steel_env_init(steel_env* e)
{
    e->inherit = true;
    steel_strvec_init(&e->kvs);
}

static void steel_env_free(steel_env* e)
{
    steel_strvec_free(&e->kvs);
}

//------------------------------------------------------------------------------
// Process spawning
//------------------------------------------------------------------------------

#if defined(_WIN32)

static char* steel_quote_arg_win(const char* s)
{
    // Minimal quoting for Windows CreateProcess command line.
    // Rule: wrap in quotes if spaces or tabs or quotes exist; escape quotes with backslash.
    bool need = false;
    for (const char* p = s; *p; p++)
    {
        if (*p == ' ' || *p == '\t' || *p == '"') { need = true; break; }
    }
    if (!need) return steel_xstrdup(s);

    size_t n = 2; // quotes
    for (const char* p = s; *p; p++)
    {
        if (*p == '"') n += 2; else n += 1;
    }

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
    // Build a single command line: program + args.
    steel_strvec parts;
    steel_strvec_init(&parts);

    steel_strvec_push(&parts, program);
    for (size_t i = 0; i < args->len; i++) steel_strvec_push(&parts, args->items[i]);

    // Quote each part as needed.
    size_t total = 0;
    char** quoted = (char**)steel_xmalloc(parts.len * sizeof(char*));
    for (size_t i = 0; i < parts.len; i++)
    {
        quoted[i] = steel_quote_arg_win(parts.items[i]);
        total += strlen(quoted[i]) + 1;
    }

    char* cmd = (char*)steel_xmalloc(total + 1);
    cmd[0] = 0;
    for (size_t i = 0; i < parts.len; i++)
    {
        strcat(cmd, quoted[i]);
        if (i + 1 < parts.len) strcat(cmd, " ");
        free(quoted[i]);
    }

    free(quoted);
    steel_strvec_free(&parts);
    return cmd;
}

static char* steel_build_env_block_win(const steel_env* env)
{
    // Build environment block for CreateProcess: sequence of NUL-terminated strings,
    // terminated by an extra NUL.
    // If inherit=true, we start from current env and override with kvs.

    // Gather base environment
    steel_strvec all;
    steel_strvec_init(&all);

    if (env->inherit)
    {
        LPCH block = GetEnvironmentStringsA();
        if (block)
        {
            for (const char* p = block; *p; )
            {
                size_t n = strlen(p);
                steel_strvec_push(&all, p);
                p += n + 1;
            }
            FreeEnvironmentStringsA(block);
        }
    }

    // Apply overrides
    for (size_t i = 0; i < env->kvs.len; i++)
    {
        const char* kv = env->kvs.items[i];
        const char* eq = strchr(kv, '=');
        if (!eq) continue;

        size_t klen = (size_t)(eq - kv);

        // Remove existing key entries (case-insensitive on Windows)
        for (size_t j = 0; j < all.len; )
        {
            const char* cur = all.items[j];
            const char* ceq = strchr(cur, '=');
            size_t clen = ceq ? (size_t)(ceq - cur) : strlen(cur);

            bool match = (clen == klen);
            if (match)
            {
                for (size_t k = 0; k < klen; k++)
                {
                    char a = kv[k];
                    char b = cur[k];
                    if (a >= 'a' && a <= 'z') a = (char)(a - 'a' + 'A');
                    if (b >= 'a' && b <= 'z') b = (char)(b - 'a' + 'A');
                    if (a != b) { match = false; break; }
                }
            }

            if (match)
            {
                free(all.items[j]);
                memmove(&all.items[j], &all.items[j + 1], (all.len - j - 1) * sizeof(char*));
                all.len--;
                continue;
            }
            j++;
        }

        steel_strvec_push(&all, kv);
    }

    // Sort is optional; CreateProcess doesn't require, but stable ordering helps debugging.
    // We'll do a simple bubble for small sizes to avoid qsort function pointer overhead.
    for (size_t i = 0; i + 1 < all.len; i++)
    {
        for (size_t j = i + 1; j < all.len; j++)
        {
            if (strcmp(all.items[i], all.items[j]) > 0)
            {
                char* t = all.items[i];
                all.items[i] = all.items[j];
                all.items[j] = t;
            }
        }
    }

    size_t total = 1; // final NUL
    for (size_t i = 0; i < all.len; i++) total += strlen(all.items[i]) + 1;

    char* block = (char*)steel_xmalloc(total);
    char* w = block;
    for (size_t i = 0; i < all.len; i++)
    {
        size_t n = strlen(all.items[i]);
        memcpy(w, all.items[i], n);
        w += n;
        *w++ = 0;
    }
    *w++ = 0;

    steel_strvec_free(&all);
    return block;
}

static int steel_spawn_win(const char* program, const steel_strvec* args, const char* cwd, const steel_env* env)
{
    char* cmdline = steel_build_cmdline_win(program, args);
    char* envblk = steel_build_env_block_win(env);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
        NULL,            // application name; we pass full command line
        cmdline,         // command line
        NULL, NULL,      // security
        TRUE,            // inherit handles
        0,               // flags
        envblk,          // environment block
        (cwd && cwd[0]) ? cwd : NULL,
        &si,
        &pi);

    if (!ok)
    {
        DWORD e = GetLastError();
        fprintf(stderr, "steel run: CreateProcess failed (winerr=%lu)\n", (unsigned long)e);
        free(cmdline);
        free(envblk);
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    free(cmdline);
    free(envblk);

    return (int)code;
}

#else

extern char** environ;

static int steel_spawn_posix(const char* program, const steel_strvec* args, const char* cwd, const steel_env* env)
{
    // Build argv: program + args + NULL
    size_t argc2 = 1 + args->len + 1;
    char** argv2 = (char**)steel_xmalloc(argc2 * sizeof(char*));
    size_t k = 0;
    argv2[k++] = (char*)program;
    for (size_t i = 0; i < args->len; i++) argv2[k++] = args->items[i];
    argv2[k++] = NULL;

    // Build envp if needed
    char** envp = NULL;
    steel_strvec envbuf;
    steel_strvec_init(&envbuf);

    if (!env->inherit || env->kvs.len > 0)
    {
        // Start from empty or inherit
        if (env->inherit)
        {
            for (char** p = environ; *p; p++) steel_strvec_push(&envbuf, *p);
        }

        // Apply overrides (replace by key)
        for (size_t i = 0; i < env->kvs.len; i++)
        {
            const char* kv = env->kvs.items[i];
            const char* eq = strchr(kv, '=');
            if (!eq) continue;
            size_t klen = (size_t)(eq - kv);

            for (size_t j = 0; j < envbuf.len; )
            {
                const char* cur = envbuf.items[j];
                const char* ceq = strchr(cur, '=');
                size_t clen = ceq ? (size_t)(ceq - cur) : strlen(cur);
                if (clen == klen && strncmp(cur, kv, klen) == 0)
                {
                    free(envbuf.items[j]);
                    memmove(&envbuf.items[j], &envbuf.items[j + 1], (envbuf.len - j - 1) * sizeof(char*));
                    envbuf.len--;
                    continue;
                }
                j++;
            }

            steel_strvec_push(&envbuf, kv);
        }

        envp = (char**)steel_xmalloc((envbuf.len + 1) * sizeof(char*));
        for (size_t i = 0; i < envbuf.len; i++) envp[i] = envbuf.items[i];
        envp[envbuf.len] = NULL;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "steel run: fork failed: %s\n", strerror(errno));
        free(argv2);
        if (envp) free(envp);
        steel_strvec_free(&envbuf);
        return 1;
    }

    if (pid == 0)
    {
        if (cwd && cwd[0])
        {
            if (chdir(cwd) != 0)
            {
                fprintf(stderr, "steel run: chdir '%s' failed: %s\n", cwd, strerror(errno));
                _exit(127);
            }
        }

        if (envp)
            execve(program, argv2, envp);
        else
            execvp(program, argv2);

        // If we reached here, exec failed
        fprintf(stderr, "steel run: exec failed: %s\n", strerror(errno));
        _exit(127);
    }

    int st = 0;
    if (waitpid(pid, &st, 0) < 0)
    {
        fprintf(stderr, "steel run: waitpid failed: %s\n", strerror(errno));
        free(argv2);
        if (envp) free(envp);
        steel_strvec_free(&envbuf);
        return 1;
    }

    free(argv2);
    if (envp) free(envp);
    steel_strvec_free(&envbuf);

    if (WIFEXITED(st)) return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return 128 + WTERMSIG(st);
    return 1;
}

#endif

//------------------------------------------------------------------------------
// CLI parsing
//------------------------------------------------------------------------------

static int steel_cmd_run_impl(int argc, char** argv)
{
    const char* program = NULL;
    const char* cwd = NULL;

    steel_env env;
    steel_env_init(&env);

    steel_strvec args;
    steel_strvec_init(&args);

    bool passthru = false;

    for (int i = 1; i < argc; i++)
    {
        const char* a = argv[i];
        if (!a) continue;

        if (!passthru && steel_streq(a, "--"))
        {
            passthru = true;
            continue;
        }

        if (!passthru && (steel_streq(a, "-h") || steel_streq(a, "--help")))
        {
            steel_print_help(stdout);
            steel_env_free(&env);
            steel_strvec_free(&args);
            return 0;
        }

        if (!passthru && steel_streq(a, "--cmd"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel run: missing value after --cmd\n");
                steel_print_help(stderr);
                steel_env_free(&env);
                steel_strvec_free(&args);
                return 2;
            }
            program = argv[++i];
            continue;
        }

        if (!passthru && steel_streq(a, "--cwd"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel run: missing value after --cwd\n");
                steel_print_help(stderr);
                steel_env_free(&env);
                steel_strvec_free(&args);
                return 2;
            }
            cwd = argv[++i];
            continue;
        }

        if (!passthru && steel_streq(a, "--env"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "steel run: missing value after --env\n");
                steel_print_help(stderr);
                steel_env_free(&env);
                steel_strvec_free(&args);
                return 2;
            }
            if (env.kvs.len >= STEEL_RUN_MAX_ENV)
            {
                fprintf(stderr, "steel run: too many --env (max %d)\n", STEEL_RUN_MAX_ENV);
                steel_env_free(&env);
                steel_strvec_free(&args);
                return 2;
            }
            const char* kv = argv[++i];
            if (!strchr(kv, '='))
            {
                fprintf(stderr, "steel run: --env expects KEY=VALUE, got: %s\n", kv);
                steel_env_free(&env);
                steel_strvec_free(&args);
                return 2;
            }
            steel_strvec_push(&env.kvs, kv);
            continue;
        }

        if (!passthru && steel_streq(a, "--inherit-env"))
        {
            env.inherit = true;
            continue;
        }

        if (!passthru && steel_streq(a, "--clean-env"))
        {
            env.inherit = false;
            continue;
        }

        // Anything else:
        if (!program)
        {
            // Allow shorthand: steel run <program> -- <args>
            program = a;
            continue;
        }
        steel_strvec_push(&args, a);
    }

    if (!program || !program[0])
    {
        fprintf(stderr, "steel run: missing --cmd <program>\n");
        steel_print_help(stderr);
        steel_env_free(&env);
        steel_strvec_free(&args);
        return 2;
    }

#if defined(_WIN32)
    int code = steel_spawn_win(program, &args, cwd, &env);
#else
    int code = steel_spawn_posix(program, &args, cwd, &env);
#endif

    steel_env_free(&env);
    steel_strvec_free(&args);

    // Normalize: if the child returned 0 it's success, otherwise 1.
    // But keep the actual code if non-zero to allow tooling.
    return code;
}

// Public entry points
int steel_cmd_run(int argc, char** argv)
{
    return steel_cmd_run_impl(argc, argv);
}

int steel_cmd_run_main(int argc, char** argv)
{
    return steel_cmd_run_impl(argc, argv);
}

