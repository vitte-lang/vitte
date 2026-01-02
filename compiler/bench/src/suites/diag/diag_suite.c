

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// diag_suite.c
// -----------------------------------------------------------------------------
// Benchmark suite: diagnostics-related hot paths (formatting + span mapping).
//
// Design goals:
//  - Zero dependencies on the compiler proper (no diag subsystem coupling).
//  - Stress representative work: message formatting, span->(line,col) mapping,
//    and small/large payload assembly.
//  - Keep results stable (deterministic RNG, fixed inputs).
//
// Integration note:
//  This file registers itself through a tiny "bench" API surface:
//
//      typedef void (*bench_fn)(uint64_t iters, void* user);
//      struct bench_case  { const char* name; bench_fn fn; void* user; };
//      struct bench_suite { const char* name; const struct bench_case* cases; size_t case_count; };
//      void bench_register_suite(const bench_suite_t*);
//
//  If your bench harness uses a different API, adapt the declarations in the
//  "Bench API" section below (only that section should need edits).
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// -----------------------------------------------------------------------------
// Bench API (adapt here if your harness differs)
// -----------------------------------------------------------------------------
#ifndef VITTE_BENCH_DIAG_SUITE_API
#define VITTE_BENCH_DIAG_SUITE_API 1

typedef void (*bench_fn)(uint64_t iters, void* user);

typedef struct bench_case
{
    const char* name;
    bench_fn fn;
    void* user;
} bench_case_t;

typedef struct bench_suite
{
    const char* name;
    const bench_case_t* cases;
    size_t case_count;
} bench_suite_t;

// Provided by the bench runner.
void bench_register_suite(const bench_suite_t* suite);

#endif

// Auto-register if available.
#if defined(__GNUC__) || defined(__clang__)
#define VITTE_BENCH_CONSTRUCTOR __attribute__((constructor))
#else
#define VITTE_BENCH_CONSTRUCTOR
#endif

// -----------------------------------------------------------------------------
// Micro helpers
// -----------------------------------------------------------------------------
static volatile uint64_t g_sink_u64;
static volatile uint8_t  g_sink_u8;

static inline void bench_blackhole_u64(uint64_t v)
{
    // mix into a volatile sink to inhibit dead-code elimination
    g_sink_u64 ^= (v + 0x9e3779b97f4a7c15ULL) ^ (g_sink_u64 << 7) ^ (g_sink_u64 >> 3);
}

static inline void bench_blackhole_bytes(const void* p, size_t n)
{
    const uint8_t* b = (const uint8_t*)p;
    uint8_t acc = (uint8_t)g_sink_u8;
    // Sample a few bytes (cheap, deterministic).
    if (n)
    {
        acc ^= b[0];
        acc ^= b[n >> 1];
        acc ^= b[n - 1];
    }
    g_sink_u8 = (uint8_t)(acc + 0x3d);
}

