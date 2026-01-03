// SPDX-License-Identifier: MIT
// pal_posix_proc.c
//
// POSIX process backend (complete).
//
// Provides:
//  - Process spawn (fork/exec) with options: cwd, envp, stdio inherit/null/pipe
//  - Wait with timeout (polling via waitpid + sleep/select)
//  - Kill (signal)
//  - Optional capture helper: run + collect stdout/stderr into buffers
//
// Notes:
//  - Designed for toolchain use: running compilers, linkers, helpers.
//  - Uses only portable POSIX APIs.
//  - If `pal_proc.h` exists, it will be included; otherwise this file provides
//    a fallback public API that can later be moved to `pal_proc.h`.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("../pal_proc.h")
    #include "../pal_proc.h"
    #define STEEL_HAS_PAL_PROC_H 1
  #elif __has_include("pal_proc.h")
    #include "pal_proc.h"
    #define STEEL_HAS_PAL_PROC_H 1
  #endif
#endif

#ifndef STEEL_HAS_PAL_PROC_H

//------------------------------------------------------------------------------
// Fallback API (align later with pal_proc.h)
//------------------------------------------------------------------------------

typedef enum pal_proc_err
{
    PAL_PROC_OK = 0,
    PAL_PROC_EINVAL,
    PAL_PROC_EIO,
    PAL_PROC_EFORK,
    PAL_PROC_EEXEC,
    PAL_PROC_ETIMEDOUT,
} pal_proc_err;

typedef enum pal_stdio_mode
{
    PAL_STDIO_INHERIT = 0,
    PAL_STDIO_NULL,
    PAL_STDIO_PIPE,
} pal_stdio_mode;

typedef struct pal_proc_opts
{
    const char* cwd;                 // optional
    const char* const* envp;         // optional (NULL => inherit)

    pal_stdio_mode stdin_mode;       // default inherit
    pal_stdio_mode stdout_mode;      // default inherit
    pal_stdio_mode stderr_mode;      // default inherit

} pal_proc_opts;

typedef struct pal_proc
{
    pid_t pid;

    // If STDIO_PIPE requested, parent holds read-end here.
    int stdin_fd;   // write-end for child's stdin (parent writes)
    int stdout_fd;  // read-end for child's stdout (parent reads)
    int stderr_fd;  // read-end for child's stderr (parent reads)

    bool running;

} pal_proc;

typedef struct pal_proc_blob
{
    uint8_t* data; // owned
    size_t len;
} pal_proc_blob;

typedef struct pal_proc_capture
{
    pal_proc_blob out;
    pal_proc_blob err;
    int exit_code;
    bool exited;
} pal_proc_capture;

void pal_proc_init(pal_proc* p);
void pal_proc_dispose(pal_proc* p);

pal_proc_err pal_proc_spawn(pal_proc* p, const char* path, const char* const* argv, const pal_proc_opts* opt);

pal_proc_err pal_proc_wait(pal_proc* p, uint32_t timeout_ms, int* out_exit_code, bool* out_exited);

pal_proc_err pal_proc_kill(pal_proc* p, int sig);

bool pal_proc_is_running(pal_proc* p);

void pal_proc_blob_dispose(pal_proc_blob* b);
void pal_proc_capture_dispose(pal_proc_capture* c);

// Convenience: spawn + collect stdout/stderr until process exits or timeout.
pal_proc_err pal_proc_spawn_capture(const char* path, const char* const* argv, const pal_proc_opts* opt,
                                   uint32_t timeout_ms, pal_proc_capture* out);

const char* pal_proc_last_error(void);

#endif // !STEEL_HAS_PAL_PROC_H

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_proc_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    snprintf(g_proc_err_, sizeof(g_proc_err_), "%s", msg);
}

static void set_errno_(const char* prefix)
{
    const char* e = strerror(errno);
    if (!prefix) prefix = "";
    snprintf(g_proc_err_, sizeof(g_proc_err_), "%s%s%s", prefix, (prefix[0] ? ": " : ""), e ? e : "");
}

const char* pal_proc_last_error(void)
{
    return g_proc_err_;
}

//------------------------------------------------------------------------------
// Small helpers
//------------------------------------------------------------------------------

static void close_fd_(int* fd)
{
    if (!fd) return;
    if (*fd >= 0)
    {
        close(*fd);
        *fd = -1;
    }
}

