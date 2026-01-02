

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// parse_suite.c
// -----------------------------------------------------------------------------
// Benchmark suite: parser-like hot paths.
//
// This suite is compiler-independent. It benchmarks typical parsing workloads:
//   - token stream production (lightweight lexer, simplified)
//   - recursive-descent style parsing over a Vitte-like "phrase" grammar
//   - AST node allocation (arena/bump)
//   - error recovery behavior (synchronization points)
//   - pretty-print of AST (walk + string building)
//
// It does NOT aim to be a correct Vitte parser; it is a stable workload to track
// regressions in parsing-style algorithms.
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
#ifndef VITTE_BENCH_PARSE_SUITE_API
#define VITTE_BENCH_PARSE_SUITE_API 1

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
    g_sink_u64 ^= (v + 0x9e3779b97f4a7c15ULL) ^ (g_sink_u64 << 6) ^ (g_sink_u64 >> 3);
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
    g_sink_u8 = (uint8_t)(acc + 0x3D);
}

// -----------------------------------------------------------------------------
// RNG + hash
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
// Synthetic sources
// -----------------------------------------------------------------------------

typedef struct src_blob
{
    char*  src;
    size_t len;
} src_blob_t;

enum { kBlobCount = 3 };
static src_blob_t g_blobs[kBlobCount];
static bool g_inited;

static const char* kIds[] = {
    "alpha","beta","gamma","delta","epsilon","omega",
    "x","y","z","i","j","k","n","m",
    "Parser","Lexer","Token","Span","Ast","Node",
    "Vec","Map","Option","Result",
};

static const char* kTypes[] = {
    "i32","u32","i64","u64","bool","str","f32","f64",
    "Vec[i32]","Vec[u8]","Map[str,i32]","Option[str]",
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

static void gen_stmt(char** out, size_t* len, size_t* cap, uint64_t* rng, uint32_t depth)
{
    // indentation with noise
    const uint32_t ind = depth * 2 + (uint32_t)(xorshift64(rng) & 3u);
    for (uint32_t i = 0; i < ind; ++i)
        blob_append_ch(out, len, cap, (xorshift64(rng) & 7u) == 0u ? '\t' : ' ');

    const uint32_t kind = (uint32_t)(xorshift64(rng) & 15u);

    if (kind == 0)
    {
        blob_append(out, len, cap, "# comment line with noise\n");
        return;
    }

    if (kind == 1)
    {
        // let x: T = expr
        blob_append(out, len, cap, "let ");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        if (xorshift64(rng) & 1u)
        {
            blob_append(out, len, cap, ": ");
            blob_append(out, len, cap, kTypes[xorshift64(rng) % (sizeof(kTypes)/sizeof(kTypes[0]))]);
        }
        blob_append(out, len, cap, (xorshift64(rng) & 1u) ? " = " : "=");
        // expr
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, (xorshift64(rng) & 1u) ? " + " : "+");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, "\n");
        return;
    }

    if (kind == 2)
    {
        // set x = expr
        blob_append(out, len, cap, "set ");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, " = ");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, "\n");
        return;
    }

    if (kind == 3)
    {
        // say "..."
        blob_append(out, len, cap, "say \"");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, "\"\n");
        return;
    }

    if (kind == 4)
    {
        // do call(args)
        blob_append(out, len, cap, "do ");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, "(");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        if (xorshift64(rng) & 1u)
        {
            blob_append(out, len, cap, ", ");
            blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        }
        blob_append(out, len, cap, ")\n");
        return;
    }

    if (kind == 5)
    {
        // ret expr
        blob_append(out, len, cap, "ret ");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, "\n");
        return;
    }

    if (kind == 6)
    {
        // if cond
        blob_append(out, len, cap, "if ");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, (xorshift64(rng) & 1u) ? " == " : "==");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, "\n");
        return;
    }

    if (kind == 7)
    {
        // while cond
        blob_append(out, len, cap, "while ");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, (xorshift64(rng) & 1u) ? " < " : "<");
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%u", (unsigned)u32_range(rng, 0, 10000));
        blob_append(out, len, cap, tmp);
        blob_append(out, len, cap, "\n");
        return;
    }

    if (kind == 8)
    {
        // match x
        blob_append(out, len, cap, "match ");
        blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
        blob_append(out, len, cap, "\n");
        return;
    }

    if (kind == 9)
    {
        // pattern arm
        blob_append(out, len, cap, (xorshift64(rng) & 1u) ? "_" : "1");
        blob_append(out, len, cap, " => ");
        blob_append(out, len, cap, "ret ");
        blob_append(out, len, cap, (xorshift64(rng) & 1u) ? "0" : "1");
        blob_append(out, len, cap, "\n");
        return;
    }

    if (kind == 10)
    {
        // .end
        blob_append(out, len, cap, ".end\n");
        return;
    }

    // generic expr stmt
    blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
    blob_append(out, len, cap, "(" );
    blob_append(out, len, cap, kIds[xorshift64(rng) % (sizeof(kIds)/sizeof(kIds[0]))]);
    blob_append(out, len, cap, ")\n");
}

