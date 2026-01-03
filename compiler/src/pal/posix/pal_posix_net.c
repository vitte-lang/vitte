// SPDX-License-Identifier: MIT
// pal_posix_net.c
//
// POSIX networking backend (max).
//
// Scope (intentionally pragmatic for toolchain needs):
//  - TCP connect/listen/accept
//  - send/recv with timeouts
//  - address helpers (IPv4/IPv6, parse/format)
//  - simple HTTP-ish client building blocks can layer on top
//
// Notes:
//  - Uses BSD sockets API.
//  - On Linux: may need -pthread only if you use threads elsewhere.
//  - This file provides a fallback public API if `pal_net.h` doesn't exist yet.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("../pal_net.h")
    #include "../pal_net.h"
    #define STEEL_HAS_PAL_NET_H 1
  #elif __has_include("pal_net.h")
    #include "pal_net.h"
    #define STEEL_HAS_PAL_NET_H 1
  #endif
#endif

#ifndef STEEL_HAS_PAL_NET_H

//------------------------------------------------------------------------------
// Fallback API (align later with pal_net.h)
//------------------------------------------------------------------------------

typedef struct pal_sock
{
    int fd;
} pal_sock;

typedef enum pal_sock_kind
{
    PAL_SOCK_TCP = 1,
} pal_sock_kind;

typedef enum pal_sock_err
{
    PAL_SOCK_OK = 0,
    PAL_SOCK_EINVAL,
    PAL_SOCK_EIO,
    PAL_SOCK_ETIMEDOUT,
    PAL_SOCK_ECONN,
    PAL_SOCK_ENOTFOUND,
} pal_sock_err;

typedef struct pal_addr
{
    // Enough for IPv6.
    struct sockaddr_storage ss;
    socklen_t len;
} pal_addr;

void pal_sock_init(pal_sock* s);
void pal_sock_dispose(pal_sock* s);

bool pal_sock_is_open(const pal_sock* s);

// connect / listen
pal_sock_err pal_tcp_connect(pal_sock* out, const char* host, const char* port, uint32_t timeout_ms);

pal_sock_err pal_tcp_listen(pal_sock* out, const char* bind_host, const char* port, int backlog);

pal_sock_err pal_tcp_accept(const pal_sock* listener, pal_sock* out_client, pal_addr* out_peer);

// send/recv
pal_sock_err pal_sock_send_all(const pal_sock* s, const void* data, size_t len, uint32_t timeout_ms);

pal_sock_err pal_sock_recv_some(const pal_sock* s, void* out_buf, size_t cap, size_t* out_len, uint32_t timeout_ms);

// address helpers
pal_sock_err pal_addr_parse(pal_addr* out, const char* ip, const char* port);
size_t       pal_addr_format(const pal_addr* a, char* out, size_t out_cap);

const char*  pal_net_last_error(void);

#endif // !STEEL_HAS_PAL_NET_H

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_net_err_[256];

static void set_err_(const char* prefix)
{
    if (!prefix) prefix = "";
    const char* e = strerror(errno);
    snprintf(g_net_err_, sizeof(g_net_err_), "%s%s%s", prefix, (prefix[0] ? ": " : ""), e ? e : "");
}

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    snprintf(g_net_err_, sizeof(g_net_err_), "%s", msg);
}

const char* pal_net_last_error(void)
{
    return g_net_err_;
}

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

static pal_sock_err map_errno_(int e)
{
    switch (e)
    {
        case 0: return PAL_SOCK_OK;
        case EINVAL: return PAL_SOCK_EINVAL;
        case ETIMEDOUT: return PAL_SOCK_ETIMEDOUT;
        case ECONNREFUSED:
        case ENOTCONN:
        case EPIPE:
        case ECONNRESET:
            return PAL_SOCK_ECONN;
        case EAI_NONAME:
        case EAI_FAIL:
        case EAI_NODATA:
            return PAL_SOCK_ENOTFOUND;
        default:
            return PAL_SOCK_EIO;
    }
}

static pal_sock_err map_gai_(int gai)
{
    if (gai == 0) return PAL_SOCK_OK;

    // Provide textual error in last_error.
    const char* s = gai_strerror(gai);
    if (s) set_msg_(s);

    switch (gai)
    {
        case EAI_NONAME:
#ifdef EAI_NODATA
        case EAI_NODATA:
#endif
            return PAL_SOCK_ENOTFOUND;
        default:
            return PAL_SOCK_EIO;
    }
}

static void sock_close_(pal_sock* s)
{
    if (!s) return;
    if (s->fd >= 0)
    {
        close(s->fd);
        s->fd = -1;
    }
}

static bool set_nonblock_(int fd, bool nb)
{
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0) return false;

    if (nb) fl |= O_NONBLOCK;
    else fl &= ~O_NONBLOCK;

    return fcntl(fd, F_SETFL, fl) == 0;
}

