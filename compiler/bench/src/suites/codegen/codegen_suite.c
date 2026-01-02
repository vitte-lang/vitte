

/* suite: codegen

  Code generation microbenchmarks for the Vitte benchmark harness.

  Scope
  - These tests measure "codegen-like" workloads without requiring the full
    compiler pipeline (parsing/IR/sema). The intent is to isolate:
      * string builder / buffer growth behavior
      * formatting throughput (appendf)
      * token emission patterns
      * small IR-to-text patterns (mini AST -> text)
      * escaping and identifier mangling

  The suite uses the harness arena allocator (bench_alloc) to keep overhead
  stable and to match typical compiler allocator patterns.

  NOTE
  - This suite does not attempt to be a whole-program benchmark.
  - It should be stable across versions to make regressions visible.
*/

#include "bench/bench.h"
#include "../suites/suite_common.h"

#include "bench/detail/format.h" /* vitte_buf */
#include "bench/str.h"
#include "bench/time.h"

#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Helpers                                                                     */
/* -------------------------------------------------------------------------- */

static inline uint64_t u64_mix(uint64_t x) {
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return x;
}

static inline uint32_t u32_rand(bench_ctx *ctx) {
  return bench_rand_u32(ctx);
}

static inline void *aalloc(bench_ctx *ctx, size_t n) {
  return bench_alloc(ctx, n);
}

static inline void mem_fill(unsigned char *p, size_t n, uint32_t seed) {
  uint64_t s = u64_mix(((uint64_t)seed << 1) | 1ULL);
  for (size_t i = 0; i < n; i += 64) {
    s = u64_mix(s + (uint64_t)i);
    p[i] = (unsigned char)(s & 0xFFu);
  }
  if (n) p[n - 1] ^= (unsigned char)(seed & 0xFFu);
}

static inline uint64_t mem_checksum(const unsigned char *p, size_t n) {
  uint64_t acc = 0x9e3779b97f4a7c15ULL;
  for (size_t i = 0; i < n; i += 97) {
    acc ^= (uint64_t)p[i] + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
  }
  if (n) {
    acc ^= (uint64_t)p[n - 1] + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
  }
  return acc;
}

static inline bool buf_init_arena(vitte_buf *b, bench_ctx *ctx, size_t cap) {
  if (!b) return false;
  memset(b, 0, sizeof(*b));

  /* Try to reserve an initial buffer from arena (deterministic). */
  if (cap) {
    char *p = (char *)aalloc(ctx, cap);
    if (!p) return false;
    b->data = p;
    b->len = 0;
    b->cap = cap;
  }
  return true;
}

/* Minimal escaping for string literals. */
static inline void emit_escaped(vitte_buf *out, const unsigned char *s, size_t n) {
  static const char hexd[] = "0123456789abcdef";

  for (size_t i = 0; i < n; ++i) {
    unsigned c = (unsigned)s[i];
    switch (c) {
      case '\\': (void)vitte_buf_append(out, "\\\\", 2); break;
      case '"':  (void)vitte_buf_append(out, "\\\"", 2); break;
      case '\n': (void)vitte_buf_append(out, "\\n", 2); break;
      case '\r': (void)vitte_buf_append(out, "\\r", 2); break;
      case '\t': (void)vitte_buf_append(out, "\\t", 2); break;
      default:
        if (c >= 32 && c < 127) {
          char ch = (char)c;
          (void)vitte_buf_append(out, &ch, 1);
        } else {
          char tmp[4];
          tmp[0] = '\\';
          tmp[1] = 'x';
          tmp[2] = hexd[(c >> 4) & 0xFu];
          tmp[3] = hexd[c & 0xFu];
          (void)vitte_buf_append(out, tmp, 4);
        }
        break;
    }
  }
}

/* Identifier mangling: keep ASCII [a-zA-Z0-9_], escape others to _uXXXX_. */
static inline void emit_mangled_ident(vitte_buf *out, const unsigned char *s, size_t n) {
  static const char hexd[] = "0123456789abcdef";

  for (size_t i = 0; i < n; ++i) {
    unsigned c = (unsigned)s[i];
    const bool ok = (c == '_') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    if (ok) {
      char ch = (char)c;
      (void)vitte_buf_append(out, &ch, 1);
      continue;
    }

    char tmp[8];
    tmp[0] = '_';
    tmp[1] = 'u';
    tmp[2] = hexd[(c >> 4) & 0xFu];
    tmp[3] = hexd[c & 0xFu];
    tmp[4] = hexd[(c >> 4) & 0xFu];
    tmp[5] = hexd[c & 0xFu];
    tmp[6] = '_';
    tmp[7] = 0;
    (void)vitte_buf_append(out, tmp, 7);
  }
}

/* -------------------------------------------------------------------------- */
/* Case: token emission                                                        */
/* -------------------------------------------------------------------------- */