static void gen_blob(src_blob_t* b, uint64_t seed, size_t target_bytes)
{
    char* out = NULL;
    size_t len = 0;
    size_t cap = 0;

    uint64_t rng = seed;
    uint32_t depth = 0;

    // start with a program block
    blob_append(&out, &len, &cap, "program my/app\n");
    depth = 1;

    while (len < target_bytes)
    {
        const uint32_t r = (uint32_t)(xorshift64(&rng) & 31u);
        // occasionally open/close logical blocks
        if (r == 0 && depth < 32) depth++;
        if (r == 1 && depth > 1) depth--;

        // sometimes insert malformed lines to force recovery
        if ((xorshift64(&rng) & 127u) == 0u)
        {
            blob_append(&out, &len, &cap, "if if if ==\n");
            continue;
        }
        if ((xorshift64(&rng) & 255u) == 0u)
        {
            blob_append(&out, &len, &cap, "let = = =\n");
            continue;
        }

        gen_stmt(&out, &len, &cap, &rng, depth);

        if ((xorshift64(&rng) & 63u) == 0u)
            blob_append(&out, &len, &cap, "\n");
    }

    // close remaining blocks
    while (depth--)
        blob_append(&out, &len, &cap, ".end\n");

    b->src = out;
    b->len = len;
}

static void init_blobs_once(void)
{
    if (g_inited) return;
    g_inited = true;

    gen_blob(&g_blobs[0], 0xAAAAAAAABBBBBBBBULL, 32 * 1024);
    gen_blob(&g_blobs[1], 0xCCCCCCCCDDDDDDDDULL, 256 * 1024);
    gen_blob(&g_blobs[2], 0xEEEEEEEEFFFFFFFFULL, 2 * 1024 * 1024);
}

// -----------------------------------------------------------------------------
// Lightweight lexer to produce tokens for parsing
// -----------------------------------------------------------------------------

typedef enum tk
{
    TK_EOF = 0,

    TK_NL,
    TK_WS,
    TK_COMMENT,

    TK_IDENT,
    TK_INT,
    TK_STRING,

    TK_DOT,
    TK_LPAREN,
    TK_RPAREN,
    TK_COMMA,
    TK_COLON,

    TK_EQ,
    TK_EQEQ,
    TK_PLUS,
    TK_LT,
    TK_ARROW,   // =>

    TK_KW_PROGRAM,
    TK_KW_FN,
    TK_KW_LET,
    TK_KW_CONST,
    TK_KW_SET,
    TK_KW_SAY,
    TK_KW_DO,
    TK_KW_RET,
    TK_KW_IF,
    TK_KW_ELIF,
    TK_KW_ELSE,
    TK_KW_WHILE,
    TK_KW_FOR,
    TK_KW_MATCH,
    TK_KW_WHEN,
    TK_KW_LOOP,

    TK_BAD,
} tk_t;

typedef struct token
{
    tk_t kind;
    uint32_t start;
    uint32_t end;
    uint32_t aux; // keyword id / error flags
} token_t;