static bool set_cloexec_(int fd)
{
    int fl = fcntl(fd, F_GETFD, 0);
    if (fl < 0) return false;
    return fcntl(fd, F_SETFD, fl | FD_CLOEXEC) == 0;
}

static bool set_nonblock_(int fd, bool nb)
{
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0) return false;
    if (nb) fl |= O_NONBLOCK;
    else fl &= ~O_NONBLOCK;
    return fcntl(fd, F_SETFL, fl) == 0;
}

static int open_devnull_(int flags)
{
    return open("/dev/null", flags, 0);
}

static pal_proc_opts default_opts_(void)
{
    pal_proc_opts o;
    memset(&o, 0, sizeof(o));
    o.stdin_mode = PAL_STDIO_INHERIT;
    o.stdout_mode = PAL_STDIO_INHERIT;
    o.stderr_mode = PAL_STDIO_INHERIT;
    return o;
}

static const char* env_find_(const char* const* envp, const char* key)
{
    if (!envp || !key) return NULL;
    size_t kl = strlen(key);
    for (size_t i = 0; envp[i]; i++)
    {
        const char* s = envp[i];
        if (!s) continue;
        if (strncmp(s, key, kl) == 0 && s[kl] == '=')
            return s + kl + 1;
    }
    return NULL;
}

static bool has_slash_(const char* s)
{
    if (!s) return false;
    for (; *s; s++)
        if (*s == '/') return true;
    return false;
}

static void exec_with_envp_(const char* path, char* const* argv, char* const* envp)
{
    // If path contains '/', call execve directly.
    if (has_slash_(path))
    {
        execve(path, argv, envp);
        return;
    }

    // Search PATH from envp (if provided) else from current env.
    const char* path_env = envp ? env_find_((const char* const*)envp, "PATH") : getenv("PATH");
    if (!path_env || !path_env[0])
    {
        execve(path, argv, envp);
        return;
    }

    // Iterate PATH entries.
    const char* p = path_env;
    while (*p)
    {
        const char* start = p;
        while (*p && *p != ':') p++;
        size_t len = (size_t)(p - start);

        // Build candidate: dir + '/' + path
        char buf[4096];
        if (len == 0)
        {
            // empty entry means current directory
            int n = snprintf(buf, sizeof(buf), "./%s", path);
            if (n > 0 && (size_t)n < sizeof(buf))
                execve(buf, argv, envp);
        }
        else
        {
            if (len + 1 + strlen(path) + 1 < sizeof(buf))
            {
                memcpy(buf, start, len);
                buf[len] = '/';
                snprintf(buf + len + 1, sizeof(buf) - (len + 1), "%s", path);
                execve(buf, argv, envp);
            }
        }

        if (*p == ':') p++;
    }

    // fallback
    execve(path, argv, envp);
}

