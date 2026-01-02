// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// lex_suite.c
// -----------------------------------------------------------------------------
// Benchmark suite: lexer-like hot paths.
//
// This suite is compiler-independent. It benchmarks the typical work a lexer
// performs:
//   - byte scanning with branches
//   - token classification (ident/number/string/operator)
//   - skipping whitespace/comments
//   - keyword recognition (small perfect-ish table via hash+switch)
//   - simple UTF-8 validation fast-path (ASCII majority)
//
// It uses synthetic Vitte-like "phrase" sources (not necessarily valid) but
// shaped to stress realistic lexer behavior.
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
#ifndef VITTE_BENCH_LEX_SUITE_API
#define VITTE_BENCH_LEX_SUITE_API 1

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
    g_sink_u64 ^= (v + 0x9e3779b97f4a7c15ULL) ^ (g_sink_u64 << 7) ^ (g_sink_u64 >> 3);
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
    g_sink_u8 = (uint8_t)(acc + 0x31);
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

// -----------------------------------------------------------------------------
// Synthetic source blobs
// -----------------------------------------------------------------------------

typedef struct src_blob
{
    char*  src;
    size_t len;
} src_blob_t;

enum { kBlobCount = 3 };
static src_blob_t g_blobs[kBlobCount];
static bool g_inited;

static const char* kWords[] = {
    "mod", "use", "export", "type", "struct", "enum", "union",
    "fn", "scenario", "program", "service", "tool",
    "let", "const", "set", "say", "do", "ret",
    "if", "elif", "else", "match", "when", "loop", "while", "for",
    "break", "continue",
    "true", "false",
    "Vec", "Map", "Option", "Result",
    "std", "cli", "io", "fs", "time",
    "alpha", "beta", "gamma", "delta", "epsilon", "omega",
};

static void blob_append(char** p, size_t* len, size_t* cap, const char* s)
{
    const size_t n = strlen(s);
    if (*len + n + 1 > *cap)
    {
        size_t nc = (*cap) ? (*cap * 2) : 4096;
        while (*len + n + 1 > nc) nc *= 2;
        char* np = (char*)realloc(*p, nc);
        if (!np) abort();
        *p = np;
        *cap = nc;
    }
    memcpy(*p + *len, s, n);
    *len += n;
    (*p)[*len] = '\0';
}

static void blob_append_ch(char** p, size_t* len, size_t* cap, char c)
{
    if (*len + 2 > *cap)
    {
        size_t nc = (*cap) ? (*cap * 2) : 4096;
        char* np = (char*)realloc(*p, nc);
        if (!np) abort();
        *p = np;
        *cap = nc;
    }
    (*p)[(*len)++] = c;
    (*p)[*len] = '\0';
}