BENCH_CASE("codegen", "emit_tokens_1m", bm_codegen_emit_tokens_1m) {
  /* Emit ~1M small tokens into a buffer. */
  const size_t tokens = 1u * 1000u * 1000u;

  vitte_buf b;
  (void)buf_init_arena(&b, ctx, 1u << 20); /* 1 MiB initial */

  uint64_t acc = 0;
  for (size_t i = 0; i < tokens; ++i) {
    uint32_t r = u32_rand(ctx);
    switch (r & 7u) {
      case 0: (void)vitte_buf_append(&b, "fn ", 3); break;
      case 1: (void)vitte_buf_append(&b, "let ", 4); break;
      case 2: (void)vitte_buf_append(&b, "ret ", 4); break;
      case 3: (void)vitte_buf_append(&b, "if ", 3); break;
      case 4: (void)vitte_buf_append(&b, "+", 1); break;
      case 5: (void)vitte_buf_append(&b, "-", 1); break;
      case 6: (void)vitte_buf_append(&b, "*", 1); break;
      default:(void)vitte_buf_append(&b, ";\n", 2); break;
    }

    /* occasional identifier */
    if ((r & 255u) == 0u) {
      char tmp[32];
      (void)vitte_buf_append(&b, "v", 1);
      (void)vitte_buf_appendf(&b, "%u", (unsigned)(r ^ (uint32_t)i));
      (void)vitte_buf_append(&b, "_", 1);
      (void)vitte_buf_append(&b, "x", 1);
      tmp[0] = 0;
    }

    if ((i & 1023u) == 0u) {
      acc ^= (uint64_t)b.len;
    }
  }

  acc ^= (uint64_t)b.len;
  volatile uint64_t v = acc;
  (void)v;
}

/* -------------------------------------------------------------------------- */
/* Case: appendf formatting                                                     */
/* -------------------------------------------------------------------------- */

BENCH_CASE("codegen", "appendf_200k_lines", bm_codegen_appendf_200k_lines) {
  /* Produce ~200k formatted lines (like emitting IR/debug dumps). */
  const size_t lines = 200000u;

  vitte_buf b;
  (void)buf_init_arena(&b, ctx, 1u << 20);

  uint64_t acc = 0;
  for (size_t i = 0; i < lines; ++i) {
    uint32_t r = u32_rand(ctx);
    unsigned a = (unsigned)(r & 0xFFFFu);
    unsigned b0 = (unsigned)((r >> 16) & 0xFFFFu);
    unsigned op = (unsigned)(r & 7u);

    /* keep format stable */
    (void)vitte_buf_appendf(&b, "bb%u: v%u = op%u v%u, v%u ; # %u\n",
                            (unsigned)(i & 4095u),
                            (unsigned)(i & 65535u),
                            op,
                            a,
                            b0,
                            (unsigned)i);

    if ((i & 4095u) == 0u) acc ^= (uint64_t)b.len;
  }

  acc ^= (uint64_t)b.len;
  volatile uint64_t v = acc;
  (void)v;
}

/* -------------------------------------------------------------------------- */
/* Case: escape string literals                                                 */
/* -------------------------------------------------------------------------- */

BENCH_CASE("codegen", "escape_literals_50k", bm_codegen_escape_literals_50k) {
  const size_t nlit = 50000u;
  const size_t lit_sz = 64u;

  /* build input pool in arena */
  unsigned char *pool = (unsigned char *)aalloc(ctx, nlit * lit_sz);
  mem_fill(pool, nlit * lit_sz, u32_rand(ctx));

  vitte_buf out;
  (void)buf_init_arena(&out, ctx, 1u << 20);

  uint64_t acc = 0;
  for (size_t i = 0; i < nlit; ++i) {
    const unsigned char *s = pool + i * lit_sz;
    (void)vitte_buf_append(&out, "\"", 1);
    emit_escaped(&out, s, lit_sz);
    (void)vitte_buf_append(&out, "\"\n", 2);

    if ((i & 1023u) == 0u) acc ^= (uint64_t)out.len;
  }

  acc ^= (uint64_t)out.len;
  volatile uint64_t v = acc;
  (void)v;
}

/* -------------------------------------------------------------------------- */
/* Case: mangle identifiers                                                     */
/* -------------------------------------------------------------------------- */

BENCH_CASE("codegen", "mangle_ident_200k", bm_codegen_mangle_ident_200k) {
  const size_t n = 200000u;
  const size_t sz = 20u;

  unsigned char *pool = (unsigned char *)aalloc(ctx, n * sz);
  mem_fill(pool, n * sz, 0xA5A5A5A5u ^ u32_rand(ctx));

  vitte_buf out;
  (void)buf_init_arena(&out, ctx, 1u << 20);

  uint64_t acc = 0;
  for (size_t i = 0; i < n; ++i) {
    const unsigned char *s = pool + i * sz;
    emit_mangled_ident(&out, s, sz);
    (void)vitte_buf_append(&out, "\n", 1);

    if ((i & 4095u) == 0u) acc ^= (uint64_t)out.len;
  }

  acc ^= (uint64_t)out.len;
  volatile uint64_t v = acc;
  (void)v;
}