static inline uint64_t xorshift64(uint64_t* state)
{
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static inline uint32_t u32_range(uint64_t* state, uint32_t lo, uint32_t hi)
{
    // inclusive lo, exclusive hi
    const uint32_t span = (hi > lo) ? (hi - lo) : 1u;
    return lo + (uint32_t)(xorshift64(state) % span);
}

// -----------------------------------------------------------------------------
// Synthetic diagnostic model
// -----------------------------------------------------------------------------
typedef enum diag_severity
{
    DIAG_NOTE = 0,
    DIAG_HELP = 1,
    DIAG_WARNING = 2,
    DIAG_ERROR = 3,
} diag_severity_t;

typedef struct diag_span
{
    uint32_t file_id;
    uint32_t start;  // byte offset in file
    uint32_t end;    // byte offset in file
} diag_span_t;

typedef struct diag_record
{
    diag_severity_t sev;
    uint32_t code;
    diag_span_t span;
    const char* msg;
    const char* note;
    const char* help;
} diag_record_t;

static const char* sev_str(diag_severity_t s)
{
    switch (s)
    {
        case DIAG_NOTE: return "note";
        case DIAG_HELP: return "help";
        case DIAG_WARNING: return "warning";
        case DIAG_ERROR: return "error";
        default: return "unknown";
    }
}

// -----------------------------------------------------------------------------
// Synthetic "source file" data + span mapping
// -----------------------------------------------------------------------------
enum
{
    kFileCount = 3,
    kSourceBytes = 256 * 1024,
    kMaxLines = 32 * 1024,

    kRecords = 4096,
};

typedef struct file_map
{
    const char* path;
    char* src;
    size_t src_len;

    uint32_t* line_offsets; // offsets of each line start
    uint32_t line_count;
} file_map_t;

static file_map_t g_files[kFileCount];
static diag_record_t g_records[kRecords];
static bool g_inited;

static void init_sources_once(void)
{
    if (g_inited) return;
    g_inited = true;

    // Paths
    g_files[0].path = "src/main.vitte";
    g_files[1].path = "src/lib/parse.vitte";
    g_files[2].path = "std/cli/args.vitte";

    // Allocate sources + line tables.
    for (uint32_t fi = 0; fi < kFileCount; ++fi)
    {
        g_files[fi].src_len = kSourceBytes;
        g_files[fi].src = (char*)malloc(g_files[fi].src_len);
        g_files[fi].line_offsets = (uint32_t*)malloc(sizeof(uint32_t) * kMaxLines);
        g_files[fi].line_count = 0;

        // Fill with deterministic pseudo-code; insert newlines periodically.
        uint64_t rng = 0xC0FFEE123456789ULL ^ (uint64_t)fi;
        uint32_t line = 0;
        uint32_t off = 0;
        g_files[fi].line_offsets[line++] = 0;

        while (off + 64 < g_files[fi].src_len && line + 1 < kMaxLines)
        {
            // Emit a small line of text.
            uint32_t w = u32_range(&rng, 20, 60);
            for (uint32_t i = 0; i < w && off + 2 < g_files[fi].src_len; ++i)
            {
                const char c = (char)('a' + (xorshift64(&rng) % 26));
                g_files[fi].src[off++] = c;
            }
            g_files[fi].src[off++] = '\n';
            g_files[fi].line_offsets[line++] = off;
        }

        // Pad remaining bytes (no newlines) to keep mapping realistic.
        while (off < g_files[fi].src_len)
        {
            g_files[fi].src[off++] = 'x';
        }

        g_files[fi].line_count = line;
    }

    // Diagnostic messages corpus.
    static const char* kMsgs[] = {
        "unexpected token",
        "expected identifier",
        "unterminated string literal",
        "type mismatch",
        "cannot infer type parameter",
        "use of moved value",
        "unresolved import",
        "invalid UTF-8 sequence",
        "duplicate definition",
        "this pattern is unreachable",
    };

    static const char* kNotes[] = {
        "while parsing this expression",
        "this originates in a macro expansion",
        "required by this bound",
        "because of this dereference",
        "the value is borrowed here",
        "previous definition is here",
        "consider adding an explicit type",
        "this argument is never used",
    };

    static const char* kHelps[] = {
        "try removing this character",
        "consider importing the module",
        "add a semicolon here",
        "wrap the expression in parentheses",
        "use `as` to cast explicitly",
        "did you mean to call this function?",
    };

    uint64_t rng = 0x9BADC0DE12345678ULL;
    for (uint32_t i = 0; i < kRecords; ++i)
    {
        diag_record_t* r = &g_records[i];
        const uint32_t fi = u32_range(&rng, 0, kFileCount);
        const uint32_t start = u32_range(&rng, 0, (uint32_t)g_files[fi].src_len - 32);
        const uint32_t len = u32_range(&rng, 1, 48);

        r->sev = (diag_severity_t)u32_range(&rng, 0, 4);
        r->code = 1000u + (uint32_t)(xorshift64(&rng) % 9000u);
        r->span.file_id = fi;
        r->span.start = start;
        r->span.end = start + len;
        r->msg = kMsgs[xorshift64(&rng) % (sizeof(kMsgs) / sizeof(kMsgs[0]))];

        // Keep some fields NULL to emulate common fast-paths.
        r->note = ((xorshift64(&rng) & 3u) == 0u)
            ? kNotes[xorshift64(&rng) % (sizeof(kNotes) / sizeof(kNotes[0]))]
            : NULL;
        r->help = ((xorshift64(&rng) & 7u) == 0u)
            ? kHelps[xorshift64(&rng) % (sizeof(kHelps) / sizeof(kHelps[0]))]
            : NULL;
    }
}

static inline uint32_t upper_bound_u32(const uint32_t* a, uint32_t n, uint32_t key)
{
    // first index i such that a[i] > key
    uint32_t lo = 0, hi = n;
    while (lo < hi)
    {
        const uint32_t mid = lo + ((hi - lo) >> 1);
        if (a[mid] <= key) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

static void span_to_linecol(const file_map_t* fm, uint32_t off, uint32_t* out_line1, uint32_t* out_col1)
{
    // off is byte offset. We map to 1-based (line, col).
    const uint32_t lc = fm->line_count;
    const uint32_t idx = (lc == 0) ? 0 : (upper_bound_u32(fm->line_offsets, lc, off) - 1u);
    const uint32_t line0 = idx;
    const uint32_t line_start = fm->line_offsets[line0];
    const uint32_t col0 = (off >= line_start) ? (off - line_start) : 0;

    *out_line1 = line0 + 1u;
    *out_col1 = col0 + 1u;
}

// -----------------------------------------------------------------------------
// Formatting
// -----------------------------------------------------------------------------
typedef struct strbuf
{
    char* p;
    size_t cap;
    size_t len;
} strbuf_t;

static void sb_init(strbuf_t* sb, char* dst, size_t cap)
{
    sb->p = dst;
    sb->cap = cap;
    sb->len = 0;
    if (cap) sb->p[0] = '\0';
}

static void sb_puts(strbuf_t* sb, const char* s)
{
    if (!s) return;
    const size_t n = strlen(s);
    const size_t rem = (sb->cap > sb->len) ? (sb->cap - sb->len) : 0;
    if (rem == 0) return;
    const size_t w = (n < rem - 1) ? n : (rem - 1);
    memcpy(sb->p + sb->len, s, w);
    sb->len += w;
    sb->p[sb->len] = '\0';
}

static void sb_printf(strbuf_t* sb, const char* fmt, ...)
{
    const size_t rem = (sb->cap > sb->len) ? (sb->cap - sb->len) : 0;
    if (rem == 0) return;

    va_list ap;
    va_start(ap, fmt);
    const int w = vsnprintf(sb->p + sb->len, rem, fmt, ap);
    va_end(ap);

    if (w <= 0) return;
    const size_t uw = (size_t)w;
    sb->len += (uw < rem) ? uw : (rem - 1);
}

static size_t format_diag_compact(const diag_record_t* r, char* out, size_t cap)
{
    const file_map_t* fm = &g_files[r->span.file_id % kFileCount];

    uint32_t line1 = 0, col1 = 0;
    span_to_linecol(fm, r->span.start, &line1, &col1);

    strbuf_t sb;
    sb_init(&sb, out, cap);

    // path:line:col: severity[code]: message
    sb_printf(&sb, "%s:%u:%u: %s[%u]: ", fm->path, (unsigned)line1, (unsigned)col1, sev_str(r->sev), (unsigned)r->code);
    sb_puts(&sb, r->msg);

    return sb.len;
}

static size_t format_diag_verbose(const diag_record_t* r, char* out, size_t cap)
{
    const file_map_t* fm = &g_files[r->span.file_id % kFileCount];

    uint32_t line1 = 0, col1 = 0;
    span_to_linecol(fm, r->span.start, &line1, &col1);

    strbuf_t sb;
    sb_init(&sb, out, cap);

    sb_printf(&sb, "%s:%u:%u: %s[%u]: ", fm->path, (unsigned)line1, (unsigned)col1, sev_str(r->sev), (unsigned)r->code);
    sb_puts(&sb, r->msg);

    // Add a pseudo code-frame line.
    sb_puts(&sb, "\n  |");
    sb_puts(&sb, "\n");

    // Fetch the line.
    const uint32_t lc = fm->line_count;
    const uint32_t idx = (lc == 0) ? 0 : (upper_bound_u32(fm->line_offsets, lc, r->span.start) - 1u);
    const uint32_t ls = fm->line_offsets[idx];
    const uint32_t le = (idx + 1u < lc) ? fm->line_offsets[idx + 1u] : (uint32_t)fm->src_len;
    const uint32_t line_len = (le > ls) ? (le - ls) : 0u;

    // Print line number + content.
    sb_printf(&sb, "%4u | ", (unsigned)(idx + 1u));
    if (line_len)
    {
        // Copy without trailing newline (if any).
        uint32_t copy_len = line_len;
        if (copy_len && fm->src[ls + copy_len - 1u] == '\n') copy_len -= 1u;
        const size_t rem = (sb.cap > sb.len) ? (sb.cap - sb.len) : 0;
        const size_t w = (copy_len < rem - 1) ? (size_t)copy_len : (rem - 1);
        if (w)
        {
            memcpy(sb.p + sb.len, fm->src + ls, w);
            sb.len += w;
            sb.p[sb.len] = '\0';
        }
    }

    // Caret underline.
    sb_puts(&sb, "\n     | ");
    for (uint32_t i = 1; i < col1 && sb.len + 1 < sb.cap; ++i) sb_puts(&sb, " ");
    sb_puts(&sb, "^\n");

    if (r->note)
    {
        sb_puts(&sb, "note: ");
        sb_puts(&sb, r->note);
        sb_puts(&sb, "\n");
    }

    if (r->help)
    {
        sb_puts(&sb, "help: ");
        sb_puts(&sb, r->help);
        sb_puts(&sb, "\n");
    }

    return sb.len;
}

// -----------------------------------------------------------------------------
// Bench cases
// -----------------------------------------------------------------------------
static void bm_diag_format_compact(uint64_t iters, void* user)
{
    (void)user;
    init_sources_once();

    char buf[256];
    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        const diag_record_t* r = &g_records[(size_t)(i & (kRecords - 1))];
        const size_t n = format_diag_compact(r, buf, sizeof(buf));
        acc += (uint64_t)n;
        bench_blackhole_bytes(buf, n);
    }

    bench_blackhole_u64(acc);
}

static void bm_diag_format_verbose(uint64_t iters, void* user)
{
    (void)user;
    init_sources_once();

    char buf[2048];
    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        const diag_record_t* r = &g_records[(size_t)(i & (kRecords - 1))];
        const size_t n = format_diag_verbose(r, buf, sizeof(buf));
        acc += (uint64_t)n;
        bench_blackhole_bytes(buf, n);
    }

    bench_blackhole_u64(acc);
}

static void bm_diag_span_map(uint64_t iters, void* user)
{
    (void)user;
    init_sources_once();

    uint64_t rng = 0xA11CE5EEDULL;
    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        const uint32_t fi = u32_range(&rng, 0, kFileCount);
        const file_map_t* fm = &g_files[fi];
        const uint32_t off = u32_range(&rng, 0, (uint32_t)fm->src_len);

        uint32_t line1 = 0, col1 = 0;
        span_to_linecol(fm, off, &line1, &col1);

        acc += (uint64_t)line1 * 1315423911ULL + (uint64_t)col1;
    }

    bench_blackhole_u64(acc);
}

