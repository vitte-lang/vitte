

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// fmt_suite.c
// -----------------------------------------------------------------------------
// Benchmark suite: formatting / pretty-printing hot paths.
//
// Goal:
//  - Measure throughput of a deterministic, compiler-independent formatter that
//    performs work typical of a source formatter:
//      * scan of a byte buffer
//      * whitespace normalization
//      * indentation management
//      * (optional) lightweight wrap handling
//      * output assembly
//
// This file intentionally does NOT depend on the compiler proper.
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Bench API (adapt here if your harness differs)
// -----------------------------------------------------------------------------
#ifndef VITTE_BENCH_FMT_SUITE_API
#define VITTE_BENCH_FMT_SUITE_API 1

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

#if defined(__GNUC__) || defined(__clang__)
#define VITTE_BENCH_CONSTRUCTOR __attribute__((constructor))
#else
#define VITTE_BENCH_CONSTRUCTOR
#endif

// -----------------------------------------------------------------------------
// Blackhole
// -----------------------------------------------------------------------------
static volatile uint64_t g_sink_u64;
static volatile uint8_t  g_sink_u8;

static inline void bench_blackhole_u64(uint64_t v)
{
    g_sink_u64 ^= (v + 0x9e3779b97f4a7c15ULL) ^ (g_sink_u64 << 5) ^ (g_sink_u64 >> 3);
}

static inline void bench_blackhole_bytes(const void* p, size_t n)
{
    const uint8_t* b = (const uint8_t*)p;
    uint8_t acc = (uint8_t)g_sink_u8;
    if (n)
    {
        acc ^= b[0];
        acc ^= b[n >> 1];
        acc ^= b[n - 1];
    }
    g_sink_u8 = (uint8_t)(acc + 0x27);
}

// -----------------------------------------------------------------------------
// Deterministic RNG
// -----------------------------------------------------------------------------
static inline uint64_t xorshift64(uint64_t* s)
{
    uint64_t x = *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x;
    return x;
}

static inline uint32_t u32_range(uint64_t* s, uint32_t lo, uint32_t hi)
{
    const uint32_t span = (hi > lo) ? (hi - lo) : 1u;
    return lo + (uint32_t)(xorshift64(s) % span);
}