static inline bool is_space(unsigned char c) { return c == ' ' || c == '\t' || c == '\r'; }
static inline bool is_nl(unsigned char c)    { return c == '\n'; }
static inline bool is_digit(unsigned char c) { return (c >= '0' && c <= '9'); }
static inline bool is_ident_start(unsigned char c) { return (c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
static inline bool is_ident_continue(unsigned char c) { return is_ident_start(c) || is_digit(c); }

static inline bool word_eq(const char* s, size_t n, const char* lit)
{
    const size_t m = strlen(lit);
    return (n == m) && memcmp(s, lit, m) == 0;
}

static inline tk_t keyword_kind(const char* s, size_t n)
{
    // Straight compares; small set.
    if (word_eq(s, n, "program")) return TK_KW_PROGRAM;
    if (word_eq(s, n, "fn"))      return TK_KW_FN;
    if (word_eq(s, n, "let"))     return TK_KW_LET;
    if (word_eq(s, n, "const"))   return TK_KW_CONST;
    if (word_eq(s, n, "set"))     return TK_KW_SET;
    if (word_eq(s, n, "say"))     return TK_KW_SAY;
    if (word_eq(s, n, "do"))      return TK_KW_DO;
    if (word_eq(s, n, "ret"))     return TK_KW_RET;
    if (word_eq(s, n, "if"))      return TK_KW_IF;
    if (word_eq(s, n, "elif"))    return TK_KW_ELIF;
    if (word_eq(s, n, "else"))    return TK_KW_ELSE;
    if (word_eq(s, n, "while"))   return TK_KW_WHILE;
    if (word_eq(s, n, "for"))     return TK_KW_FOR;
    if (word_eq(s, n, "match"))   return TK_KW_MATCH;
    if (word_eq(s, n, "when"))    return TK_KW_WHEN;
    if (word_eq(s, n, "loop"))    return TK_KW_LOOP;
    return TK_IDENT;
}

static token_t lex_next(const char* src, size_t len, uint32_t* io_off)
{
    uint32_t i = *io_off;
    if (i >= (uint32_t)len) return (token_t){ TK_EOF, i, i, 0 };

    const unsigned char c = (unsigned char)src[i];

    if (is_space(c))
    {
        const uint32_t s = i;
        while (i < (uint32_t)len && is_space((unsigned char)src[i])) i++;
        *io_off = i;
        return (token_t){ TK_WS, s, i, 0 };
    }

    if (is_nl(c))
    {
        const uint32_t s = i;
        i++;
        *io_off = i;
        return (token_t){ TK_NL, s, i, 0 };
    }

    if (c == '#')
    {
        const uint32_t s = i;
        while (i < (uint32_t)len && !is_nl((unsigned char)src[i])) i++;
        *io_off = i;
        return (token_t){ TK_COMMENT, s, i, 0 };
    }

    if (c == '"')
    {
        const uint32_t s = i;
        i++;
        while (i < (uint32_t)len)
        {
            const unsigned char d = (unsigned char)src[i];
            if (d == '"') { i++; break; }
            if (d == '\\') { i++; if (i < (uint32_t)len) i++; continue; }
            if (is_nl(d)) break;
            i++;
        }
        *io_off = i;
        return (token_t){ TK_STRING, s, i, 0 };
    }

    if (is_digit(c))
    {
        const uint32_t s = i;
        while (i < (uint32_t)len && is_digit((unsigned char)src[i])) i++;
        *io_off = i;
        return (token_t){ TK_INT, s, i, 0 };
    }

    if (is_ident_start(c))
    {
        const uint32_t s = i;
        i++;
        while (i < (uint32_t)len && is_ident_continue((unsigned char)src[i])) i++;
        const tk_t k = keyword_kind(src + s, (size_t)(i - s));
        *io_off = i;
        return (token_t){ k, s, i, 0 };
    }

    // 2-char operators
    if (i + 1 < (uint32_t)len)
    {
        const unsigned char n = (unsigned char)src[i + 1];
        if (c == '=' && n == '=') { *io_off = i + 2; return (token_t){ TK_EQEQ, i, i + 2, 0 }; }
        if (c == '=' && n == '>') { *io_off = i + 2; return (token_t){ TK_ARROW, i, i + 2, 0 }; }
    }

    // single-char
    *io_off = i + 1;
    switch (c)
    {
        case '.': return (token_t){ TK_DOT, i, i + 1, 0 };
        case '(': return (token_t){ TK_LPAREN, i, i + 1, 0 };
        case ')': return (token_t){ TK_RPAREN, i, i + 1, 0 };
        case ',': return (token_t){ TK_COMMA, i, i + 1, 0 };
        case ':': return (token_t){ TK_COLON, i, i + 1, 0 };
        case '=': return (token_t){ TK_EQ, i, i + 1, 0 };
        case '+': return (token_t){ TK_PLUS, i, i + 1, 0 };
        case '<': return (token_t){ TK_LT, i, i + 1, 0 };
        default:  return (token_t){ TK_BAD, i, i + 1, (uint32_t)c };
    }
}

// -----------------------------------------------------------------------------
// Token buffer
// -----------------------------------------------------------------------------

typedef struct tokbuf
{
    token_t* t;
    uint32_t cap;
    uint32_t len;
} tokbuf_t;

static void tokbuf_init(tokbuf_t* b, uint32_t cap)
{
    b->t = (token_t*)malloc(sizeof(token_t) * (size_t)cap);
    if (!b->t) abort();
    b->cap = cap;
    b->len = 0;
}

static void tokbuf_reset(tokbuf_t* b)
{
    b->len = 0;
}

static void tokbuf_free(tokbuf_t* b)
{
    free(b->t);
    b->t = NULL;
    b->cap = 0;
    b->len = 0;
}

static void tokbuf_push(tokbuf_t* b, token_t t)
{
    if (b->len >= b->cap) return; // truncate for bench stability
    b->t[b->len++] = t;
}

static void tokenize_all(tokbuf_t* b, const char* src, size_t len)
{
    tokbuf_reset(b);

    uint32_t off = 0;
    while (1)
    {
        token_t t = lex_next(src, len, &off);
        tokbuf_push(b, t);
        if (t.kind == TK_EOF) break;
        if (off > (uint32_t)len) break;
    }
}

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

typedef struct arena
{
    uint8_t* p;
    size_t cap;
    size_t off;
} arena_t;

static void arena_init(arena_t* a, size_t cap)
{
    a->p = (uint8_t*)malloc(cap);
    if (!a->p) abort();
    a->cap = cap;
    a->off = 0;
}

static void arena_reset(arena_t* a)
{
    a->off = 0;
}

static void arena_free(arena_t* a)
{
    free(a->p);
    a->p = NULL;
    a->cap = 0;
    a->off = 0;
}

static void* arena_alloc(arena_t* a, size_t n, size_t align)
{
    const size_t mask = align - 1;
    size_t at = (a->off + mask) & ~mask;
    if (at + n > a->cap) return NULL;
    void* p = a->p + at;
    a->off = at + n;
    return p;
}

// -----------------------------------------------------------------------------
// AST
// -----------------------------------------------------------------------------

typedef enum ast_kind
{
    AST_MODULE = 1,
    AST_PROGRAM,
    AST_FN,

    AST_BLOCK,
    AST_STMT_LET,
    AST_STMT_SET,
    AST_STMT_SAY,
    AST_STMT_DO,
    AST_STMT_RET,
    AST_STMT_IF,
    AST_STMT_WHILE,
    AST_STMT_MATCH,
    AST_MATCH_ARM,

    AST_EXPR_IDENT,
    AST_EXPR_INT,
    AST_EXPR_STR,
    AST_EXPR_BIN,
    AST_EXPR_CALL,
} ast_kind_t;

typedef struct ast_node ast_node_t;

typedef struct ast_list
{
    ast_node_t** items;
    uint32_t len;
    uint32_t cap;
} ast_list_t;

struct ast_node
{
    ast_kind_t kind;
    uint32_t start_tok;
    uint32_t end_tok;

    union
    {
        struct { ast_list_t items; } module;
        struct { const char* name; ast_node_t* body; } program;
        struct { const char* name; ast_node_t* body; } fn;

        struct { ast_list_t stmts; } block;

        struct { const char* name; const char* type_name; ast_node_t* init; } let_stmt;
        struct { const char* name; ast_node_t* value; } set_stmt;
        struct { ast_node_t* value; } say_stmt;
        struct { ast_node_t* call; } do_stmt;
        struct { ast_node_t* value; } ret_stmt;

        struct { ast_node_t* cond; ast_node_t* then_blk; ast_node_t* else_blk; } if_stmt;
        struct { ast_node_t* cond; ast_node_t* body; } while_stmt;
        struct { ast_node_t* scrut; ast_list_t arms; } match_stmt;
        struct { ast_node_t* pat; ast_node_t* body; } match_arm;

        struct { const char* name; } ident;
        struct { uint32_t value; } int_lit;
        struct { const char* s; } str_lit;
        struct { uint32_t op; ast_node_t* a; ast_node_t* b; } bin;
        struct { ast_node_t* callee; ast_list_t args; } call;
    } as;
};

static ast_list_t ast_list_make(arena_t* a, uint32_t cap)
{
    ast_list_t l;
    l.items = (ast_node_t**)arena_alloc(a, sizeof(ast_node_t*) * (size_t)cap, _Alignof(ast_node_t*));
    l.len = 0;
    l.cap = l.items ? cap : 0;
    return l;
}

static void ast_list_push(ast_list_t* l, ast_node_t* n)
{
    if (l->len < l->cap)
        l->items[l->len++] = n;
}

static ast_node_t* ast_new(arena_t* a, ast_kind_t k, uint32_t s, uint32_t e)
{
    ast_node_t* n = (ast_node_t*)arena_alloc(a, sizeof(ast_node_t), _Alignof(ast_node_t));
    if (!n) return NULL;
    memset(n, 0, sizeof(*n));
    n->kind = k;
    n->start_tok = s;
    n->end_tok = e;
    return n;
}

// -----------------------------------------------------------------------------
// Parser
// -----------------------------------------------------------------------------

typedef struct parser
{
    const char* src;
    size_t src_len;

    const token_t* t;
    uint32_t t_len;

    uint32_t i;      // cursor
    uint32_t errors; // count

    arena_t* arena;
} parser_t;

static inline token_t cur(parser_t* p)
{
    if (p->i < p->t_len) return p->t[p->i];
    return (token_t){ TK_EOF, (uint32_t)p->src_len, (uint32_t)p->src_len, 0 };
}

static inline bool is_kind(parser_t* p, tk_t k)
{
    return cur(p).kind == k;
}

static inline bool eat(parser_t* p, tk_t k)
{
    if (is_kind(p, k)) { p->i++; return true; }
    return false;
}

static inline void skip_ws(parser_t* p)
{
    while (1)
    {
        const tk_t k = cur(p).kind;
        if (k == TK_WS || k == TK_COMMENT) { p->i++; continue; }
        break;
    }
}

static inline bool eat_nl(parser_t* p)
{
    bool any = false;
    while (eat(p, TK_NL)) any = true;
    return any;
}

static inline void sync_to_line(parser_t* p)
{
    // error recovery: skip tokens until NL or .end
    while (1)
    {
        tk_t k = cur(p).kind;
        if (k == TK_EOF) break;
        if (k == TK_NL) { p->i++; break; }
        if (k == TK_DOT)
        {
            // possible .end
            uint32_t save = p->i;
            p->i++;
            skip_ws(p);
            if (is_kind(p, TK_IDENT))
            {
                token_t id = cur(p);
                const char* s = p->src + id.start;
                const size_t n = (size_t)(id.end - id.start);
                if (word_eq(s, n, "end"))
                {
                    p->i++; // consume "end"
                    break;
                }
            }
            p->i = save;
        }
        p->i++;
    }
}

// Forward decls
static ast_node_t* parse_expr(parser_t* p);
static ast_node_t* parse_stmt(parser_t* p);
static ast_node_t* parse_block_until_end(parser_t* p);

static ast_node_t* parse_primary(parser_t* p)
{
    skip_ws(p);
    token_t t = cur(p);

    if (t.kind == TK_IDENT)
    {
        p->i++;
        ast_node_t* n = ast_new(p->arena, AST_EXPR_IDENT, p->i - 1, p->i);
        if (!n) return NULL;
        n->as.ident.name = p->src + t.start;
        return n;
    }

    if (t.kind == TK_INT)
    {
        p->i++;
        ast_node_t* n = ast_new(p->arena, AST_EXPR_INT, p->i - 1, p->i);
        if (!n) return NULL;
        // parse value (bounded)
        uint32_t v = 0;
        for (uint32_t i = t.start; i < t.end; ++i)
        {
            unsigned char c = (unsigned char)p->src[i];
            if (!is_digit(c)) break;
            v = v * 10u + (uint32_t)(c - '0');
        }
        n->as.int_lit.value = v;
        return n;
    }

    if (t.kind == TK_STRING)
    {
        p->i++;
        ast_node_t* n = ast_new(p->arena, AST_EXPR_STR, p->i - 1, p->i);
        if (!n) return NULL;
        n->as.str_lit.s = p->src + t.start;
        return n;
    }

    if (eat(p, TK_LPAREN))
    {
        ast_node_t* e = parse_expr(p);
        skip_ws(p);
        eat(p, TK_RPAREN);
        return e;
    }

    // error node: treat unknown as ident to keep going
    p->errors++;
    p->i++;
    ast_node_t* n = ast_new(p->arena, AST_EXPR_IDENT, p->i - 1, p->i);
    if (n) n->as.ident.name = "<bad>";
    return n;
}

static ast_node_t* parse_postfix(parser_t* p)
{
    ast_node_t* e = parse_primary(p);
    if (!e) return NULL;

    while (1)
    {
        skip_ws(p);
        if (!is_kind(p, TK_LPAREN)) break;

        // call
        uint32_t call_start = p->i;
        eat(p, TK_LPAREN);

        ast_node_t* call = ast_new(p->arena, AST_EXPR_CALL, call_start, call_start);
        if (!call) return e;
        call->as.call.callee = e;
        call->as.call.args = ast_list_make(p->arena, 8);

        skip_ws(p);
        if (!eat(p, TK_RPAREN))
        {
            while (1)
            {
                ast_node_t* a = parse_expr(p);
                if (a) ast_list_push(&call->as.call.args, a);
                skip_ws(p);
                if (eat(p, TK_COMMA)) { skip_ws(p); continue; }
                eat(p, TK_RPAREN);
                break;
            }
        }

        call->end_tok = p->i;
        e = call;
    }

    return e;
}

static ast_node_t* parse_bin_rhs(parser_t* p, ast_node_t* lhs, int min_prec)
{
    while (1)
    {
        skip_ws(p);
        tk_t op = cur(p).kind;
        int prec = 0;

        if (op == TK_PLUS) prec = 10;
        else if (op == TK_EQEQ || op == TK_LT) prec = 5;
        else break;

        if (prec < min_prec) break;

        p->i++; // consume op
        ast_node_t* rhs = parse_postfix(p);
        if (!rhs) return lhs;

        // next op
        skip_ws(p);
        tk_t next = cur(p).kind;
        int next_prec = 0;
        if (next == TK_PLUS) next_prec = 10;
        else if (next == TK_EQEQ || next == TK_LT) next_prec = 5;

        if (next_prec > prec)
            rhs = parse_bin_rhs(p, rhs, prec + 1);

        ast_node_t* bin = ast_new(p->arena, AST_EXPR_BIN, lhs->start_tok, rhs->end_tok);
        if (!bin) return lhs;
        bin->as.bin.op = (uint32_t)op;
        bin->as.bin.a = lhs;
        bin->as.bin.b = rhs;
        lhs = bin;
    }

    return lhs;
}

static ast_node_t* parse_expr(parser_t* p)
{
    ast_node_t* lhs = parse_postfix(p);
    if (!lhs) return NULL;
    return parse_bin_rhs(p, lhs, 0);
}

static ast_node_t* parse_stmt_let(parser_t* p, uint32_t start)
{
    skip_ws(p);
    token_t id = cur(p);
    if (id.kind != TK_IDENT)
    {
        p->errors++;
        sync_to_line(p);
        return NULL;
    }
    p->i++;

    const char* name = p->src + id.start;

    // optional : Type
    const char* type_name = NULL;
    skip_ws(p);
    if (eat(p, TK_COLON))
    {
        skip_ws(p);
        token_t ty = cur(p);
        if (ty.kind == TK_IDENT)
        {
            type_name = p->src + ty.start;
            p->i++;
        }
    }

    skip_ws(p);
    if (!eat(p, TK_EQ))
    {
        // tolerate missing '='
        p->errors++;
    }

    ast_node_t* init = parse_expr(p);

    ast_node_t* n = ast_new(p->arena, AST_STMT_LET, start, p->i);
    if (!n) return NULL;
    n->as.let_stmt.name = name;
    n->as.let_stmt.type_name = type_name;
    n->as.let_stmt.init = init;

    return n;
}

static ast_node_t* parse_stmt_set(parser_t* p, uint32_t start)
{
    skip_ws(p);
    token_t id = cur(p);
    if (id.kind != TK_IDENT)
    {
        p->errors++;
        sync_to_line(p);
        return NULL;
    }
    p->i++;
    const char* name = p->src + id.start;

    skip_ws(p);
    eat(p, TK_EQ);

    ast_node_t* v = parse_expr(p);

    ast_node_t* n = ast_new(p->arena, AST_STMT_SET, start, p->i);
    if (!n) return NULL;
    n->as.set_stmt.name = name;
    n->as.set_stmt.value = v;
    return n;
}

static ast_node_t* parse_stmt_say(parser_t* p, uint32_t start)
{
    ast_node_t* v = parse_expr(p);
    ast_node_t* n = ast_new(p->arena, AST_STMT_SAY, start, p->i);
    if (n) n->as.say_stmt.value = v;
    return n;
}

static ast_node_t* parse_stmt_do(parser_t* p, uint32_t start)
{
    ast_node_t* c = parse_expr(p);
    ast_node_t* n = ast_new(p->arena, AST_STMT_DO, start, p->i);
    if (n) n->as.do_stmt.call = c;
    return n;
}

static ast_node_t* parse_stmt_ret(parser_t* p, uint32_t start)
{
    ast_node_t* v = parse_expr(p);
    ast_node_t* n = ast_new(p->arena, AST_STMT_RET, start, p->i);
    if (n) n->as.ret_stmt.value = v;
    return n;
}

static ast_node_t* parse_stmt_if(parser_t* p, uint32_t start)
{
    ast_node_t* cond = parse_expr(p);
    eat_nl(p);

    ast_node_t* then_blk = parse_block_until_end(p);

    // optional else
    ast_node_t* else_blk = NULL;
    skip_ws(p);
    if (eat(p, TK_KW_ELSE))
    {
        eat_nl(p);
        else_blk = parse_block_until_end(p);
    }

    ast_node_t* n = ast_new(p->arena, AST_STMT_IF, start, p->i);
    if (!n) return NULL;
    n->as.if_stmt.cond = cond;
    n->as.if_stmt.then_blk = then_blk;
    n->as.if_stmt.else_blk = else_blk;
    return n;
}

static ast_node_t* parse_stmt_while(parser_t* p, uint32_t start)
{
    ast_node_t* cond = parse_expr(p);
    eat_nl(p);
    ast_node_t* body = parse_block_until_end(p);

    ast_node_t* n = ast_new(p->arena, AST_STMT_WHILE, start, p->i);
    if (!n) return NULL;
    n->as.while_stmt.cond = cond;
    n->as.while_stmt.body = body;
    return n;
}

static ast_node_t* parse_stmt_match(parser_t* p, uint32_t start)
{
    ast_node_t* scrut = parse_expr(p);
    eat_nl(p);

    ast_node_t* blk = ast_new(p->arena, AST_STMT_MATCH, start, start);
    if (!blk) return NULL;
    blk->as.match_stmt.scrut = scrut;
    blk->as.match_stmt.arms = ast_list_make(p->arena, 64);

    while (1)
    {
        skip_ws(p);
        if (is_kind(p, TK_EOF)) break;

        // try to detect .end
        if (is_kind(p, TK_DOT))
        {
            uint32_t save = p->i;
            p->i++;
            skip_ws(p);
            if (is_kind(p, TK_IDENT))
            {
                token_t id = cur(p);
                const char* s = p->src + id.start;
                const size_t n = (size_t)(id.end - id.start);
                if (word_eq(s, n, "end"))
                {
                    p->i++;
                    break;
                }
            }
            p->i = save;
        }

        // pattern: '_' | int | ident
        ast_node_t* pat = parse_primary(p);
        skip_ws(p);

        // expect =>
        if (!eat(p, TK_ARROW))
        {
            p->errors++;
            sync_to_line(p);
            continue;
        }

        ast_node_t* body = parse_stmt(p);
        if (!body)
        {
            sync_to_line(p);
            continue;
        }

        ast_node_t* arm = ast_new(p->arena, AST_MATCH_ARM, pat ? pat->start_tok : start, p->i);
        if (arm)
        {
            arm->as.match_arm.pat = pat;
            arm->as.match_arm.body = body;
            ast_list_push(&blk->as.match_stmt.arms, arm);
        }

        eat_nl(p);
    }

    blk->end_tok = p->i;
    return blk;
}

static ast_node_t* parse_stmt(parser_t* p)
{
    skip_ws(p);
    uint32_t start = p->i;
    tk_t k = cur(p).kind;

    ast_node_t* n = NULL;

    if (eat(p, TK_KW_LET)) n = parse_stmt_let(p, start);
    else if (eat(p, TK_KW_SET)) n = parse_stmt_set(p, start);
    else if (eat(p, TK_KW_SAY)) n = parse_stmt_say(p, start);
    else if (eat(p, TK_KW_DO)) n = parse_stmt_do(p, start);
    else if (eat(p, TK_KW_RET)) n = parse_stmt_ret(p, start);
    else if (eat(p, TK_KW_IF)) n = parse_stmt_if(p, start);
    else if (eat(p, TK_KW_WHILE)) n = parse_stmt_while(p, start);
    else if (eat(p, TK_KW_MATCH)) n = parse_stmt_match(p, start);
    else
    {
        // expr stmt
        n = parse_expr(p);
        // wrap into do_stmt-ish node to keep AST uniform
        if (n)
        {
            ast_node_t* s = ast_new(p->arena, AST_STMT_DO, start, p->i);
            if (s) { s->as.do_stmt.call = n; n = s; }
        }
    }

    // consume to line end
    while (cur(p).kind == TK_WS || cur(p).kind == TK_COMMENT) p->i++;
    eat_nl(p);

    return n;
}

static ast_node_t* parse_block_until_end(parser_t* p)
{
    ast_node_t* b = ast_new(p->arena, AST_BLOCK, p->i, p->i);
    if (!b) return NULL;
    b->as.block.stmts = ast_list_make(p->arena, 256);

    while (1)
    {
        skip_ws(p);
        if (is_kind(p, TK_EOF)) break;

        // detect .end
        if (is_kind(p, TK_DOT))
        {
            uint32_t save = p->i;
            p->i++;
            skip_ws(p);
            if (is_kind(p, TK_IDENT))
            {
                token_t id = cur(p);
                const char* s = p->src + id.start;
                const size_t n = (size_t)(id.end - id.start);
                if (word_eq(s, n, "end"))
                {
                    p->i++;
                    break;
                }
            }
            p->i = save;
        }

        ast_node_t* st = parse_stmt(p);
        if (st)
            ast_list_push(&b->as.block.stmts, st);
        else
            sync_to_line(p);
    }

    b->end_tok = p->i;
    return b;
}

static ast_node_t* parse_program(parser_t* p)
{
    skip_ws(p);
    uint32_t start = p->i;
    if (!eat(p, TK_KW_PROGRAM))
        return NULL;

    skip_ws(p);
    token_t name = cur(p);
    const char* nstr = NULL;
    if (name.kind == TK_IDENT)
    {
        nstr = p->src + name.start;
        p->i++;
    }

    eat_nl(p);

    ast_node_t* body = parse_block_until_end(p);

    ast_node_t* n = ast_new(p->arena, AST_PROGRAM, start, p->i);
    if (!n) return NULL;
    n->as.program.name = nstr;
    n->as.program.body = body;
    return n;
}

static ast_node_t* parse_module(parser_t* p)
{
    ast_node_t* m = ast_new(p->arena, AST_MODULE, 0, 0);
    if (!m) return NULL;
    m->as.module.items = ast_list_make(p->arena, 64);

    while (1)
    {
        skip_ws(p);
        if (is_kind(p, TK_EOF)) break;

        if (is_kind(p, TK_KW_PROGRAM))
        {
            ast_node_t* pr = parse_program(p);
            if (pr) ast_list_push(&m->as.module.items, pr);
            continue;
        }

        // skip unknown top-level lines
        p->errors++;
        sync_to_line(p);
    }

    m->end_tok = p->i;
    return m;
}

// -----------------------------------------------------------------------------
// AST pretty printer
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

static inline void sb_puts(strbuf_t* sb, const char* s)
{
    if (!s) return;
    size_t n = strlen(s);
    size_t rem = (sb->cap > sb->len) ? (sb->cap - sb->len) : 0;
    if (rem <= 1) return;
    size_t w = (n < rem - 1) ? n : (rem - 1);
    memcpy(sb->p + sb->len, s, w);
    sb->len += w;
    sb->p[sb->len] = '\0';
}

static inline void sb_indent(strbuf_t* sb, uint32_t d)
{
    for (uint32_t i = 0; i < d; ++i) sb_puts(sb, "  ");
}

static void pp_node(strbuf_t* sb, const ast_node_t* n, uint32_t depth)
{
    if (!n) { sb_indent(sb, depth); sb_puts(sb, "<null>\n"); return; }

    sb_indent(sb, depth);

    switch (n->kind)
    {
        case AST_MODULE:
            sb_puts(sb, "Module\n");
            for (uint32_t i = 0; i < n->as.module.items.len; ++i)
                pp_node(sb, n->as.module.items.items[i], depth + 1);
            break;

        case AST_PROGRAM:
            sb_puts(sb, "Program ");
            sb_puts(sb, n->as.program.name ? n->as.program.name : "<noname>");
            sb_putc(sb, '\n');
            pp_node(sb, n->as.program.body, depth + 1);
            break;

        case AST_BLOCK:
            sb_puts(sb, "Block\n");
            for (uint32_t i = 0; i < n->as.block.stmts.len; ++i)
                pp_node(sb, n->as.block.stmts.items[i], depth + 1);
            break;

        case AST_STMT_LET:
            sb_puts(sb, "Let ");
            sb_puts(sb, n->as.let_stmt.name ? n->as.let_stmt.name : "<noname>");
            if (n->as.let_stmt.type_name)
            {
                sb_puts(sb, ": ");
                sb_puts(sb, n->as.let_stmt.type_name);
            }
            sb_putc(sb, '\n');
            pp_node(sb, n->as.let_stmt.init, depth + 1);
            break;

        case AST_STMT_SET:
            sb_puts(sb, "Set ");
            sb_puts(sb, n->as.set_stmt.name ? n->as.set_stmt.name : "<noname>");
            sb_putc(sb, '\n');
            pp_node(sb, n->as.set_stmt.value, depth + 1);
            break;

        case AST_STMT_SAY:
            sb_puts(sb, "Say\n");
            pp_node(sb, n->as.say_stmt.value, depth + 1);
            break;

        case AST_STMT_DO:
            sb_puts(sb, "Do\n");
            pp_node(sb, n->as.do_stmt.call, depth + 1);
            break;

        case AST_STMT_RET:
            sb_puts(sb, "Ret\n");
            pp_node(sb, n->as.ret_stmt.value, depth + 1);
            break;

        case AST_STMT_IF:
            sb_puts(sb, "If\n");
            sb_indent(sb, depth + 1); sb_puts(sb, "Cond\n");
            pp_node(sb, n->as.if_stmt.cond, depth + 2);
            sb_indent(sb, depth + 1); sb_puts(sb, "Then\n");
            pp_node(sb, n->as.if_stmt.then_blk, depth + 2);
            if (n->as.if_stmt.else_blk)
            {
                sb_indent(sb, depth + 1); sb_puts(sb, "Else\n");
                pp_node(sb, n->as.if_stmt.else_blk, depth + 2);
            }
            break;

        case AST_STMT_WHILE:
            sb_puts(sb, "While\n");
            sb_indent(sb, depth + 1); sb_puts(sb, "Cond\n");
            pp_node(sb, n->as.while_stmt.cond, depth + 2);
            sb_indent(sb, depth + 1); sb_puts(sb, "Body\n");
            pp_node(sb, n->as.while_stmt.body, depth + 2);
            break;

        case AST_STMT_MATCH:
            sb_puts(sb, "Match\n");
            sb_indent(sb, depth + 1); sb_puts(sb, "Scrut\n");
            pp_node(sb, n->as.match_stmt.scrut, depth + 2);
            for (uint32_t i = 0; i < n->as.match_stmt.arms.len; ++i)
                pp_node(sb, n->as.match_stmt.arms.items[i], depth + 1);
            break;

        case AST_MATCH_ARM:
            sb_puts(sb, "Arm\n");
            sb_indent(sb, depth + 1); sb_puts(sb, "Pat\n");
            pp_node(sb, n->as.match_arm.pat, depth + 2);
            sb_indent(sb, depth + 1); sb_puts(sb, "Body\n");
            pp_node(sb, n->as.match_arm.body, depth + 2);
            break;

        case AST_EXPR_IDENT:
            sb_puts(sb, "Ident ");
            sb_puts(sb, n->as.ident.name ? n->as.ident.name : "<id>");
            sb_putc(sb, '\n');
            break;

        case AST_EXPR_INT:
        {
            sb_puts(sb, "Int ");
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%u", (unsigned)n->as.int_lit.value);
            sb_puts(sb, tmp);
            sb_putc(sb, '\n');
        } break;

        case AST_EXPR_STR:
            sb_puts(sb, "Str\n");
            break;

        case AST_EXPR_BIN:
            sb_puts(sb, "Bin\n");
            pp_node(sb, n->as.bin.a, depth + 1);
            pp_node(sb, n->as.bin.b, depth + 1);
            break;

        case AST_EXPR_CALL:
            sb_puts(sb, "Call\n");
            pp_node(sb, n->as.call.callee, depth + 1);
            for (uint32_t i = 0; i < n->as.call.args.len; ++i)
                pp_node(sb, n->as.call.args.items[i], depth + 1);
            break;

        default:
            sb_puts(sb, "<node>\n");
            break;
    }
}

static size_t pp_ast(const ast_node_t* root, char* out, size_t cap)
{
    strbuf_t sb;
    sb_init(&sb, out, cap);
    pp_node(&sb, root, 0);
    return sb.len;
}

// -----------------------------------------------------------------------------
// Bench driver
// -----------------------------------------------------------------------------

typedef struct parse_case_cfg
{
    uint32_t blob_index;
    uint32_t tok_cap;
    size_t arena_cap;
} parse_case_cfg_t;

static tokbuf_t g_tok;
static arena_t  g_arena;
static bool g_fx_inited;

static void ensure_fx(uint32_t tok_cap, size_t arena_cap)
{
    if (g_fx_inited) return;
    tokbuf_init(&g_tok, tok_cap);
    arena_init(&g_arena, arena_cap);
    g_fx_inited = true;
}

static void parse_one(const char* src, size_t len, uint32_t tok_cap, size_t arena_cap, uint64_t* io_hash)
{
    ensure_fx(tok_cap, arena_cap);
    arena_reset(&g_arena);

    tokenize_all(&g_tok, src, len);

    parser_t p;
    memset(&p, 0, sizeof(p));
    p.src = src;
    p.src_len = len;
    p.t = g_tok.t;
    p.t_len = g_tok.len;
    p.i = 0;
    p.errors = 0;
    p.arena = &g_arena;

    ast_node_t* root = parse_module(&p);

    // pretty-print into a fixed buffer
    const size_t out_cap = 256 * 1024;
    char* out = (char*)malloc(out_cap);
    if (!out) abort();
    const size_t out_len = pp_ast(root, out, out_cap);

    // mix hashes: AST print + error count + token count
    uint64_t h = fnv1a64(out, out_len);
    h ^= (uint64_t)p.errors * 1315423911ULL;
    h ^= (uint64_t)g_tok.len * 2654435761ULL;

    bench_blackhole_bytes(out, out_len);
    free(out);

    *io_hash ^= (h + (*io_hash << 7) + (*io_hash >> 3));
}

static void bm_parse(uint64_t iters, void* user)
{
    init_blobs_once();

    const parse_case_cfg_t* cc = (const parse_case_cfg_t*)user;
    const uint32_t bi = (cc && cc->blob_index < kBlobCount) ? cc->blob_index : 0;
    const uint32_t tok_cap = (cc) ? cc->tok_cap : (1u << 20);
    const size_t arena_cap = (cc) ? cc->arena_cap : (8u << 20);

    const char* src = g_blobs[bi].src;
    const size_t len = g_blobs[bi].len;

    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        parse_one(src, len, tok_cap, arena_cap, &acc);
    }

    bench_blackhole_u64(acc);
}

// -----------------------------------------------------------------------------
// Suite registration
// -----------------------------------------------------------------------------

static const parse_case_cfg_t k_small = { 0, 1u << 18, 4u << 20  }; // tok 262k, arena 4MB
static const parse_case_cfg_t k_med   = { 1, 1u << 19, 8u << 20  }; // tok 524k, arena 8MB
static const parse_case_cfg_t k_large = { 2, 1u << 20, 16u << 20 }; // tok 1M, arena 16MB

static const bench_case_t k_cases[] = {
    { "parse_small", bm_parse, (void*)&k_small },
    { "parse_med",   bm_parse, (void*)&k_med   },
    { "parse_large", bm_parse, (void*)&k_large },
};

static const bench_suite_t k_suite = {
    "parse",
    k_cases,
    sizeof(k_cases) / sizeof(k_cases[0]),
};

VITTE_BENCH_CONSTRUCTOR static void parse_suite_autoregister(void)
{
    bench_register_suite(&k_suite);
}

// Optional explicit symbol for runners that call suites manually.
void vitte_bench_register_parse_suite(void)
{
    bench_register_suite(&k_suite);
}

// Optional teardown hook.
void vitte_bench_parse_suite_teardown(void)
{
    if (g_fx_inited)
    {
        tokbuf_free(&g_tok);
        arena_free(&g_arena);
        g_fx_inited = false;
    }
}