// SPDX-License-Identifier: MIT
// pal_posix_thread.c
//
// POSIX threads backend (max).
//
// Provides:
//  - threads (create/join/detach)
//  - mutex (init/lock/trylock/unlock/dispose)
//  - rwlock (init/rdlock/wrlock/try/ unlock/dispose)
//  - condvar (init/wait/timedwait/signal/broadcast/dispose)
//  - once
//  - TLS key/value
//  - sleep / yield
//  - current thread id
//
// Notes:
//  - Uses pthreads.
//  - Timed waits use CLOCK_REALTIME for portability.
//  - Provides a fallback API if `pal_thread.h` is not present.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sched.h>

#include <pthread.h>

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("../pal_thread.h")
    #include "../pal_thread.h"
    #define STEEL_HAS_PAL_THREAD_H 1
  #elif __has_include("pal_thread.h")
    #include "pal_thread.h"
    #define STEEL_HAS_PAL_THREAD_H 1
  #endif
#endif

#ifndef STEEL_HAS_PAL_THREAD_H

//------------------------------------------------------------------------------
// Fallback API (align later with pal_thread.h)
//------------------------------------------------------------------------------

typedef struct pal_thread
{
    pthread_t t;
    bool started;
} pal_thread;

typedef void* (*pal_thread_fn)(void* user);

typedef struct pal_mutex
{
    pthread_mutex_t m;
    bool inited;
} pal_mutex;

typedef struct pal_rwlock
{
    pthread_rwlock_t rw;
    bool inited;
} pal_rwlock;

typedef struct pal_cond
{
    pthread_cond_t c;
    bool inited;
} pal_cond;

typedef struct pal_once
{
    pthread_once_t once;
} pal_once;

#define PAL_ONCE_INIT { PTHREAD_ONCE_INIT }

typedef struct pal_tls
{
    pthread_key_t key;
    bool inited;
} pal_tls;

typedef enum pal_thread_err
{
    PAL_THREAD_OK = 0,
    PAL_THREAD_EINVAL,
    PAL_THREAD_EBUSY,
    PAL_THREAD_EIO,
    PAL_THREAD_EOOM,
    PAL_THREAD_ETIMEDOUT,
} pal_thread_err;

void pal_thread_init(pal_thread* t);

pal_thread_err pal_thread_create(pal_thread* t, pal_thread_fn fn, void* user);

pal_thread_err pal_thread_join(pal_thread* t, void** out_ret);

pal_thread_err pal_thread_detach(pal_thread* t);

uint64_t pal_thread_current_id(void);

void pal_thread_yield(void);
void pal_thread_sleep_ms(uint32_t ms);

// mutex
pal_thread_err pal_mutex_init(pal_mutex* m);
void          pal_mutex_dispose(pal_mutex* m);

pal_thread_err pal_mutex_lock(pal_mutex* m);
pal_thread_err pal_mutex_trylock(pal_mutex* m);
pal_thread_err pal_mutex_unlock(pal_mutex* m);

// rwlock
pal_thread_err pal_rwlock_init(pal_rwlock* rw);
void          pal_rwlock_dispose(pal_rwlock* rw);

pal_thread_err pal_rwlock_rdlock(pal_rwlock* rw);
pal_thread_err pal_rwlock_tryrdlock(pal_rwlock* rw);
pal_thread_err pal_rwlock_wrlock(pal_rwlock* rw);
pal_thread_err pal_rwlock_trywrlock(pal_rwlock* rw);
pal_thread_err pal_rwlock_unlock(pal_rwlock* rw);

// cond
pal_thread_err pal_cond_init(pal_cond* c);
void          pal_cond_dispose(pal_cond* c);

pal_thread_err pal_cond_wait(pal_cond* c, pal_mutex* m);
pal_thread_err pal_cond_timedwait_ms(pal_cond* c, pal_mutex* m, uint32_t timeout_ms);
pal_thread_err pal_cond_signal(pal_cond* c);
pal_thread_err pal_cond_broadcast(pal_cond* c);

// once
pal_thread_err pal_once_call(pal_once* o, void (*fn)(void));

// tls
pal_thread_err pal_tls_init(pal_tls* t, void (*destructor)(void*));
void          pal_tls_dispose(pal_tls* t);

pal_thread_err pal_tls_set(pal_tls* t, void* value);
void*         pal_tls_get(pal_tls* t);

const char* pal_thread_last_error(void);