/* -------------------------------------------------------------------------- */
/* Case: tiny AST -> text                                                       */
/* -------------------------------------------------------------------------- */

typedef enum tiny_op {
  T_OP_ADD,
  T_OP_SUB,
  T_OP_MUL,
  T_OP_XOR,
} tiny_op;

typedef struct tiny_expr {
  uint32_t kind; /* 0=imm, 1=bin */
  uint32_t imm;
  tiny_op op;
  uint32_t lhs;
  uint32_t rhs;
} tiny_expr;

static inline void emit_op(vitte_buf *out, tiny_op op) {
  switch (op) {
    case T_OP_ADD: (void)vitte_buf_append(out, "+", 1); break;
    case T_OP_SUB: (void)vitte_buf_append(out, "-", 1); break;
    case T_OP_MUL: (void)vitte_buf_append(out, "*", 1); break;
    default:       (void)vitte_buf_append(out, "^", 1); break;
  }
}

static inline void emit_expr(vitte_buf *out, const tiny_expr *es, uint32_t id, uint32_t depth) {
  const tiny_expr *e = &es[id];
  if (e->kind == 0u || depth == 0u) {
    (void)vitte_buf_appendf(out, "%u", (unsigned)e->imm);
    return;
  }

  (void)vitte_buf_append(out, "(", 1);
  emit_expr(out, es, e->lhs, depth - 1u);
  (void)vitte_buf_append(out, " ", 1);
  emit_op(out, e->op);
  (void)vitte_buf_append(out, " ", 1);
  emit_expr(out, es, e->rhs, depth - 1u);
  (void)vitte_buf_append(out, ")", 1);
}

BENCH_CASE("codegen", "tiny_ast_emit_100k", bm_codegen_tiny_ast_emit_100k) {
  const uint32_t exprs = 100000u;

  tiny_expr *es = (tiny_expr *)aalloc(ctx, (size_t)exprs * sizeof(tiny_expr));

  /* Build a deterministic forest */
  for (uint32_t i = 0; i < exprs; ++i) {
    uint32_t r = u32_rand(ctx) ^ (i * 2654435761u);
    tiny_expr e;
    e.kind = (r & 3u) ? 1u : 0u;
    e.imm = r & 1023u;
    e.op = (tiny_op)((r >> 10) & 3u);
    e.lhs = (i > 0) ? (uint32_t)(r % i) : 0u;
    e.rhs = (i > 1) ? (uint32_t)((r >> 16) % i) : 0u;
    es[i] = e;
  }

  vitte_buf out;
  (void)buf_init_arena(&out, ctx, 1u << 20);

  uint64_t acc = 0;
  for (uint32_t i = 0; i < exprs; ++i) {
    (void)vitte_buf_append(&out, "let v", 5);
    (void)vitte_buf_appendf(&out, "%u", (unsigned)i);
    (void)vitte_buf_append(&out, " = ", 3);
    emit_expr(&out, es, i, 4u);
    (void)vitte_buf_append(&out, ";\n", 2);

    if ((i & 4095u) == 0u) acc ^= (uint64_t)out.len;
  }

  acc ^= (uint64_t)out.len;
  volatile uint64_t v = acc;
  (void)v;
}

/* -------------------------------------------------------------------------- */
/* Case: buffer growth patterns                                                 */
/* -------------------------------------------------------------------------- */

BENCH_CASE("codegen", "buf_growth_geometric", bm_codegen_buf_growth_geometric) {
  /* Simulate repeated growth via append() calls; measure amortized behavior. */
  const size_t iters = 300000u;
  const size_t chunk = 24u;

  vitte_buf out;
  (void)buf_init_arena(&out, ctx, 256u);

  unsigned char tmp[chunk];
  mem_fill(tmp, chunk, u32_rand(ctx));

  uint64_t acc = 0;
  for (size_t i = 0; i < iters; ++i) {
    (void)vitte_buf_append(&out, (const char *)tmp, chunk);
    tmp[i % chunk] ^= (unsigned char)i;

    if ((i & 8191u) == 0u) acc ^= (uint64_t)out.len;
  }

  acc ^= (uint64_t)out.len;
  volatile uint64_t v = acc;
  (void)v;
}

/* -------------------------------------------------------------------------- */
/* Suite init                                                                   */
/* -------------------------------------------------------------------------- */

void bench_suite_codegen_init(void) {
  BENCH_REG(bm_codegen_emit_tokens_1m);
  BENCH_REG(bm_codegen_appendf_200k_lines);
  BENCH_REG(bm_codegen_escape_literals_50k);
  BENCH_REG(bm_codegen_mangle_ident_200k);
  BENCH_REG(bm_codegen_tiny_ast_emit_100k);
  BENCH_REG(bm_codegen_buf_growth_geometric);
}