static pal_sock_err wait_writable_(int fd, uint32_t timeout_ms)
{
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);

    struct timeval tv;
    tv.tv_sec = (time_t)(timeout_ms / 1000);
    tv.tv_usec = (suseconds_t)((timeout_ms % 1000) * 1000);

    int rc = select(fd + 1, NULL, &wfds, NULL, timeout_ms ? &tv : NULL);
    if (rc == 0) return PAL_SOCK_ETIMEDOUT;
    if (rc < 0)
    {
        set_err_("select");
        return PAL_SOCK_EIO;
    }

    return PAL_SOCK_OK;
}

static pal_sock_err wait_readable_(int fd, uint32_t timeout_ms)
{
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct timeval tv;
    tv.tv_sec = (time_t)(timeout_ms / 1000);
    tv.tv_usec = (suseconds_t)((timeout_ms % 1000) * 1000);

    int rc = select(fd + 1, &rfds, NULL, NULL, timeout_ms ? &tv : NULL);
    if (rc == 0) return PAL_SOCK_ETIMEDOUT;
    if (rc < 0)
    {
        set_err_("select");
        return PAL_SOCK_EIO;
    }

    return PAL_SOCK_OK;
}

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

void pal_sock_init(pal_sock* s)
{
    if (!s) return;
    s->fd = -1;
}

void pal_sock_dispose(pal_sock* s)
{
    if (!s) return;
    sock_close_(s);
}

bool pal_sock_is_open(const pal_sock* s)
{
    return s && s->fd >= 0;
}

pal_sock_err pal_tcp_connect(pal_sock* out, const char* host, const char* port, uint32_t timeout_ms)
{
    if (out) pal_sock_init(out);

    if (!out || !host || !port)
    {
        set_msg_("invalid args");
        return PAL_SOCK_EINVAL;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    struct addrinfo* res = NULL;
    int gai = getaddrinfo(host, port, &hints, &res);
    if (gai != 0)
        return map_gai_(gai);

    pal_sock_err last = PAL_SOCK_ECONN;

    for (struct addrinfo* it = res; it; it = it->ai_next)
    {
        int fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0)
        {
            set_err_("socket");
            last = PAL_SOCK_EIO;
            continue;
        }

        // Non-blocking connect to implement timeout.
        if (!set_nonblock_(fd, true))
        {
            set_err_("fcntl");
            close(fd);
            last = PAL_SOCK_EIO;
            continue;
        }

        int rc = connect(fd, it->ai_addr, it->ai_addrlen);
        if (rc == 0)
        {
            // Connected immediately.
            set_nonblock_(fd, false);
            out->fd = fd;
            last = PAL_SOCK_OK;
            break;
        }

        if (rc < 0 && errno != EINPROGRESS)
        {
            last = map_errno_(errno);
            close(fd);
            continue;
        }

        // Wait for writable.
        pal_sock_err w = wait_writable_(fd, timeout_ms);
        if (w != PAL_SOCK_OK)
        {
            last = w;
            close(fd);
            continue;
        }

        // Check SO_ERROR
        int soerr = 0;
        socklen_t sl = (socklen_t)sizeof(soerr);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &sl) != 0)
        {
            set_err_("getsockopt");
            last = PAL_SOCK_EIO;
            close(fd);
            continue;
        }

        if (soerr != 0)
        {
            errno = soerr;
            last = map_errno_(soerr);
            close(fd);
            continue;
        }

        // Success.
        set_nonblock_(fd, false);
        out->fd = fd;
        last = PAL_SOCK_OK;
        break;
    }

    freeaddrinfo(res);
    return last;
}

pal_sock_err pal_tcp_listen(pal_sock* out, const char* bind_host, const char* port, int backlog)
{
    if (out) pal_sock_init(out);

    if (!out || !port)
    {
        set_msg_("invalid args");
        return PAL_SOCK_EINVAL;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* res = NULL;
    int gai = getaddrinfo(bind_host, port, &hints, &res);
    if (gai != 0)
        return map_gai_(gai);

    pal_sock_err last = PAL_SOCK_EIO;

    for (struct addrinfo* it = res; it; it = it->ai_next)
    {
        int fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0)
        {
            set_err_("socket");
            last = PAL_SOCK_EIO;
            continue;
        }

        int yes = 1;
        (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, (socklen_t)sizeof(yes));

#ifdef SO_REUSEPORT
        (void)setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, (socklen_t)sizeof(yes));
#endif

        if (bind(fd, it->ai_addr, it->ai_addrlen) != 0)
        {
            set_err_("bind");
            close(fd);
            last = map_errno_(errno);
            continue;
        }

        if (listen(fd, backlog) != 0)
        {
            set_err_("listen");
            close(fd);
            last = map_errno_(errno);
            continue;
        }

        out->fd = fd;
        last = PAL_SOCK_OK;
        break;
    }

    freeaddrinfo(res);
    return last;
}