#endif // !STEEL_HAS_PAL_THREAD_H

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_thr_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    // simple copy
    size_t n = strlen(msg);
    if (n >= sizeof(g_thr_err_)) n = sizeof(g_thr_err_) - 1;
    memcpy(g_thr_err_, msg, n);
    g_thr_err_[n] = 0;
}

static void set_errno_(const char* prefix, int e)
{
    const char* s = strerror(e);
    if (!prefix) prefix = "";
    if (!s) s = "";

    if (prefix[0])
        snprintf(g_thr_err_, sizeof(g_thr_err_), "%s: %s", prefix, s);
    else
        snprintf(g_thr_err_, sizeof(g_thr_err_), "%s", s);
}

const char* pal_thread_last_error(void)
{
    return g_thr_err_;
}

static pal_thread_err map_pthread_(int rc)
{
    switch (rc)
    {
        case 0: return PAL_THREAD_OK;
        case EINVAL: return PAL_THREAD_EINVAL;
        case EBUSY: return PAL_THREAD_EBUSY;
        case ETIMEDOUT: return PAL_THREAD_ETIMEDOUT;
        case ENOMEM: return PAL_THREAD_EOOM;
        default: return PAL_THREAD_EIO;
    }
}

//------------------------------------------------------------------------------
// Time helpers
//------------------------------------------------------------------------------

static void timespec_add_ms_(struct timespec* ts, uint32_t ms)
{
    if (!ts) return;

    uint64_t sec = (uint64_t)(ms / 1000);
    uint64_t nsec = (uint64_t)(ms % 1000) * 1000000ull;

    uint64_t s = (uint64_t)ts->tv_sec + sec;
    uint64_t ns = (uint64_t)ts->tv_nsec + nsec;

    s += ns / 1000000000ull;
    ns = ns % 1000000000ull;

    ts->tv_sec = (time_t)s;
    ts->tv_nsec = (long)ns;
}

//------------------------------------------------------------------------------
// Threads
//------------------------------------------------------------------------------

void pal_thread_init(pal_thread* t)
{
    if (!t) return;
    memset(t, 0, sizeof(*t));
}

pal_thread_err pal_thread_create(pal_thread* t, pal_thread_fn fn, void* user)
{
    if (!t || !fn)
    {
        set_msg_("invalid args");
        return PAL_THREAD_EINVAL;
    }

    if (t->started)
    {
        set_msg_("thread already started");
        return PAL_THREAD_EBUSY;
    }

    int rc = pthread_create(&t->t, NULL, (void* (*)(void*))fn, user);
    if (rc != 0)
    {
        set_errno_("pthread_create", rc);
        return map_pthread_(rc);
    }

    t->started = true;
    return PAL_THREAD_OK;
}