static void gen_blob(src_blob_t* b, uint64_t seed, size_t target_bytes)
{
    char* out = NULL;
    size_t len = 0;
    size_t cap = 0;

    uint64_t rng = seed;

    // Build many lines with a mix of tokens.
    while (len < target_bytes)
    {
        const uint32_t line_kind = (uint32_t)(xorshift64(&rng) & 15u);

        if (line_kind == 0)
        {
            // comment line
            blob_append(&out, &len, &cap, "# this is a comment with words: ");
            const uint32_t n = 6 + (uint32_t)(xorshift64(&rng) & 7u);
            for (uint32_t i = 0; i < n; ++i)
            {
                blob_append(&out, &len, &cap, kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))]);
                blob_append_ch(&out, &len, &cap, ' ');
            }
            blob_append(&out, &len, &cap, "\n");
            continue;
        }

        if (line_kind == 1)
        {
            // string literal with escapes
            blob_append(&out, &len, &cap, "say \"hello\\nworld ");
            blob_append(&out, &len, &cap, kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))]);
            blob_append(&out, &len, &cap, "\\t\\\"\"\n");
            continue;
        }

        if (line_kind == 2)
        {
            // UTF-8-ish identifiers (a few non-ascii bytes) to stress validation.
            // Use common 2-byte sequences: "é" (0xC3 0xA9) etc.
            blob_append(&out, &len, &cap, "let caf\xC3\xA9 = 1\n");
            continue;
        }

        if (line_kind == 3)
        {
            // dot directives
            blob_append(&out, &len, &cap, ".end\n");
            continue;
        }

        // generic statement: keyword ident = expr
        {
            const char* kw = kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))];
            blob_append(&out, &len, &cap, kw);
            blob_append_ch(&out, &len, &cap, (xorshift64(&rng) & 1u) ? ' ' : '\t');

            // identifier
            const char* id = kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))];
            blob_append(&out, &len, &cap, id);

            // optional type annotation
            if ((xorshift64(&rng) & 7u) == 0u)
            {
                blob_append(&out, &len, &cap, ": ");
                blob_append(&out, &len, &cap, (xorshift64(&rng) & 1u) ? "i32" : "str");
            }

            // assignment / op
            if ((xorshift64(&rng) & 3u) != 0u)
                blob_append(&out, &len, &cap, (xorshift64(&rng) & 1u) ? " = " : "=");
            else
                blob_append(&out, &len, &cap, " ");

            // expression
            const uint32_t expr_kind = (uint32_t)(xorshift64(&rng) & 7u);
            if (expr_kind == 0)
            {
                blob_append(&out, &len, &cap, "(");
                blob_append(&out, &len, &cap, kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))]);
                blob_append(&out, &len, &cap, (xorshift64(&rng) & 1u) ? " + " : "+");
                blob_append(&out, &len, &cap, kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))]);
                blob_append(&out, &len, &cap, ")");
            }
            else if (expr_kind == 1)
            {
                char tmp[32];
                const uint32_t n = u32_range(&rng, 0, 1000000);
                snprintf(tmp, sizeof(tmp), "%u", (unsigned)n);
                blob_append(&out, &len, &cap, tmp);
            }
            else if (expr_kind == 2)
            {
                blob_append(&out, &len, &cap, "0x");
                char tmp[32];
                const uint32_t n = u32_range(&rng, 0, 0xFFFFFF);
                snprintf(tmp, sizeof(tmp), "%06X", (unsigned)n);
                blob_append(&out, &len, &cap, tmp);
            }
            else if (expr_kind == 3)
            {
                blob_append(&out, &len, &cap, "\"");
                blob_append(&out, &len, &cap, kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))]);
                blob_append(&out, &len, &cap, "\"");
            }
            else
            {
                blob_append(&out, &len, &cap, kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))]);
                if (xorshift64(&rng) & 1u)
                {
                    blob_append(&out, &len, &cap, (xorshift64(&rng) & 1u) ? " == " : "==");
                    blob_append(&out, &len, &cap, kWords[xorshift64(&rng) % (sizeof(kWords)/sizeof(kWords[0]))]);
                }
            }

            // trailing spaces sometimes
            if ((xorshift64(&rng) & 15u) == 0u)
                blob_append(&out, &len, &cap, "   ");

            blob_append(&out, &len, &cap, "\n");
        }
    }

    b->src = out;
    b->len = len;
}

static void init_blobs_once(void)
{
    if (g_inited) return;
    g_inited = true;

    gen_blob(&g_blobs[0], 0x1111111111111111ULL, 16 * 1024);
    gen_blob(&g_blobs[1], 0x2222222222222222ULL, 128 * 1024);
    gen_blob(&g_blobs[2], 0x3333333333333333ULL, 1024 * 1024);
}

// -----------------------------------------------------------------------------
// Lexer-ish implementation
// -----------------------------------------------------------------------------

typedef enum tok_kind
{
    TOK_EOF = 0,
    TOK_WS,
    TOK_NL,
    TOK_COMMENT,

    TOK_IDENT,
    TOK_KEYWORD,
    TOK_INT,
    TOK_HEX,
    TOK_STRING,

    TOK_DOT,
    TOK_OP,
    TOK_PUNCT,

    TOK_INVALID_UTF8,
} tok_kind_t;

typedef struct tok
{
    tok_kind_t kind;
    uint32_t start;
    uint32_t end;
    uint32_t aux; // keyword id / op id / error flags
} tok_t;