// -----------------------------------------------------------------------------
// Tiny string builder
// -----------------------------------------------------------------------------
typedef struct strbuf
{
    char*  p;
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

static inline void sb_putc(strbuf_t* sb, char c)
{
    if (sb->len + 1 >= sb->cap) return;
    sb->p[sb->len++] = c;
    sb->p[sb->len] = '\0';
}

static inline void sb_puts_n(strbuf_t* sb, const char* s, size_t n)
{
    if (!n) return;
    const size_t rem = (sb->cap > sb->len) ? (sb->cap - sb->len) : 0;
    if (rem <= 1) return;
    const size_t w = (n < rem - 1) ? n : (rem - 1);
    memcpy(sb->p + sb->len, s, w);
    sb->len += w;
    sb->p[sb->len] = '\0';
}

static inline void sb_puts(strbuf_t* sb, const char* s)
{
    if (!s) return;
    sb_puts_n(sb, s, strlen(s));
}

static inline void sb_spaces(strbuf_t* sb, uint32_t n)
{
    while (n--)
        sb_putc(sb, ' ');
}

// -----------------------------------------------------------------------------
// Synthetic source generation
// -----------------------------------------------------------------------------
// We generate Vitte-like “phrase” code with irregular whitespace, then our
// formatter normalizes it.

typedef struct src_blob
{
    char*  src;
    size_t len;
} src_blob_t;

enum { kBlobCount = 3 };
static src_blob_t g_blobs[kBlobCount];
static bool g_inited;

static const char* kIdents[] = {
    "alpha", "beta", "gamma", "delta", "epsilon", "omega",
    "Parser", "Lexer", "Token", "Span", "Ast", "Node",
    "Vec", "Map", "Hash", "Result", "Option", "Error",
};

static const char* kTypes[] = {
    "i32", "u32", "i64", "u64", "bool", "str", "f32", "f64",
    "Vec[u8]", "Vec[i32]", "Map[str, i32]", "Option[str]",
};

static const char* kKeywords[] = {
    "fn", "let", "const", "if", "elif", "else", "while", "for", "match",
    "ret", "do", "say", "set", "when", "loop", "type", "struct",
};

static void gen_line(strbuf_t* sb, uint64_t* rng, uint32_t depth)
{
    const uint32_t k = u32_range(rng, 0, 10);

    // Indentation (noisy): tabs/spaces mixed.
    const uint32_t base = depth * 4;
    const uint32_t jitter = u32_range(rng, 0, 4);
    const uint32_t ind = base + jitter;
    for (uint32_t i = 0; i < ind; ++i)
        sb_putc(sb, (xorshift64(rng) & 7u) == 0u ? '\t' : ' ');

    if (k == 0)
    {
        // block open
        sb_puts(sb, "fn");
        sb_putc(sb, (xorshift64(rng) & 1u) ? ' ' : '\t');
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_puts(sb, "(");
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_puts(sb, ":");
        if (xorshift64(rng) & 1u) sb_putc(sb, ' ');
        sb_puts(sb, kTypes[xorshift64(rng) % (sizeof(kTypes)/sizeof(kTypes[0]))]);
        sb_puts(sb, ")");
        sb_puts(sb, (xorshift64(rng) & 1u) ? " -> " : "->");
        sb_puts(sb, kTypes[xorshift64(rng) % (sizeof(kTypes)/sizeof(kTypes[0]))]);
        sb_puts(sb, (xorshift64(rng) & 1u) ? "\n" : "  \n");
        return;
    }

    if (k == 1)
    {
        // end marker
        sb_puts(sb, (xorshift64(rng) & 1u) ? ".end\n" : " .end\n");
        return;
    }

    if (k == 2)
    {
        // let assignment
        sb_puts(sb, "let");
        sb_putc(sb, (xorshift64(rng) & 1u) ? ' ' : '\t');
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        if (xorshift64(rng) & 1u)
        {
            sb_puts(sb, ":");
            if (xorshift64(rng) & 1u) sb_putc(sb, ' ');
            sb_puts(sb, kTypes[xorshift64(rng) % (sizeof(kTypes)/sizeof(kTypes[0]))]);
        }
        sb_puts(sb, (xorshift64(rng) & 1u) ? " = " : "=");
        sb_puts(sb, "(");
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_puts(sb, (xorshift64(rng) & 1u) ? " + " : "+");
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_puts(sb, ")\n");
        return;
    }

    if (k == 3)
    {
        // if condition
        sb_puts(sb, "if");
        sb_putc(sb, (xorshift64(rng) & 1u) ? ' ' : '\t');
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_puts(sb, (xorshift64(rng) & 1u) ? " == " : "==");
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_putc(sb, '\n');
        return;
    }

    if (k == 4)
    {
        // while
        sb_puts(sb, "while");
        sb_putc(sb, ' ');
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_puts(sb, (xorshift64(rng) & 1u) ? " < " : "<");
        char tmp[16];
        const uint32_t n = u32_range(rng, 0, 10000);
        snprintf(tmp, sizeof(tmp), "%u", (unsigned)n);
        sb_puts(sb, tmp);
        sb_putc(sb, '\n');
        return;
    }

    // generic expr/stmt
    {
        const char* kw = kKeywords[xorshift64(rng) % (sizeof(kKeywords)/sizeof(kKeywords[0]))];
        sb_puts(sb, kw);
        if ((xorshift64(rng) & 3u) == 0u) sb_putc(sb, '\t');
        else sb_putc(sb, ' ');
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_puts(sb, (xorshift64(rng) & 1u) ? "(" : " (");
        sb_puts(sb, kIdents[xorshift64(rng) % (sizeof(kIdents)/sizeof(kIdents[0]))]);
        sb_puts(sb, (xorshift64(rng) & 1u) ? ")\n" : ")  \n");
    }
}

static void gen_blob(src_blob_t* b, uint64_t seed, size_t target_bytes)
{
    char* tmp = (char*)malloc(target_bytes + 256);
    if (!tmp) abort();

    strbuf_t sb;
    sb_init(&sb, tmp, target_bytes + 256);

    uint64_t rng = seed;
    uint32_t depth = 0;

    while (sb.len + 128 < sb.cap && sb.len < target_bytes)
    {
        // Bias: open blocks sometimes, close sometimes.
        const uint32_t r = (uint32_t)(xorshift64(&rng) & 15u);
        if (r == 0 && depth < 64) { depth++; }
        if (r == 1 && depth > 0)  { depth--; }

        gen_line(&sb, &rng, depth);

        // Occasionally insert blank lines / trailing spaces.
        if ((xorshift64(&rng) & 31u) == 0u)
            sb_puts(&sb, "\n");
        if ((xorshift64(&rng) & 63u) == 0u)
            sb_puts(&sb, "   \n");
    }

    // Close remaining blocks with .end
    while (depth--)
        sb_puts(&sb, ".end\n");

    b->src = tmp;
    b->len = sb.len;
}

static void init_blobs_once(void)
{
    if (g_inited) return;
    g_inited = true;

    // Small/medium/large.
    gen_blob(&g_blobs[0], 0xF00DF00DULL, 8 * 1024);
    gen_blob(&g_blobs[1], 0x12345678ABCDEF01ULL, 64 * 1024);
    gen_blob(&g_blobs[2], 0xC0FFEEBEEFULL, 512 * 1024);
}

// -----------------------------------------------------------------------------
// Formatter (single-pass, allocation-free)
// -----------------------------------------------------------------------------
// Rules:
//  - Normalize indentation using N spaces per depth.
//  - Collapse runs of whitespace inside a line to single spaces.
//  - Remove trailing spaces.
//  - Keep `.end` on its own line and decrease indent before emitting.
//  - Treat some keywords as block openers (increase indent AFTER line).
//
// Not a real Vitte formatter; it is a stand-in to benchmark typical workload.

typedef struct fmt_cfg
{
    uint32_t indent_spaces;   // e.g. 4
    uint32_t wrap_column;     // 0 = disabled
} fmt_cfg_t;

static inline bool is_space(unsigned char c)
{
    return (c == ' ') || (c == '\t') || (c == '\r');
}

static inline bool is_nl(unsigned char c)
{
    return (c == '\n');
}

static inline bool is_ident_start(unsigned char c)
{
    return (c == '_') || ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static inline bool is_ident_continue(unsigned char c)
{
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

static inline uint64_t fnv1a64(const void* data, size_t n)
{
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; ++i)
    {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static inline bool word_eq(const char* w, size_t n, const char* lit)
{
    const size_t m = strlen(lit);
    return (n == m) && (memcmp(w, lit, m) == 0);
}

static inline bool is_block_opener_word(const char* w, size_t n)
{
    // Increase indent after line for these.
    return word_eq(w, n, "fn") || word_eq(w, n, "if") || word_eq(w, n, "elif") || word_eq(w, n, "else")
        || word_eq(w, n, "while") || word_eq(w, n, "for") || word_eq(w, n, "match")
        || word_eq(w, n, "struct") || word_eq(w, n, "enum") || word_eq(w, n, "union")
        || word_eq(w, n, "scenario") || word_eq(w, n, "program") || word_eq(w, n, "service");
}

static inline bool is_end_marker_word(const char* w, size_t n)
{
    return word_eq(w, n, ".end");
}

static size_t format_one(const fmt_cfg_t* cfg, const char* src, size_t len, char* out, size_t out_cap)
{
    strbuf_t sb;
    sb_init(&sb, out, out_cap);

    uint32_t indent_depth = 0;
    bool at_line_start = true;

    // For per-line behavior.
    const char* first_word = NULL;
    size_t      first_word_len = 0;
    bool        first_word_seen = false;
    bool        pending_space = false;
    bool        line_has_content = false;

    // Trailing spaces trimming.
    size_t line_start_out = 0;
    size_t last_non_space_out = 0;

    // Column tracking for optional wrapping.
    uint32_t col = 0;

    size_t i = 0;
    while (i < len)
    {
        const unsigned char c = (unsigned char)src[i];

        // Normalize newlines.
        if (is_nl(c))
        {
            // Trim trailing spaces in output.
            if (sb.len > line_start_out)
            {
                // keep at least line_start_out
                const size_t trimmed = (last_non_space_out > line_start_out) ? last_non_space_out : line_start_out;
                sb.len = trimmed;
                if (sb.cap) sb.p[sb.len] = '\0';
            }

            sb_putc(&sb, '\n');

            // After emitting the line, adjust indent for next line based on first word.
            if (first_word_seen && first_word && first_word_len)
            {
                if (!is_end_marker_word(first_word, first_word_len) && is_block_opener_word(first_word, first_word_len))
                    indent_depth++;
            }

            // Reset per-line state.
            at_line_start = true;
            pending_space = false;
            line_has_content = false;
            first_word = NULL;
            first_word_len = 0;
            first_word_seen = false;
            line_start_out = sb.len;
            last_non_space_out = sb.len;
            col = 0;

            i++;
            continue;
        }

        // Skip leading whitespace; we will re-emit normalized indentation.
        if (at_line_start)
        {
            while (i < len)
            {
                const unsigned char c2 = (unsigned char)src[i];
                if (is_nl(c2)) break;
                if (!is_space(c2)) break;
                i++;
            }

            if (i >= len) break;
            if (is_nl((unsigned char)src[i]))
            {
                // empty line
                continue;
            }

            // Detect first word for this line before emitting indentation.
            if (!first_word_seen)
            {
                const unsigned char c3 = (unsigned char)src[i];
                if (c3 == '.')
                {
                    size_t j = i;
                    j++; // '.'
                    while (j < len && is_ident_continue((unsigned char)src[j])) j++;
                    first_word = &src[i];
                    first_word_len = (j - i);
                    first_word_seen = true;
                }
                else if (is_ident_start(c3))
                {
                    size_t j = i + 1;
                    while (j < len && is_ident_continue((unsigned char)src[j])) j++;
                    first_word = &src[i];
                    first_word_len = (j - i);
                    first_word_seen = true;
                }
            }

            // `.end` decreases indent BEFORE emitting.
            if (first_word_seen && first_word && first_word_len && is_end_marker_word(first_word, first_word_len))
            {
                if (indent_depth) indent_depth--;
            }

            // Emit normalized indentation.
            const uint32_t spaces = indent_depth * cfg->indent_spaces;
            sb_spaces(&sb, spaces);
            col += spaces;

            at_line_start = false;
        }

        // Inside a line: collapse whitespace.
        if (is_space(c))
        {
            pending_space = true;
            i++;
            continue;
        }

        // If we are about to emit content and we have pending space, emit exactly one.
        if (pending_space && line_has_content)
        {
            // Optional wrap: if adding a space would exceed wrap, insert newline + indent.
            if (cfg->wrap_column && col + 1 >= cfg->wrap_column)
            {
                // Trim trailing spaces before wrap.
                if (sb.len > line_start_out)
                {
                    const size_t trimmed = (last_non_space_out > line_start_out) ? last_non_space_out : line_start_out;
                    sb.len = trimmed;
                    if (sb.cap) sb.p[sb.len] = '\0';
                }

                sb_putc(&sb, '\n');

                // continuation indent: one extra depth
                const uint32_t spaces = (indent_depth + 1) * cfg->indent_spaces;
                sb_spaces(&sb, spaces);

                line_start_out = sb.len - spaces;
                last_non_space_out = sb.len;
                col = spaces;
            }
            else
            {
                sb_putc(&sb, ' ');
                col += 1;
            }
        }
        pending_space = false;

        // Emit the current character.
        sb_putc(&sb, (char)c);
        line_has_content = true;

        if (c != ' ' && c != '\t' && c != '\r')
        {
            last_non_space_out = sb.len;
        }
        col += 1;

        i++;
    }

    // Final trim (if no trailing newline).
    if (sb.len > line_start_out)
    {
        const size_t trimmed = (last_non_space_out > line_start_out) ? last_non_space_out : line_start_out;
        sb.len = trimmed;
        if (sb.cap) sb.p[sb.len] = '\0';
    }

    return sb.len;
}

// -----------------------------------------------------------------------------
// Bench cases
// -----------------------------------------------------------------------------
typedef struct fmt_case_cfg
{
    uint32_t blob_index;
    uint32_t wrap_column;
} fmt_case_cfg_t;

static void bm_fmt_run(uint64_t iters, void* user)
{
    init_blobs_once();

    const fmt_case_cfg_t* cc = (const fmt_case_cfg_t*)user;
    const uint32_t bi = (cc && cc->blob_index < kBlobCount) ? cc->blob_index : 0;

    const char* src = g_blobs[bi].src;
    const size_t src_len = g_blobs[bi].len;

    // Output buffer: worst-case ~ input + indentation growth.
    const size_t out_cap = (src_len * 2) + 1024;
    char* out = (char*)malloc(out_cap);
    if (!out) abort();

    fmt_cfg_t cfg;
    cfg.indent_spaces = 4;
    cfg.wrap_column = (cc) ? cc->wrap_column : 0;

    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        const size_t n = format_one(&cfg, src, src_len, out, out_cap);
        const uint64_t h = fnv1a64(out, n);
        acc ^= (h + (uint64_t)n + (acc << 7) + (acc >> 3));
        bench_blackhole_bytes(out, n);

        // tiny perturbation: flip wrap on/off deterministically (keeps stable)
        if ((i & 127u) == 0u)
            cfg.wrap_column = cfg.wrap_column ? cfg.wrap_column : 0;
    }

    bench_blackhole_u64(acc);
    free(out);
}

static const fmt_case_cfg_t k_small_nowrap = { 0, 0 };
static const fmt_case_cfg_t k_med_nowrap   = { 1, 0 };
static const fmt_case_cfg_t k_large_nowrap = { 2, 0 };

static const fmt_case_cfg_t k_small_wrap80 = { 0, 80 };
static const fmt_case_cfg_t k_med_wrap80   = { 1, 80 };
static const fmt_case_cfg_t k_large_wrap80 = { 2, 80 };

// -----------------------------------------------------------------------------
// Suite registration
// -----------------------------------------------------------------------------
static const bench_case_t k_cases[] = {
    { "small_nowrap", bm_fmt_run, (void*)&k_small_nowrap },
    { "med_nowrap",   bm_fmt_run, (void*)&k_med_nowrap   },
    { "large_nowrap", bm_fmt_run, (void*)&k_large_nowrap },

    { "small_wrap80", bm_fmt_run, (void*)&k_small_wrap80 },
    { "med_wrap80",   bm_fmt_run, (void*)&k_med_wrap80   },
    { "large_wrap80", bm_fmt_run, (void*)&k_large_wrap80 },
};

static const bench_suite_t k_suite = {
    "fmt",
    k_cases,
    sizeof(k_cases) / sizeof(k_cases[0]),
};

VITTE_BENCH_CONSTRUCTOR static void fmt_suite_autoregister(void)
{
    bench_register_suite(&k_suite);
}

// Optional explicit symbol for runners that call suites manually.
void vitte_bench_register_fmt_suite(void)
{
    bench_register_suite(&k_suite);
}