pal_thread_err pal_thread_join(pal_thread* t, void** out_ret)
{
    if (!t || !t->started)
    {
        set_msg_("invalid thread");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_join(t->t, out_ret);
    if (rc != 0)
    {
        set_errno_("pthread_join", rc);
        return map_pthread_(rc);
    }

    t->started = false;
    memset(&t->t, 0, sizeof(t->t));

    return PAL_THREAD_OK;
}

pal_thread_err pal_thread_detach(pal_thread* t)
{
    if (!t || !t->started)
    {
        set_msg_("invalid thread");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_detach(t->t);
    if (rc != 0)
    {
        set_errno_("pthread_detach", rc);
        return map_pthread_(rc);
    }

    // detached, caller should not join.
    t->started = false;
    memset(&t->t, 0, sizeof(t->t));

    return PAL_THREAD_OK;
}

uint64_t pal_thread_current_id(void)
{
    // pthread_t is opaque; best-effort stable-ish hash/representation.
    pthread_t self = pthread_self();

#if defined(__APPLE__)
    // On macOS, pthread_threadid_np provides a uint64 thread id.
    uint64_t tid = 0;
    if (pthread_threadid_np(NULL, &tid) == 0)
        return tid;
#endif

    // Fallback: memcpy bytes into u64.
    uint64_t v = 0;
    size_t n = sizeof(self);
    if (n > sizeof(v)) n = sizeof(v);
    memcpy(&v, &self, n);
    return v;
}

void pal_thread_yield(void)
{
    sched_yield();
}

void pal_thread_sleep_ms(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000);
    ts.tv_nsec = (long)((ms % 1000) * 1000000ul);

    while (nanosleep(&ts, &ts) != 0)
    {
        if (errno == EINTR)
            continue;
        break;
    }
}

//------------------------------------------------------------------------------
// Mutex
//------------------------------------------------------------------------------

pal_thread_err pal_mutex_init(pal_mutex* m)
{
    if (!m)
    {
        set_msg_("invalid args");
        return PAL_THREAD_EINVAL;
    }

    memset(m, 0, sizeof(*m));

    pthread_mutexattr_t attr;
    int rc = pthread_mutexattr_init(&attr);
    if (rc != 0)
    {
        set_errno_("pthread_mutexattr_init", rc);
        return map_pthread_(rc);
    }

    // Default is non-recursive.
    rc = pthread_mutex_init(&m->m, &attr);
    pthread_mutexattr_destroy(&attr);

    if (rc != 0)
    {
        set_errno_("pthread_mutex_init", rc);
        return map_pthread_(rc);
    }

    m->inited = true;
    return PAL_THREAD_OK;
}

void pal_mutex_dispose(pal_mutex* m)
{
    if (!m || !m->inited) return;
    (void)pthread_mutex_destroy(&m->m);
    m->inited = false;
}

pal_thread_err pal_mutex_lock(pal_mutex* m)
{
    if (!m || !m->inited)
    {
        set_msg_("invalid mutex");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_mutex_lock(&m->m);
    if (rc != 0)
    {
        set_errno_("pthread_mutex_lock", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_mutex_trylock(pal_mutex* m)
{
    if (!m || !m->inited)
    {
        set_msg_("invalid mutex");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_mutex_trylock(&m->m);
    if (rc != 0)
    {
        set_errno_("pthread_mutex_trylock", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_mutex_unlock(pal_mutex* m)
{
    if (!m || !m->inited)
    {
        set_msg_("invalid mutex");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_mutex_unlock(&m->m);
    if (rc != 0)
    {
        set_errno_("pthread_mutex_unlock", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

//------------------------------------------------------------------------------
// RWLock
//------------------------------------------------------------------------------

pal_thread_err pal_rwlock_init(pal_rwlock* rw)
{
    if (!rw)
    {
        set_msg_("invalid args");
        return PAL_THREAD_EINVAL;
    }

    memset(rw, 0, sizeof(*rw));

    int rc = pthread_rwlock_init(&rw->rw, NULL);
    if (rc != 0)
    {
        set_errno_("pthread_rwlock_init", rc);
        return map_pthread_(rc);
    }

    rw->inited = true;
    return PAL_THREAD_OK;
}

void pal_rwlock_dispose(pal_rwlock* rw)
{
    if (!rw || !rw->inited) return;
    (void)pthread_rwlock_destroy(&rw->rw);
    rw->inited = false;
}

pal_thread_err pal_rwlock_rdlock(pal_rwlock* rw)
{
    if (!rw || !rw->inited)
    {
        set_msg_("invalid rwlock");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_rwlock_rdlock(&rw->rw);
    if (rc != 0)
    {
        set_errno_("pthread_rwlock_rdlock", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_rwlock_tryrdlock(pal_rwlock* rw)
{
    if (!rw || !rw->inited)
    {
        set_msg_("invalid rwlock");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_rwlock_tryrdlock(&rw->rw);
    if (rc != 0)
    {
        set_errno_("pthread_rwlock_tryrdlock", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_rwlock_wrlock(pal_rwlock* rw)
{
    if (!rw || !rw->inited)
    {
        set_msg_("invalid rwlock");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_rwlock_wrlock(&rw->rw);
    if (rc != 0)
    {
        set_errno_("pthread_rwlock_wrlock", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_rwlock_trywrlock(pal_rwlock* rw)
{
    if (!rw || !rw->inited)
    {
        set_msg_("invalid rwlock");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_rwlock_trywrlock(&rw->rw);
    if (rc != 0)
    {
        set_errno_("pthread_rwlock_trywrlock", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_rwlock_unlock(pal_rwlock* rw)
{
    if (!rw || !rw->inited)
    {
        set_msg_("invalid rwlock");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_rwlock_unlock(&rw->rw);
    if (rc != 0)
    {
        set_errno_("pthread_rwlock_unlock", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

//------------------------------------------------------------------------------
// Condvar
//------------------------------------------------------------------------------

pal_thread_err pal_cond_init(pal_cond* c)
{
    if (!c)
    {
        set_msg_("invalid args");
        return PAL_THREAD_EINVAL;
    }

    memset(c, 0, sizeof(*c));

    int rc = pthread_cond_init(&c->c, NULL);
    if (rc != 0)
    {
        set_errno_("pthread_cond_init", rc);
        return map_pthread_(rc);
    }

    c->inited = true;
    return PAL_THREAD_OK;
}

void pal_cond_dispose(pal_cond* c)
{
    if (!c || !c->inited) return;
    (void)pthread_cond_destroy(&c->c);
    c->inited = false;
}

pal_thread_err pal_cond_wait(pal_cond* c, pal_mutex* m)
{
    if (!c || !c->inited || !m || !m->inited)
    {
        set_msg_("invalid cond/mutex");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_cond_wait(&c->c, &m->m);
    if (rc != 0)
    {
        set_errno_("pthread_cond_wait", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_cond_timedwait_ms(pal_cond* c, pal_mutex* m, uint32_t timeout_ms)
{
    if (!c || !c->inited || !m || !m->inited)
    {
        set_msg_("invalid cond/mutex");
        return PAL_THREAD_EINVAL;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    {
        set_errno_("clock_gettime", errno);
        return PAL_THREAD_EIO;
    }

    timespec_add_ms_(&ts, timeout_ms);

    int rc = pthread_cond_timedwait(&c->c, &m->m, &ts);
    if (rc != 0)
    {
        if (rc == ETIMEDOUT)
            return PAL_THREAD_ETIMEDOUT;

        set_errno_("pthread_cond_timedwait", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_cond_signal(pal_cond* c)
{
    if (!c || !c->inited)
    {
        set_msg_("invalid cond");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_cond_signal(&c->c);
    if (rc != 0)
    {
        set_errno_("pthread_cond_signal", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

pal_thread_err pal_cond_broadcast(pal_cond* c)
{
    if (!c || !c->inited)
    {
        set_msg_("invalid cond");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_cond_broadcast(&c->c);
    if (rc != 0)
    {
        set_errno_("pthread_cond_broadcast", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

//------------------------------------------------------------------------------
// Once
//------------------------------------------------------------------------------

typedef struct pal_once_wrap
{
    void (*fn)(void);
} pal_once_wrap;

static void* g_once_fn_slot_ = NULL;
static pthread_mutex_t g_once_mu_ = PTHREAD_MUTEX_INITIALIZER;

static void once_thunk_(void)
{
    void (*fn)(void) = NULL;

    pthread_mutex_lock(&g_once_mu_);
    fn = (void (*)(void))g_once_fn_slot_;
    pthread_mutex_unlock(&g_once_mu_);

    if (fn) fn();
}

pal_thread_err pal_once_call(pal_once* o, void (*fn)(void))
{
    if (!o || !fn)
    {
        set_msg_("invalid args");
        return PAL_THREAD_EINVAL;
    }

    // Store fn in global slot (best-effort). If you need multiple once objects
    // concurrently, move to per-once storage in pal_thread.h.
    pthread_mutex_lock(&g_once_mu_);
    g_once_fn_slot_ = (void*)fn;
    pthread_mutex_unlock(&g_once_mu_);

    int rc = pthread_once(&o->once, once_thunk_);
    if (rc != 0)
    {
        set_errno_("pthread_once", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

//------------------------------------------------------------------------------
// TLS
//------------------------------------------------------------------------------

pal_thread_err pal_tls_init(pal_tls* t, void (*destructor)(void*))
{
    if (!t)
    {
        set_msg_("invalid args");
        return PAL_THREAD_EINVAL;
    }

    memset(t, 0, sizeof(*t));

    int rc = pthread_key_create(&t->key, destructor);
    if (rc != 0)
    {
        set_errno_("pthread_key_create", rc);
        return map_pthread_(rc);
    }

    t->inited = true;
    return PAL_THREAD_OK;
}

void pal_tls_dispose(pal_tls* t)
{
    if (!t || !t->inited) return;
    (void)pthread_key_delete(t->key);
    t->inited = false;
}

pal_thread_err pal_tls_set(pal_tls* t, void* value)
{
    if (!t || !t->inited)
    {
        set_msg_("invalid tls");
        return PAL_THREAD_EINVAL;
    }

    int rc = pthread_setspecific(t->key, value);
    if (rc != 0)
    {
        set_errno_("pthread_setspecific", rc);
        return map_pthread_(rc);
    }

    return PAL_THREAD_OK;
}

void* pal_tls_get(pal_tls* t)
{
    if (!t || !t->inited)
        return NULL;
    return pthread_getspecific(t->key);
}