pal_sock_err pal_tcp_accept(const pal_sock* listener, pal_sock* out_client, pal_addr* out_peer)
{
    if (out_client) pal_sock_init(out_client);

    if (!listener || listener->fd < 0 || !out_client)
    {
        set_msg_("invalid args");
        return PAL_SOCK_EINVAL;
    }

    struct sockaddr_storage ss;
    socklen_t sl = (socklen_t)sizeof(ss);

    int cfd = accept(listener->fd, (struct sockaddr*)&ss, &sl);
    if (cfd < 0)
    {
        set_err_("accept");
        return map_errno_(errno);
    }

    out_client->fd = cfd;

    if (out_peer)
    {
        memset(out_peer, 0, sizeof(*out_peer));
        memcpy(&out_peer->ss, &ss, sl);
        out_peer->len = sl;
    }

    return PAL_SOCK_OK;
}

pal_sock_err pal_sock_send_all(const pal_sock* s, const void* data, size_t len, uint32_t timeout_ms)
{
    if (!s || s->fd < 0 || (!data && len != 0))
    {
        set_msg_("invalid args");
        return PAL_SOCK_EINVAL;
    }

    const uint8_t* p = (const uint8_t*)data;
    size_t off = 0;

    while (off < len)
    {
        pal_sock_err w = wait_writable_(s->fd, timeout_ms);
        if (w != PAL_SOCK_OK)
            return w;

        ssize_t n = send(s->fd, p + off, len - off, 0);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            set_err_("send");
            return map_errno_(errno);
        }

        off += (size_t)n;
    }

    return PAL_SOCK_OK;
}

pal_sock_err pal_sock_recv_some(const pal_sock* s, void* out_buf, size_t cap, size_t* out_len, uint32_t timeout_ms)
{
    if (out_len) *out_len = 0;

    if (!s || s->fd < 0 || !out_buf || cap == 0)
    {
        set_msg_("invalid args");
        return PAL_SOCK_EINVAL;
    }

    pal_sock_err r = wait_readable_(s->fd, timeout_ms);
    if (r != PAL_SOCK_OK)
        return r;

    ssize_t n = recv(s->fd, out_buf, cap, 0);
    if (n < 0)
    {
        if (errno == EINTR) return PAL_SOCK_EIO;
        set_err_("recv");
        return map_errno_(errno);
    }

    if (out_len) *out_len = (size_t)n;
    return PAL_SOCK_OK;
}

pal_sock_err pal_addr_parse(pal_addr* out, const char* ip, const char* port)
{
    if (!out || !ip || !port)
    {
        set_msg_("invalid args");
        return PAL_SOCK_EINVAL;
    }

    memset(out, 0, sizeof(*out));

    // Try IPv6 first.
    struct sockaddr_in6 a6;
    memset(&a6, 0, sizeof(a6));
    a6.sin6_family = AF_INET6;

    int rc6 = inet_pton(AF_INET6, ip, &a6.sin6_addr);
    if (rc6 == 1)
    {
        long p = strtol(port, NULL, 10);
        if (p < 0 || p > 65535)
        {
            set_msg_("bad port");
            return PAL_SOCK_EINVAL;
        }
        a6.sin6_port = htons((uint16_t)p);

        memcpy(&out->ss, &a6, sizeof(a6));
        out->len = (socklen_t)sizeof(a6);
        return PAL_SOCK_OK;
    }

    // IPv4
    struct sockaddr_in a4;
    memset(&a4, 0, sizeof(a4));
    a4.sin_family = AF_INET;

    int rc4 = inet_pton(AF_INET, ip, &a4.sin_addr);
    if (rc4 == 1)
    {
        long p = strtol(port, NULL, 10);
        if (p < 0 || p > 65535)
        {
            set_msg_("bad port");
            return PAL_SOCK_EINVAL;
        }
        a4.sin_port = htons((uint16_t)p);

        memcpy(&out->ss, &a4, sizeof(a4));
        out->len = (socklen_t)sizeof(a4);
        return PAL_SOCK_OK;
    }

    set_msg_("invalid ip");
    return PAL_SOCK_EINVAL;
}

size_t pal_addr_format(const pal_addr* a, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return 0;
    out[0] = 0;

    if (!a || a->len == 0)
        return 0;

    char host[INET6_ADDRSTRLEN + 8];
    char serv[32];

    int rc = getnameinfo((const struct sockaddr*)&a->ss, a->len,
                         host, sizeof(host),
                         serv, sizeof(serv),
                         NI_NUMERICHOST | NI_NUMERICSERV);

    if (rc != 0)
    {
        const char* s = gai_strerror(rc);
        if (s) set_msg_(s);
        return 0;
    }

    // IPv6 with brackets
    if (a->ss.ss_family == AF_INET6)
    {
        int n = snprintf(out, out_cap, "[%s]:%s", host, serv);
        if (n < 0) { out[0] = 0; return 0; }
        if ((size_t)n >= out_cap) { out[out_cap - 1] = 0; return out_cap - 1; }
        return (size_t)n;
    }

    int n = snprintf(out, out_cap, "%s:%s", host, serv);
    if (n < 0) { out[0] = 0; return 0; }
    if ((size_t)n >= out_cap) { out[out_cap - 1] = 0; return out_cap - 1; }
    return (size_t)n;
}
