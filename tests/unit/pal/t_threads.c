/*
 * tests/unit/pal/t_threads.c
 *
 * Unit tests: PAL threading primitives.
 *
 * Assumed public API (C):
 *   - #include "pal/threads.h"
 *   - pal_thread_t
 *   - pal_thread_create(&t, fn, user)
 *   - pal_thread_join(t)
 *   - pal_mutex_t / pal_mutex_init/lock/unlock/free
 *   - pal_cond_t / pal_cond_init/wait/signal/broadcast/free
 *   - pal_sleep_ms(ms)
 *
 * Adapt names/types if your PAL differs.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "pal/threads.h"

static int g_fail = 0;

static void t_fail(const char* file, int line, const char* expr) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
    g_fail = 1;
}
#define T_ASSERT(x) do { if (!(x)) t_fail(__FILE__, __LINE__, #x); } while (0)

typedef struct {
    pal_mutex_t mu;
    pal_cond_t  cv;
    int         value;
    bool        ready;
} shared_state;

/* Worker increments value and signals readiness. */
static void* worker_main(void* user) {
    shared_state* s = (shared_state*)user;

    pal_mutex_lock(&s->mu);
    s->value += 41;
    s->ready = true;
    pal_cond_signal(&s->cv);
    pal_mutex_unlock(&s->mu);

    return NULL;
}

static void test_thread_create_join(void) {
    shared_state s;
    s.value = 1;
    s.ready = false;

    T_ASSERT(pal_mutex_init(&s.mu) == PAL_OK);
    T_ASSERT(pal_cond_init(&s.cv) == PAL_OK);

    pal_thread_t t;
    T_ASSERT(pal_thread_create(&t, worker_main, &s) == PAL_OK);

    /* Wait until worker signals. */
    pal_mutex_lock(&s.mu);
    while (!s.ready) {
        T_ASSERT(pal_cond_wait(&s.cv, &s.mu) == PAL_OK);
    }
    pal_mutex_unlock(&s.mu);

    T_ASSERT(pal_thread_join(t) == PAL_OK);

    /* Should have been incremented: 1 + 41 = 42 */
    T_ASSERT(s.value == 42);

    pal_cond_free(&s.cv);
    pal_mutex_free(&s.mu);
}

static void test_mutex_contention(void) {
    pal_mutex_t mu;
    T_ASSERT(pal_mutex_init(&mu) == PAL_OK);

    /* Lock/unlock basic */
    T_ASSERT(pal_mutex_lock(&mu) == PAL_OK);
    T_ASSERT(pal_mutex_unlock(&mu) == PAL_OK);

    pal_mutex_free(&mu);
}

static void test_sleep(void) {
    /* Just ensure it doesn't crash and returns OK. */
    T_ASSERT(pal_sleep_ms(10) == PAL_OK);
}

int main(void) {
    test_thread_create_join();
    test_mutex_contention();
    test_sleep();

    if (g_fail) return 1;
    printf("OK: pal threads tests\n");
    return 0;
}