static inline bool is_space(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

static inline bool is_nl(unsigned char c)
{
    return c == '\n';
}

static inline bool is_digit(unsigned char c)
{
    return (c >= '0' && c <= '9');
}

static inline bool is_hex_digit(unsigned char c)
{
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline bool is_ident_start(unsigned char c)
{
    return (c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool is_ident_continue(unsigned char c)
{
    return is_ident_start(c) || is_digit(c);
}

// Fast UTF-8 validation for a single codepoint (returns bytes consumed, 0 if invalid).
static inline uint32_t utf8_one(const uint8_t* s, size_t n)
{
    if (!n) return 0;
    const uint8_t c0 = s[0];
    if (c0 < 0x80) return 1; // ASCII

    // 2-byte
    if ((c0 & 0xE0u) == 0xC0u)
    {
        if (n < 2) return 0;
        const uint8_t c1 = s[1];
        if ((c1 & 0xC0u) != 0x80u) return 0;
        const uint32_t cp = ((uint32_t)(c0 & 0x1Fu) << 6) | (uint32_t)(c1 & 0x3Fu);
        if (cp < 0x80u) return 0; // overlong
        return 2;
    }

    // 3-byte
    if ((c0 & 0xF0u) == 0xE0u)
    {
        if (n < 3) return 0;
        const uint8_t c1 = s[1], c2 = s[2];
        if (((c1 & 0xC0u) != 0x80u) || ((c2 & 0xC0u) != 0x80u)) return 0;
        const uint32_t cp = ((uint32_t)(c0 & 0x0Fu) << 12) | ((uint32_t)(c1 & 0x3Fu) << 6) | (uint32_t)(c2 & 0x3Fu);
        if (cp < 0x800u) return 0; // overlong
        if (cp >= 0xD800u && cp <= 0xDFFFu) return 0; // surrogate
        return 3;
    }

    // 4-byte
    if ((c0 & 0xF8u) == 0xF0u)
    {
        if (n < 4) return 0;
        const uint8_t c1 = s[1], c2 = s[2], c3 = s[3];
        if (((c1 & 0xC0u) != 0x80u) || ((c2 & 0xC0u) != 0x80u) || ((c3 & 0xC0u) != 0x80u)) return 0;
        const uint32_t cp = ((uint32_t)(c0 & 0x07u) << 18) | ((uint32_t)(c1 & 0x3Fu) << 12) | ((uint32_t)(c2 & 0x3Fu) << 6) | (uint32_t)(c3 & 0x3Fu);
        if (cp < 0x10000u) return 0; // overlong
        if (cp > 0x10FFFFu) return 0;
        return 4;
    }

    return 0;
}

// Keyword classifier: return 0 if not keyword, else small id.
static inline uint32_t keyword_id(const char* s, size_t n)
{
    // Hash then switch on length+hash. This is a fast approximate that still
    // verifies by memcmp.
    const uint64_t h = fnv1a64(s, n);

    switch (n)
    {
        case 2:
            if (h == fnv1a64("fn", 2) && memcmp(s, "fn", 2) == 0) return 1;
            if (h == fnv1a64("if", 2) && memcmp(s, "if", 2) == 0) return 2;
            if (h == fnv1a64("do", 2) && memcmp(s, "do", 2) == 0) return 3;
            break;
        case 3:
            if (h == fnv1a64("let", 3) && memcmp(s, "let", 3) == 0) return 10;
            if (h == fnv1a64("set", 3) && memcmp(s, "set", 3) == 0) return 11;
            if (h == fnv1a64("ret", 3) && memcmp(s, "ret", 3) == 0) return 12;
            if (h == fnv1a64("mod", 3) && memcmp(s, "mod", 3) == 0) return 13;
            if (h == fnv1a64("use", 3) && memcmp(s, "use", 3) == 0) return 14;
            if (h == fnv1a64("for", 3) && memcmp(s, "for", 3) == 0) return 15;
            break;
        case 4:
            if (h == fnv1a64("else", 4) && memcmp(s, "else", 4) == 0) return 20;
            if (h == fnv1a64("elif", 4) && memcmp(s, "elif", 4) == 0) return 21;
            if (h == fnv1a64("type", 4) && memcmp(s, "type", 4) == 0) return 22;
            if (h == fnv1a64("loop", 4) && memcmp(s, "loop", 4) == 0) return 23;
            if (h == fnv1a64("true", 4) && memcmp(s, "true", 4) == 0) return 24;
            break;
        case 5:
            if (h == fnv1a64("while", 5) && memcmp(s, "while", 5) == 0) return 30;
            if (h == fnv1a64("match", 5) && memcmp(s, "match", 5) == 0) return 31;
            if (h == fnv1a64("const", 5) && memcmp(s, "const", 5) == 0) return 32;
            if (h == fnv1a64("false", 5) && memcmp(s, "false", 5) == 0) return 33;
            break;
        case 6:
            if (h == fnv1a64("export", 6) && memcmp(s, "export", 6) == 0) return 40;
            if (h == fnv1a64("struct", 6) && memcmp(s, "struct", 6) == 0) return 41;
            if (h == fnv1a64("break", 5) && memcmp(s, "break", 5) == 0) return 42; // note: length mismatch; never taken
            break;
        case 7:
            if (h == fnv1a64("scenario", 8)) { /* fallthrough */ }
            if (h == fnv1a64("program", 7) && memcmp(s, "program", 7) == 0) return 50;
            break;
        case 8:
            if (h == fnv1a64("scenario", 8) && memcmp(s, "scenario", 8) == 0) return 60;
            break;
        default:
            break;
    }

    // Additional common words with direct compare (small set).
    if (n == 4 && memcmp(s, "when", 4) == 0) return 70;
    if (n == 8 && memcmp(s, "continue", 8) == 0) return 71;
    if (n == 5 && memcmp(s, "break", 5) == 0) return 72;
    if (n == 3 && memcmp(s, "say", 3) == 0) return 73;

    return 0;
}

static tok_t lex_next(const char* src, size_t len, uint32_t* io_off)
{
    uint32_t i = *io_off;

    // EOF
    if (i >= (uint32_t)len)
    {
        tok_t t = { TOK_EOF, i, i, 0 };
        return t;
    }

    const uint8_t* u = (const uint8_t*)src;

    // Whitespace / newline
    if (is_space(u[i]))
    {
        const uint32_t start = i;
        while (i < (uint32_t)len && is_space(u[i])) i++;
        *io_off = i;
        tok_t t = { TOK_WS, start, i, 0 };
        return t;
    }

    if (is_nl(u[i]))
    {
        const uint32_t start = i;
        i++;
        *io_off = i;
        tok_t t = { TOK_NL, start, i, 0 };
        return t;
    }

    // Comment (# ... to end of line)
    if (u[i] == '#')
    {
        const uint32_t start = i;
        while (i < (uint32_t)len && !is_nl(u[i])) i++;
        *io_off = i;
        tok_t t = { TOK_COMMENT, start, i, 0 };
        return t;
    }

    // String ("...")
    if (u[i] == '"')
    {
        const uint32_t start = i;
        i++; // opening
        while (i < (uint32_t)len)
        {
            const uint8_t c = u[i];
            if (c == '"') { i++; break; }
            if (c == '\\')
            {
                // skip escape
                i++;
                if (i < (uint32_t)len) i++;
                continue;
            }
            if (is_nl(c)) break; // unterminated

            if (c < 0x80)
            {
                i++;
            }
            else
            {
                const uint32_t k = utf8_one(u + i, (size_t)((uint32_t)len - i));
                if (!k)
                {
                    *io_off = i + 1;
                    tok_t t = { TOK_INVALID_UTF8, start, i + 1, 1 };
                    return t;
                }
                i += k;
            }
        }

        *io_off = i;
        tok_t t = { TOK_STRING, start, i, 0 };
        return t;
    }

    // Number (decimal or hex)
    if (is_digit(u[i]))
    {
        const uint32_t start = i;

        if (u[i] == '0' && i + 1 < (uint32_t)len && (u[i + 1] == 'x' || u[i + 1] == 'X'))
        {
            i += 2;
            while (i < (uint32_t)len && is_hex_digit(u[i])) i++;
            *io_off = i;
            tok_t t = { TOK_HEX, start, i, 0 };
            return t;
        }

        while (i < (uint32_t)len && is_digit(u[i])) i++;
        *io_off = i;
        tok_t t = { TOK_INT, start, i, 0 };
        return t;
    }

    // Identifier / keyword (ASCII start) OR UTF-8 identifier start (rare)
    if (is_ident_start(u[i]) || u[i] >= 0x80)
    {
        const uint32_t start = i;

        // Consume first codepoint (ASCII or utf8)
        if (u[i] >= 0x80)
        {
            const uint32_t k = utf8_one(u + i, (size_t)((uint32_t)len - i));
            if (!k)
            {
                *io_off = i + 1;
                tok_t t = { TOK_INVALID_UTF8, start, i + 1, 2 };
                return t;
            }
            i += k;

            // Continue consuming bytes until whitespace/punct (identifier extension).
            while (i < (uint32_t)len)
            {
                const uint8_t c = u[i];
                if (c < 0x80)
                {
                    if (!is_ident_continue(c)) break;
                    i++;
                }
                else
                {
                    const uint32_t k2 = utf8_one(u + i, (size_t)((uint32_t)len - i));
                    if (!k2) break;
                    i += k2;
                }
            }

            *io_off = i;
            tok_t t = { TOK_IDENT, start, i, 0 };
            return t;
        }

        // ASCII identifier
        i++;
        while (i < (uint32_t)len && is_ident_continue(u[i])) i++;

        const uint32_t kw = keyword_id(src + start, (size_t)(i - start));
        *io_off = i;

        tok_t t;
        if (kw)
            t = (tok_t){ TOK_KEYWORD, start, i, kw };
        else
            t = (tok_t){ TOK_IDENT, start, i, 0 };

        return t;
    }

    // Dot
    if (u[i] == '.')
    {
        const uint32_t start = i;
        i++;
        *io_off = i;
        tok_t t = { TOK_DOT, start, i, 0 };
        return t;
    }

    // Operators and punctuation (single-char classification, with a few pairs)
    {
        const uint32_t start = i;
        const uint8_t c = u[i];
        uint32_t aux = 0;

        // common two-char tokens
        if (i + 1 < (uint32_t)len)
        {
            const uint8_t n = u[i + 1];
            if ((c == '=' && n == '=') || (c == '!' && n == '=') || (c == '<' && n == '=') || (c == '>' && n == '=')
             || (c == '-' && n == '>') || (c == ':' && n == '=') || (c == '=' && n == '>')
             || (c == '&' && n == '&') || (c == '|' && n == '|')
             || (c == ':' && n == ':'))
            {
                i += 2;
                aux = ((uint32_t)c << 8) | (uint32_t)n;
                *io_off = i;
                tok_t t = { TOK_OP, start, i, aux };
                return t;
            }
        }

        i++;
        *io_off = i;

        // classify
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|' || c == '^')
        {
            tok_t t = { TOK_OP, start, i, c };
            return t;
        }

        tok_t t = { TOK_PUNCT, start, i, c };
        return t;
    }
}

typedef struct lex_stats
{
    uint64_t tokens;
    uint64_t keywords;
    uint64_t idents;
    uint64_t ints;
    uint64_t strings;
    uint64_t comments;
    uint64_t invalid_utf8;
    uint64_t hash;
} lex_stats_t;

static lex_stats_t lex_all(const char* src, size_t len)
{
    lex_stats_t st;
    memset(&st, 0, sizeof(st));

    uint32_t off = 0;
    while (1)
    {
        const tok_t t = lex_next(src, len, &off);
        st.tokens++;

        // Update stats + rolling hash.
        st.hash ^= ((uint64_t)t.kind) + ((uint64_t)t.aux << 17) + ((uint64_t)t.start << 3) + ((uint64_t)t.end);
        st.hash = st.hash * 1099511628211ULL + 0x9e3779b97f4a7c15ULL;

        switch (t.kind)
        {
            case TOK_KEYWORD: st.keywords++; break;
            case TOK_IDENT: st.idents++; break;
            case TOK_INT:
            case TOK_HEX: st.ints++; break;
            case TOK_STRING: st.strings++; break;
            case TOK_COMMENT: st.comments++; break;
            case TOK_INVALID_UTF8: st.invalid_utf8++; break;
            default: break;
        }

        if (t.kind == TOK_EOF) break;

        // Avoid infinite loops in case of bugs.
        if (off > (uint32_t)len) break;
    }

    return st;
}

// -----------------------------------------------------------------------------
// Bench cases
// -----------------------------------------------------------------------------

typedef struct lex_case_cfg
{
    uint32_t blob_index;
} lex_case_cfg_t;

static void bm_lex_scan(uint64_t iters, void* user)
{
    init_blobs_once();

    const lex_case_cfg_t* cc = (const lex_case_cfg_t*)user;
    const uint32_t bi = (cc && cc->blob_index < kBlobCount) ? cc->blob_index : 0;

    const char* src = g_blobs[bi].src;
    const size_t len = g_blobs[bi].len;

    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        const lex_stats_t st = lex_all(src, len);
        acc ^= st.hash + st.tokens + (st.keywords << 7) + (st.strings << 3) + (acc << 5) + (acc >> 2);

        // Sample a few bytes from the input to keep src live.
        bench_blackhole_bytes(src, len);
    }

    bench_blackhole_u64(acc);
}

static const lex_case_cfg_t k_small = { 0 };
static const lex_case_cfg_t k_med   = { 1 };
static const lex_case_cfg_t k_large = { 2 };

// -----------------------------------------------------------------------------
// Suite registration
// -----------------------------------------------------------------------------
static const bench_case_t k_cases[] = {
    { "scan_small", bm_lex_scan, (void*)&k_small },
    { "scan_med",   bm_lex_scan, (void*)&k_med   },
    { "scan_large", bm_lex_scan, (void*)&k_large },
};

static const bench_suite_t k_suite = {
    "lex",
    k_cases,
    sizeof(k_cases) / sizeof(k_cases[0]),
};

VITTE_BENCH_CONSTRUCTOR static void lex_suite_autoregister(void)
{
    bench_register_suite(&k_suite);
}

// Optional explicit symbol for runners that call suites manually.
void vitte_bench_register_lex_suite(void)
{
    bench_register_suite(&k_suite);
}