static pal_proc_err map_errno_(int e)
{
    switch (e)
    {
        case 0: return PAL_PROC_OK;
        case EINVAL: return PAL_PROC_EINVAL;
        case ETIMEDOUT: return PAL_PROC_ETIMEDOUT;
        default: return PAL_PROC_EIO;
    }
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void pal_proc_init(pal_proc* p)
{
    if (!p) return;
    p->pid = (pid_t)-1;
    p->stdin_fd = -1;
    p->stdout_fd = -1;
    p->stderr_fd = -1;
    p->running = false;
}

void pal_proc_dispose(pal_proc* p)
{
    if (!p) return;

    // Best-effort cleanup: close fds.
    close_fd_(&p->stdin_fd);
    close_fd_(&p->stdout_fd);
    close_fd_(&p->stderr_fd);

    // Do not auto-kill; caller decides.
    p->pid = (pid_t)-1;
    p->running = false;
}

bool pal_proc_is_running(pal_proc* p)
{
    if (!p || !p->running || p->pid <= 0)
        return false;

    int st = 0;
    pid_t rc = waitpid(p->pid, &st, WNOHANG);
    if (rc == 0)
        return true;

    if (rc == p->pid)
    {
        p->running = false;
        return false;
    }

    // error
    return false;
}

static bool make_pipe_(int fds[2])
{
#if defined(__APPLE__) || defined(__linux__)
    // Prefer pipe2 if available for CLOEXEC; but keep portable.
#endif
    if (pipe(fds) != 0)
        return false;

    (void)set_cloexec_(fds[0]);
    (void)set_cloexec_(fds[1]);
    return true;
}

static bool setup_child_stdio_(pal_stdio_mode mode, int which_fd /* 0/1/2 */, int pipe_fds[2], bool parent_writes)
{
    // pipe_fds: [0]=read, [1]=write
    // parent_writes indicates parent uses write-end (stdin), child reads.

    if (mode == PAL_STDIO_INHERIT)
        return true;

    if (mode == PAL_STDIO_NULL)
    {
        int dn = open_devnull_(which_fd == 0 ? O_RDONLY : O_WRONLY);
        if (dn < 0)
            return false;

        if (dup2(dn, which_fd) < 0)
        {
            close(dn);
            return false;
        }

        close(dn);
        return true;
    }

    if (mode == PAL_STDIO_PIPE)
    {
        int child_end = parent_writes ? pipe_fds[0] : pipe_fds[1];
        if (dup2(child_end, which_fd) < 0)
            return false;

        // Close both ends after dup2.
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return true;
    }

    return true;
}

pal_proc_err pal_proc_spawn(pal_proc* p, const char* path, const char* const* argv_in, const pal_proc_opts* opt_in)
{
    if (!p || !path || !path[0] || !argv_in || !argv_in[0])
    {
        set_msg_("invalid args");
        return PAL_PROC_EINVAL;
    }

    pal_proc_opts opt = opt_in ? *opt_in : default_opts_();

    // Reset state
    pal_proc_dispose(p);
    pal_proc_init(p);

    int in_pipe[2] = { -1, -1 };
    int out_pipe[2] = { -1, -1 };
    int err_pipe[2] = { -1, -1 };

    if (opt.stdin_mode == PAL_STDIO_PIPE)
    {
        if (!make_pipe_(in_pipe))
        {
            set_errno_("pipe(stdin)");
            return PAL_PROC_EIO;
        }
    }

    if (opt.stdout_mode == PAL_STDIO_PIPE)
    {
        if (!make_pipe_(out_pipe))
        {
            set_errno_("pipe(stdout)");
            close_fd_(&in_pipe[0]); close_fd_(&in_pipe[1]);
            return PAL_PROC_EIO;
        }
    }

    if (opt.stderr_mode == PAL_STDIO_PIPE)
    {
        if (!make_pipe_(err_pipe))
        {
            set_errno_("pipe(stderr)");
            close_fd_(&in_pipe[0]); close_fd_(&in_pipe[1]);
            close_fd_(&out_pipe[0]); close_fd_(&out_pipe[1]);
            return PAL_PROC_EIO;
        }
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        set_errno_("fork");
        close_fd_(&in_pipe[0]); close_fd_(&in_pipe[1]);
        close_fd_(&out_pipe[0]); close_fd_(&out_pipe[1]);
        close_fd_(&err_pipe[0]); close_fd_(&err_pipe[1]);
        return PAL_PROC_EFORK;
    }

    if (pid == 0)
    {
        // Child

        // stdin: parent writes -> child reads
        if (opt.stdin_mode == PAL_STDIO_PIPE)
        {
            // close parent end in child
            close(in_pipe[1]);
        }

        // stdout/stderr: parent reads -> child writes
        if (opt.stdout_mode == PAL_STDIO_PIPE)
        {
            close(out_pipe[0]);
        }
        if (opt.stderr_mode == PAL_STDIO_PIPE)
        {
            close(err_pipe[0]);
        }

        // Setup stdio
        if (!setup_child_stdio_(opt.stdin_mode, 0, in_pipe, true))
            _exit(127);
        if (!setup_child_stdio_(opt.stdout_mode, 1, out_pipe, false))
            _exit(127);
        if (!setup_child_stdio_(opt.stderr_mode, 2, err_pipe, false))
            _exit(127);

        // CWD
        if (opt.cwd && opt.cwd[0])
        {
            if (chdir(opt.cwd) != 0)
                _exit(127);
        }

        // Build argv as mutable for exec* APIs.
        // NOTE: we assume argv_in is NULL-terminated.
        size_t argc = 0;
        while (argv_in[argc]) argc++;

        char** argv = (char**)calloc(argc + 1, sizeof(char*));
        if (!argv)
            _exit(127);

        for (size_t i = 0; i < argc; i++)
            argv[i] = (char*)argv_in[i];
        argv[argc] = NULL;

        if (opt.envp)
        {
            exec_with_envp_(path, argv, (char* const*)opt.envp);
        }
        else
        {
            // Search PATH and inherit environ.
            execvp(path, argv);
        }

        _exit(127);
    }

    // Parent
    p->pid = pid;
    p->running = true;

    // Keep parent pipe ends.
    if (opt.stdin_mode == PAL_STDIO_PIPE)
    {
        // parent writes
        close(in_pipe[0]);
        p->stdin_fd = in_pipe[1];
        // best-effort nonblocking off
        (void)set_nonblock_(p->stdin_fd, false);
    }
    else
    {
        close_fd_(&in_pipe[0]); close_fd_(&in_pipe[1]);
    }

    if (opt.stdout_mode == PAL_STDIO_PIPE)
    {
        close(out_pipe[1]);
        p->stdout_fd = out_pipe[0];
        (void)set_nonblock_(p->stdout_fd, true);
    }
    else
    {
        close_fd_(&out_pipe[0]); close_fd_(&out_pipe[1]);
    }

    if (opt.stderr_mode == PAL_STDIO_PIPE)
    {
        close(err_pipe[1]);
        p->stderr_fd = err_pipe[0];
        (void)set_nonblock_(p->stderr_fd, true);
    }
    else
    {
        close_fd_(&err_pipe[0]); close_fd_(&err_pipe[1]);
    }

    return PAL_PROC_OK;
}

static pal_proc_err waitpid_timeout_(pid_t pid, uint32_t timeout_ms, int* out_status, bool* out_exited)
{
    if (out_exited) *out_exited = false;

    uint32_t elapsed = 0;
    const uint32_t step = 10; // ms

    for (;;)
    {
        int st = 0;
        pid_t rc = waitpid(pid, &st, WNOHANG);
        if (rc == pid)
        {
            if (out_status) *out_status = st;
            if (out_exited) *out_exited = true;
            return PAL_PROC_OK;
        }
        if (rc < 0)
        {
            set_errno_("waitpid");
            return PAL_PROC_EIO;
        }

        // rc == 0 => running
        if (timeout_ms != 0 && elapsed >= timeout_ms)
            return PAL_PROC_ETIMEDOUT;

        // Sleep a bit using select.
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = (suseconds_t)(step * 1000);
        (void)select(0, NULL, NULL, NULL, &tv);

        elapsed += step;
    }
}

pal_proc_err pal_proc_wait(pal_proc* p, uint32_t timeout_ms, int* out_exit_code, bool* out_exited)
{
    if (out_exited) *out_exited = false;
    if (out_exit_code) *out_exit_code = -1;

    if (!p || p->pid <= 0)
    {
        set_msg_("invalid proc");
        return PAL_PROC_EINVAL;
    }

    int st = 0;
    bool exited = false;

    pal_proc_err e = waitpid_timeout_(p->pid, timeout_ms, &st, &exited);
    if (e != PAL_PROC_OK)
        return e;

    if (exited)
    {
        int code = -1;
        if (WIFEXITED(st)) code = WEXITSTATUS(st);
        else if (WIFSIGNALED(st)) code = 128 + WTERMSIG(st);

        if (out_exit_code) *out_exit_code = code;
        if (out_exited) *out_exited = true;

        p->running = false;

        // Close pipes: child is gone.
        close_fd_(&p->stdin_fd);
        // stdout/stderr may still have buffered data; caller may read before wait.
    }

    return PAL_PROC_OK;
}

pal_proc_err pal_proc_kill(pal_proc* p, int sig)
{
    if (!p || p->pid <= 0)
    {
        set_msg_("invalid proc");
        return PAL_PROC_EINVAL;
    }

    if (kill(p->pid, sig) != 0)
    {
        set_errno_("kill");
        return PAL_PROC_EIO;
    }

    return PAL_PROC_OK;
}

//------------------------------------------------------------------------------
// Capture helpers
//------------------------------------------------------------------------------

void pal_proc_blob_dispose(pal_proc_blob* b)
{
    if (!b) return;
    free(b->data);
    b->data = NULL;
    b->len = 0;
}

void pal_proc_capture_dispose(pal_proc_capture* c)
{
    if (!c) return;
    pal_proc_blob_dispose(&c->out);
    pal_proc_blob_dispose(&c->err);
    c->exit_code = -1;
    c->exited = false;
}

static bool blob_append_(pal_proc_blob* b, const uint8_t* data, size_t n)
{
    if (!b) return false;
    if (n == 0) return true;

    size_t cap = b->len + n + 1;
    uint8_t* nb = (uint8_t*)realloc(b->data, cap);
    if (!nb) return false;

    memcpy(nb + b->len, data, n);
    b->len += n;
    nb[b->len] = 0; // NUL for convenience

    b->data = nb;
    return true;
}

static pal_proc_err drain_pipes_until_exit_(pal_proc* p, uint32_t timeout_ms, pal_proc_capture* out)
{
    // Nonblocking fds.
    int outfd = p->stdout_fd;
    int errfd = p->stderr_fd;

    uint32_t elapsed = 0;
    const uint32_t step = 25; // ms

    bool out_open = (outfd >= 0);
    bool err_open = (errfd >= 0);

    uint8_t buf[4096];

    for (;;)
    {
        // Check exit without blocking.
        int st = 0;
        pid_t rc = waitpid(p->pid, &st, WNOHANG);
        if (rc == p->pid)
        {
            p->running = false;
            out->exited = true;
            if (WIFEXITED(st)) out->exit_code = WEXITSTATUS(st);
            else if (WIFSIGNALED(st)) out->exit_code = 128 + WTERMSIG(st);
            else out->exit_code = -1;
        }
        else if (rc < 0)
        {
            set_errno_("waitpid");
            return PAL_PROC_EIO;
        }

        // If both pipes closed and process exited, done.
        if ((!out_open && !err_open) && !p->running)
            return PAL_PROC_OK;

        if (timeout_ms != 0 && elapsed >= timeout_ms)
            return PAL_PROC_ETIMEDOUT;

        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = -1;

        if (out_open) { FD_SET(outfd, &rfds); if (outfd > maxfd) maxfd = outfd; }
        if (err_open) { FD_SET(errfd, &rfds); if (errfd > maxfd) maxfd = errfd; }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = (suseconds_t)(step * 1000);

        int sel = 0;
        if (maxfd >= 0)
            sel = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        else
            sel = select(0, NULL, NULL, NULL, &tv);

        if (sel < 0)
        {
            if (errno == EINTR)
                continue;
            set_errno_("select");
            return PAL_PROC_EIO;
        }

        // Read ready fds
        if (out_open && FD_ISSET(outfd, &rfds))
        {
            for (;;)
            {
                ssize_t n = read(outfd, buf, sizeof(buf));
                if (n > 0)
                {
                    if (!blob_append_(&out->out, buf, (size_t)n))
                    {
                        set_msg_("out of memory");
                        return PAL_PROC_EIO;
                    }
                    continue;
                }
                if (n == 0)
                {
                    close_fd_(&p->stdout_fd);
                    out_open = false;
                    break;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                if (errno == EINTR)
                    continue;

                set_errno_("read(stdout)");
                return PAL_PROC_EIO;
            }
        }

        if (err_open && FD_ISSET(errfd, &rfds))
        {
            for (;;)
            {
                ssize_t n = read(errfd, buf, sizeof(buf));
                if (n > 0)
                {
                    if (!blob_append_(&out->err, buf, (size_t)n))
                    {
                        set_msg_("out of memory");
                        return PAL_PROC_EIO;
                    }
                    continue;
                }
                if (n == 0)
                {
                    close_fd_(&p->stderr_fd);
                    err_open = false;
                    break;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                if (errno == EINTR)
                    continue;

                set_errno_("read(stderr)");
                return PAL_PROC_EIO;
            }
        }

        elapsed += step;
    }
}

pal_proc_err pal_proc_spawn_capture(const char* path, const char* const* argv, const pal_proc_opts* opt_in,
                                   uint32_t timeout_ms, pal_proc_capture* out)
{
    if (!out)
    {
        set_msg_("invalid args");
        return PAL_PROC_EINVAL;
    }

    pal_proc_capture_dispose(out);

    pal_proc_opts opt = opt_in ? *opt_in : default_opts_();
    opt.stdout_mode = PAL_STDIO_PIPE;
    opt.stderr_mode = PAL_STDIO_PIPE;

    pal_proc p;
    pal_proc_init(&p);

    pal_proc_err e = pal_proc_spawn(&p, path, argv, &opt);
    if (e != PAL_PROC_OK)
    {
        pal_proc_dispose(&p);
        return e;
    }

    e = drain_pipes_until_exit_(&p, timeout_ms, out);

    // If timed out, leave process running; caller may kill.
    // Always close our pipe fds.
    close_fd_(&p.stdout_fd);
    close_fd_(&p.stderr_fd);
    close_fd_(&p.stdin_fd);

    // If process already exited, reap just in case (drain may have reaped).
    if (!p.running)
    {
        int st = 0;
        (void)waitpid(p.pid, &st, WNOHANG);
    }

    return e;
}