static void bm_diag_json_payload(uint64_t iters, void* user)
{
    (void)user;
    init_sources_once();

    // Build a small JSON-ish payload string (no real JSON library on purpose).
    char buf[512];
    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        const diag_record_t* r = &g_records[(size_t)(i & (kRecords - 1))];
        const file_map_t* fm = &g_files[r->span.file_id % kFileCount];
        uint32_t line1 = 0, col1 = 0;
        span_to_linecol(fm, r->span.start, &line1, &col1);

        strbuf_t sb;
        sb_init(&sb, buf, sizeof(buf));

        sb_puts(&sb, "{\"path\":\"");
        sb_puts(&sb, fm->path);
        sb_puts(&sb, "\",\"line\":");
        sb_printf(&sb, "%u", (unsigned)line1);
        sb_puts(&sb, ",\"col\":");
        sb_printf(&sb, "%u", (unsigned)col1);
        sb_puts(&sb, ",\"sev\":\"");
        sb_puts(&sb, sev_str(r->sev));
        sb_puts(&sb, "\",\"code\":");
        sb_printf(&sb, "%u", (unsigned)r->code);
        sb_puts(&sb, ",\"msg\":\"");
        sb_puts(&sb, r->msg);
        sb_puts(&sb, "\"}");

        acc += (uint64_t)sb.len;
        bench_blackhole_bytes(buf, sb.len);
    }

    bench_blackhole_u64(acc);
}

// -----------------------------------------------------------------------------
// Suite registration
// -----------------------------------------------------------------------------
static const bench_case_t k_cases[] = {
    { "format_compact", bm_diag_format_compact, NULL },
    { "format_verbose", bm_diag_format_verbose, NULL },
    { "span_map",       bm_diag_span_map,       NULL },
    { "json_payload",   bm_diag_json_payload,   NULL },
};

static const bench_suite_t k_suite = {
    "diag",
    k_cases,
    sizeof(k_cases) / sizeof(k_cases[0]),
};

VITTE_BENCH_CONSTRUCTOR static void diag_suite_autoregister(void)
{
    // If the bench runner links this object file, it will be registered.
    bench_register_suite(&k_suite);
}

// Optional explicit symbol for runners that call suites manually.
void vitte_bench_register_diag_suite(void)
{
    bench_register_suite(&k_suite);
}