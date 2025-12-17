// json_parser.c
// -----------------------------------------------------------------------------
// Lightweight, allocation-friendly JSON parser intended for benchmarking.
//
// Design goals:
//   - C17, single translation unit, no external deps
//   - predictable performance (arena allocation, minimal copies)
//   - reasonably strict JSON (RFC 8259-ish) with pragmatic behavior
//   - provide a stable digest/hash of the parsed value to prevent DCE in benches
//
// Notes:
//   - Strings are stored as slices into the original input when possible.
//     If unescaping is required, a copy is created in the arena.
//   - Numbers are parsed into double (and int64 when safe) using a fast path
//     for common integer/decimal forms; falls back to strtod for edge cases.
//   - This is not a validating security-grade parser; it is a benchmark tool.
// -----------------------------------------------------------------------------

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Public-ish API (keep this file self-contained; benchmarks can include it).
// -----------------------------------------------------------------------------

typedef enum JsonType {
    JSON_NULL = 0,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
} JsonType;

typedef struct JsonStr {
    const char* ptr;
    uint32_t    len;
    uint32_t    owns_copy; // 0 = slice into input, 1 = arena copy
} JsonStr;

typedef struct JsonValue JsonValue;

typedef struct JsonArray {
    JsonValue* items;
    uint32_t   count;
    uint32_t   cap;
} JsonArray;

typedef struct JsonMember {
    JsonStr    key;
    JsonValue* value;
} JsonMember;

typedef struct JsonObject {
    JsonMember* members;
    uint32_t    count;
    uint32_t    cap;
} JsonObject;

struct JsonValue {
    JsonType type;
    union {
        bool      b;
        double    num;
        JsonStr   str;
        JsonArray arr;
        JsonObject obj;
    } as;
};

typedef struct JsonError {
    size_t      offset;      // byte offset into source
    const char* message;     // static string
} JsonError;

typedef struct JsonArena {
    uint8_t* data;
    size_t   size;
    size_t   used;
} JsonArena;

typedef struct JsonDoc {
    JsonValue* root;
    size_t     consumed; // bytes consumed (for embedded JSON)
} JsonDoc;

// Arena API
static void json_arena_init(JsonArena* a, size_t initial_size);
static void json_arena_reset(JsonArena* a);
static void json_arena_free(JsonArena* a);
static void* json_arena_alloc(JsonArena* a, size_t size, size_t align);

// Parse API
static JsonDoc json_parse(const char* src, size_t len, JsonArena* arena, JsonError* err);

// Digest to keep benchmark honest
static uint64_t json_digest_u64(const JsonValue* v);

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

static inline uint64_t fnv1a_u64(const void* data, size_t n, uint64_t h)
{
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static inline uint64_t mix_u64(uint64_t x)
{
    // SplitMix64 finalizer-ish
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static inline bool json_is_ws(unsigned char c)
{
    // JSON allows: space, tab, CR, LF
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

// -----------------------------------------------------------------------------
// Arena
// -----------------------------------------------------------------------------

static void json_arena_init(JsonArena* a, size_t initial_size)
{
    if (!a) return;
    if (initial_size < 4096) initial_size = 4096;
    a->data = (uint8_t*)malloc(initial_size);
    a->size = a->data ? initial_size : 0;
    a->used = 0;
}

static void json_arena_reset(JsonArena* a)
{
    if (!a) return;
    a->used = 0;
}

static void json_arena_free(JsonArena* a)
{
    if (!a) return;
    free(a->data);
    a->data = NULL;
    a->size = 0;
    a->used = 0;
}

static void* json_arena_alloc(JsonArena* a, size_t size, size_t align)
{
    if (!a || size == 0) return NULL;
    if (align < 1) align = 1;
    if ((align & (align - 1)) != 0) {
        // align must be power of two
        align = (size_t)1u << (size_t)(8u * sizeof(size_t) - (size_t)__builtin_clzl(align - 1));
    }

    size_t p = a->used;
    size_t aligned = (p + (align - 1)) & ~(align - 1);
    size_t end = aligned + size;

    if (end > a->size) {
        // grow
        size_t new_size = a->size ? a->size : 4096;
        while (new_size < end) {
            new_size = (new_size < (SIZE_MAX / 2)) ? (new_size * 2) : SIZE_MAX;
            if (new_size == SIZE_MAX) break;
        }
        if (new_size < end) return NULL;
        uint8_t* nd = (uint8_t*)realloc(a->data, new_size);
        if (!nd) return NULL;
        a->data = nd;
        a->size = new_size;
    }

    void* out = a->data + aligned;
    a->used = end;
    return out;
}

// -----------------------------------------------------------------------------
// Parser state
// -----------------------------------------------------------------------------

typedef struct JsonParser {
    const char* s;
    size_t      n;
    size_t      i;
    JsonArena*  a;
    JsonError*  e;
} JsonParser;

static void json_set_err(JsonParser* p, const char* msg)
{
    if (!p || !p->e) return;
    if (p->e->message) return; // keep first error
    p->e->offset = p->i;
    p->e->message = msg;
}

static inline bool json_ok(const JsonParser* p)
{
    return p && (!p->e || p->e->message == NULL);
}

static inline bool json_eof(const JsonParser* p)
{
    return (p->i >= p->n);
}

static inline unsigned char json_peek(const JsonParser* p)
{
    return json_eof(p) ? 0 : (unsigned char)p->s[p->i];
}

static inline unsigned char json_get(JsonParser* p)
{
    return json_eof(p) ? 0 : (unsigned char)p->s[p->i++];
}

static void json_skip_ws(JsonParser* p)
{
    while (!json_eof(p) && json_is_ws((unsigned char)p->s[p->i])) {
        p->i++;
    }
}

static JsonValue* json_new_value(JsonParser* p, JsonType t)
{
    JsonValue* v = (JsonValue*)json_arena_alloc(p->a, sizeof(JsonValue), _Alignof(JsonValue));
    if (!v) {
        json_set_err(p, "out of memory");
        return NULL;
    }
    memset(v, 0, sizeof(*v));
    v->type = t;
    return v;
}

static bool json_match_lit(JsonParser* p, const char* lit)
{
    size_t k = 0;
    while (lit[k]) {
        if (p->i + k >= p->n) return false;
        if (p->s[p->i + k] != lit[k]) return false;
        k++;
    }
    p->i += k;
    return true;
}

// -----------------------------------------------------------------------------
// String parsing
// -----------------------------------------------------------------------------

static int json_hex_nibble(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static bool json_append_utf8(uint8_t** out, uint8_t* end, uint32_t cp)
{
    // returns false if no space
    if (cp <= 0x7F) {
        if (*out + 1 > end) return false;
        *(*out)++ = (uint8_t)cp;
        return true;
    }
    if (cp <= 0x7FF) {
        if (*out + 2 > end) return false;
        *(*out)++ = (uint8_t)(0xC0 | (cp >> 6));
        *(*out)++ = (uint8_t)(0x80 | (cp & 0x3F));
        return true;
    }
    if (cp <= 0xFFFF) {
        if (*out + 3 > end) return false;
        *(*out)++ = (uint8_t)(0xE0 | (cp >> 12));
        *(*out)++ = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
        *(*out)++ = (uint8_t)(0x80 | (cp & 0x3F));
        return true;
    }
    if (cp <= 0x10FFFF) {
        if (*out + 4 > end) return false;
        *(*out)++ = (uint8_t)(0xF0 | (cp >> 18));
        *(*out)++ = (uint8_t)(0x80 | ((cp >> 12) & 0x3F));
        *(*out)++ = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
        *(*out)++ = (uint8_t)(0x80 | (cp & 0x3F));
        return true;
    }
    return false;
}

static JsonStr json_parse_string(JsonParser* p)
{
    JsonStr out = {0};

    // Expect opening '"'
    if (json_get(p) != '"') {
        json_set_err(p, "expected string");
        return out;
    }

    size_t start = p->i;
    bool needs_copy = false;

    // First pass: find closing quote; detect escapes/control characters
    for (;;) {
        if (json_eof(p)) {
            json_set_err(p, "unterminated string");
            return out;
        }
        unsigned char c = (unsigned char)p->s[p->i++];
        if (c == '"') break;
        if (c == '\\') {
            needs_copy = true;
            if (json_eof(p)) {
                json_set_err(p, "unterminated escape");
                return out;
            }
            p->i++; // skip escaped char (or 'u')
            continue;
        }
        if (c < 0x20) {
            json_set_err(p, "control character in string");
            return out;
        }
    }

    size_t end_quote = p->i - 1;
    size_t raw_len = end_quote - start;

    if (!needs_copy) {
        // Zero-copy slice into input
        if (raw_len > UINT32_MAX) {
            json_set_err(p, "string too long");
            return out;
        }
        out.ptr = p->s + start;
        out.len = (uint32_t)raw_len;
        out.owns_copy = 0;
        return out;
    }

    // Second pass: unescape into arena.
    // Worst-case size is raw_len (escapes shrink), allocate raw_len + 1.
    if (raw_len + 1 > SIZE_MAX) {
        json_set_err(p, "string too long");
        return out;
    }

    char* buf = (char*)json_arena_alloc(p->a, raw_len + 1, 1);
    if (!buf) {
        json_set_err(p, "out of memory");
        return out;
    }

    // Rewind to start for unescape.
    size_t j = start;
    uint8_t* w = (uint8_t*)buf;
    uint8_t* wend = (uint8_t*)buf + raw_len;

    while (j < end_quote) {
        unsigned char c = (unsigned char)p->s[j++];
        if (c != '\\') {
            if (w >= wend) {
                json_set_err(p, "string overflow");
                return (JsonStr){0};
            }
            *w++ = (uint8_t)c;
            continue;
        }

        if (j >= end_quote) {
            json_set_err(p, "bad escape");
            return (JsonStr){0};
        }

        unsigned char esc = (unsigned char)p->s[j++];
        switch (esc) {
            case '"': if (w >= wend) { json_set_err(p, "string overflow"); return (JsonStr){0}; } *w++ = '"'; break;
            case '\\': if (w >= wend) { json_set_err(p, "string overflow"); return (JsonStr){0}; } *w++ = '\\'; break;
            case '/': if (w >= wend) { json_set_err(p, "string overflow"); return (JsonStr){0}; } *w++ = '/'; break;
            case 'b': if (w >= wend) { json_set_err(p, "string overflow"); return (JsonStr){0}; } *w++ = '\b'; break;
            case 'f': if (w >= wend) { json_set_err(p, "string overflow"); return (JsonStr){0}; } *w++ = '\f'; break;
            case 'n': if (w >= wend) { json_set_err(p, "string overflow"); return (JsonStr){0}; } *w++ = '\n'; break;
            case 'r': if (w >= wend) { json_set_err(p, "string overflow"); return (JsonStr){0}; } *w++ = '\r'; break;
            case 't': if (w >= wend) { json_set_err(p, "string overflow"); return (JsonStr){0}; } *w++ = '\t'; break;
            case 'u': {
                if (j + 4 > end_quote) {
                    json_set_err(p, "bad unicode escape");
                    return (JsonStr){0};
                }
                int h0 = json_hex_nibble((unsigned char)p->s[j + 0]);
                int h1 = json_hex_nibble((unsigned char)p->s[j + 1]);
                int h2 = json_hex_nibble((unsigned char)p->s[j + 2]);
                int h3 = json_hex_nibble((unsigned char)p->s[j + 3]);
                if (h0 < 0 || h1 < 0 || h2 < 0 || h3 < 0) {
                    json_set_err(p, "bad unicode escape");
                    return (JsonStr){0};
                }
                uint32_t cp = (uint32_t)((h0 << 12) | (h1 << 8) | (h2 << 4) | h3);
                j += 4;

                // Handle surrogate pairs
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    // high surrogate, must be followed by \u low surrogate
                    if (j + 6 <= end_quote && p->s[j] == '\\' && p->s[j + 1] == 'u') {
                        int a0 = json_hex_nibble((unsigned char)p->s[j + 2]);
                        int a1 = json_hex_nibble((unsigned char)p->s[j + 3]);
                        int a2 = json_hex_nibble((unsigned char)p->s[j + 4]);
                        int a3 = json_hex_nibble((unsigned char)p->s[j + 5]);
                        if (a0 >= 0 && a1 >= 0 && a2 >= 0 && a3 >= 0) {
                            uint32_t lo = (uint32_t)((a0 << 12) | (a1 << 8) | (a2 << 4) | a3);
                            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                                j += 6;
                                uint32_t hi = cp - 0xD800;
                                uint32_t low = lo - 0xDC00;
                                cp = 0x10000 + ((hi << 10) | low);
                            }
                        }
                    }
                }

                // Write as UTF-8
                uint8_t* tmp = w;
                if (!json_append_utf8(&tmp, wend, cp)) {
                    json_set_err(p, "string overflow");
                    return (JsonStr){0};
                }
                w = tmp;
            } break;
            default:
                json_set_err(p, "invalid escape");
                return (JsonStr){0};
        }
    }

    *w = 0;

    size_t out_len = (size_t)(w - (uint8_t*)buf);
    if (out_len > UINT32_MAX) {
        json_set_err(p, "string too long");
        return (JsonStr){0};
    }

    out.ptr = buf;
    out.len = (uint32_t)out_len;
    out.owns_copy = 1;
    return out;
}

// -----------------------------------------------------------------------------
// Number parsing
// -----------------------------------------------------------------------------

static bool json_is_digit(unsigned char c) { return (c >= '0' && c <= '9'); }

static bool json_parse_number_fast(const char* s, size_t n, size_t* io_i, double* out)
{
    // Fast path: parse common JSON numbers without exponent using integer + frac.
    // Returns true if parsed; false if should fallback.

    size_t i = *io_i;
    if (i >= n) return false;

    bool neg = false;
    if (s[i] == '-') {
        neg = true;
        i++;
        if (i >= n) return false;
    }

    if (!json_is_digit((unsigned char)s[i])) return false;

    // Integer part
    uint64_t ip = 0;
    size_t digits = 0;
    if (s[i] == '0') {
        i++;
        digits = 1;
    } else {
        while (i < n && json_is_digit((unsigned char)s[i])) {
            if (digits < 19) {
                ip = ip * 10u + (uint64_t)(s[i] - '0');
            } else {
                // too many digits: risk overflow/precision issues -> fallback
                return false;
            }
            digits++;
            i++;
        }
    }

    double v = (double)ip;

    // Fraction
    if (i < n && s[i] == '.') {
        i++;
        if (i >= n || !json_is_digit((unsigned char)s[i])) return false;
        double scale = 1.0;
        size_t frac_digits = 0;
        while (i < n && json_is_digit((unsigned char)s[i])) {
            if (frac_digits < 18) {
                scale *= 0.1;
                v += (double)(s[i] - '0') * scale;
            } else {
                // too many digits -> fallback for accuracy
                return false;
            }
            frac_digits++;
            i++;
        }
    }

    // Exponent? Let strtod handle to be safe.
    if (i < n && (s[i] == 'e' || s[i] == 'E')) {
        return false;
    }

    if (neg) v = -v;
    *out = v;
    *io_i = i;
    return true;
}

static double json_parse_number(JsonParser* p)
{
    size_t i0 = p->i;
    size_t i = p->i;
    double v = 0.0;

    if (json_parse_number_fast(p->s, p->n, &i, &v)) {
        p->i = i;
        return v;
    }

    // Fallback: strtod (needs NUL-terminated buffer). We'll copy a bounded slice.
    // Determine number span per JSON number grammar.
    i = i0;
    if (i < p->n && (p->s[i] == '-')) i++;
    if (i >= p->n) { json_set_err(p, "invalid number"); return 0.0; }

    if (p->s[i] == '0') {
        i++;
    } else if (json_is_digit((unsigned char)p->s[i])) {
        while (i < p->n && json_is_digit((unsigned char)p->s[i])) i++;
    } else {
        json_set_err(p, "invalid number");
        return 0.0;
    }

    if (i < p->n && p->s[i] == '.') {
        i++;
        if (i >= p->n || !json_is_digit((unsigned char)p->s[i])) {
            json_set_err(p, "invalid number");
            return 0.0;
        }
        while (i < p->n && json_is_digit((unsigned char)p->s[i])) i++;
    }

    if (i < p->n && (p->s[i] == 'e' || p->s[i] == 'E')) {
        i++;
        if (i < p->n && (p->s[i] == '+' || p->s[i] == '-')) i++;
        if (i >= p->n || !json_is_digit((unsigned char)p->s[i])) {
            json_set_err(p, "invalid number");
            return 0.0;
        }
        while (i < p->n && json_is_digit((unsigned char)p->s[i])) i++;
    }

    size_t span = i - i0;
    if (span == 0 || span > 256) {
        // overly large or empty
        json_set_err(p, "invalid number");
        return 0.0;
    }

    char tmp[257];
    memcpy(tmp, p->s + i0, span);
    tmp[span] = 0;

    errno = 0;
    char* endptr = NULL;
    double dv = strtod(tmp, &endptr);
    if (errno != 0 || endptr == tmp) {
        json_set_err(p, "invalid number");
        return 0.0;
    }

    p->i = i;
    return dv;
}

// -----------------------------------------------------------------------------
// Forward decls for recursive descent
// -----------------------------------------------------------------------------

static JsonValue* json_parse_value(JsonParser* p);

static bool json_expect(JsonParser* p, char ch, const char* msg)
{
    json_skip_ws(p);
    if (json_eof(p) || p->s[p->i] != ch) {
        json_set_err(p, msg);
        return false;
    }
    p->i++;
    return true;
}

static void json_array_push(JsonParser* p, JsonArray* a, JsonValue* v)
{
    if (!a || !v) return;
    if (a->count == a->cap) {
        uint32_t new_cap = (a->cap == 0) ? 8u : (a->cap * 2u);
        size_t new_bytes = (size_t)new_cap * sizeof(JsonValue);
        JsonValue* ni = (JsonValue*)json_arena_alloc(p->a, new_bytes, _Alignof(JsonValue));
        if (!ni) {
            json_set_err(p, "out of memory");
            return;
        }
        if (a->items && a->count) {
            memcpy(ni, a->items, (size_t)a->count * sizeof(JsonValue));
        }
        a->items = ni;
        a->cap = new_cap;
    }
    // Copy value object into contiguous array to keep traversal cache-friendly.
    a->items[a->count++] = *v;
}

static void json_object_push(JsonParser* p, JsonObject* o, JsonStr key, JsonValue* v)
{
    if (!o || !v) return;
    if (o->count == o->cap) {
        uint32_t new_cap = (o->cap == 0) ? 8u : (o->cap * 2u);
        size_t new_bytes = (size_t)new_cap * sizeof(JsonMember);
        JsonMember* nm = (JsonMember*)json_arena_alloc(p->a, new_bytes, _Alignof(JsonMember));
        if (!nm) {
            json_set_err(p, "out of memory");
            return;
        }
        if (o->members && o->count) {
            memcpy(nm, o->members, (size_t)o->count * sizeof(JsonMember));
        }
        o->members = nm;
        o->cap = new_cap;
    }
    o->members[o->count].key = key;
    // Store value as a copy in arena to avoid pointer chasing.
    JsonValue* slot = (JsonValue*)json_arena_alloc(p->a, sizeof(JsonValue), _Alignof(JsonValue));
    if (!slot) {
        json_set_err(p, "out of memory");
        return;
    }
    *slot = *v;
    o->members[o->count].value = slot;
    o->count++;
}

static JsonValue* json_parse_array(JsonParser* p)
{
    JsonValue* v = json_new_value(p, JSON_ARRAY);
    if (!v) return NULL;

    if (!json_expect(p, '[', "expected '['")) return v;
    json_skip_ws(p);

    if (!json_eof(p) && p->s[p->i] == ']') {
        p->i++;
        return v;
    }

    for (;;) {
        JsonValue* item = json_parse_value(p);
        if (!json_ok(p)) return v;
        json_array_push(p, &v->as.arr, item);
        if (!json_ok(p)) return v;

        json_skip_ws(p);
        if (json_eof(p)) {
            json_set_err(p, "unterminated array");
            return v;
        }
        char c = (char)p->s[p->i];
        if (c == ',') {
            p->i++;
            continue;
        }
        if (c == ']') {
            p->i++;
            return v;
        }
        json_set_err(p, "expected ',' or ']' in array");
        return v;
    }
}

static JsonValue* json_parse_object(JsonParser* p)
{
    JsonValue* v = json_new_value(p, JSON_OBJECT);
    if (!v) return NULL;

    if (!json_expect(p, '{', "expected '{'")) return v;
    json_skip_ws(p);

    if (!json_eof(p) && p->s[p->i] == '}') {
        p->i++;
        return v;
    }

    for (;;) {
        json_skip_ws(p);
        if (json_eof(p) || p->s[p->i] != '"') {
            json_set_err(p, "expected string key");
            return v;
        }

        JsonStr key = json_parse_string(p);
        if (!json_ok(p)) return v;

        if (!json_expect(p, ':', "expected ':' after key")) return v;

        JsonValue* val = json_parse_value(p);
        if (!json_ok(p)) return v;

        json_object_push(p, &v->as.obj, key, val);
        if (!json_ok(p)) return v;

        json_skip_ws(p);
        if (json_eof(p)) {
            json_set_err(p, "unterminated object");
            return v;
        }

        char c = (char)p->s[p->i];
        if (c == ',') {
            p->i++;
            continue;
        }
        if (c == '}') {
            p->i++;
            return v;
        }

        json_set_err(p, "expected ',' or '}' in object");
        return v;
    }
}

static JsonValue* json_parse_value(JsonParser* p)
{
    json_skip_ws(p);
    if (json_eof(p)) {
        json_set_err(p, "unexpected end of input");
        return json_new_value(p, JSON_NULL);
    }

    unsigned char c = json_peek(p);

    if (c == 'n') {
        JsonValue* v = json_new_value(p, JSON_NULL);
        if (json_match_lit(p, "null")) return v;
        json_set_err(p, "invalid literal");
        return v;
    }

    if (c == 't') {
        JsonValue* v = json_new_value(p, JSON_BOOL);
        if (json_match_lit(p, "true")) { v->as.b = true; return v; }
        json_set_err(p, "invalid literal");
        return v;
    }

    if (c == 'f') {
        JsonValue* v = json_new_value(p, JSON_BOOL);
        if (json_match_lit(p, "false")) { v->as.b = false; return v; }
        json_set_err(p, "invalid literal");
        return v;
    }

    if (c == '"') {
        JsonValue* v = json_new_value(p, JSON_STRING);
        v->as.str = json_parse_string(p);
        return v;
    }

    if (c == '[') {
        return json_parse_array(p);
    }

    if (c == '{') {
        return json_parse_object(p);
    }

    // number
    if (c == '-' || json_is_digit(c)) {
        JsonValue* v = json_new_value(p, JSON_NUMBER);
        v->as.num = json_parse_number(p);
        return v;
    }

    json_set_err(p, "unexpected character");
    return json_new_value(p, JSON_NULL);
}

static JsonDoc json_parse(const char* src, size_t len, JsonArena* arena, JsonError* err)
{
    JsonDoc doc = {0};
    if (err) {
        err->offset = 0;
        err->message = NULL;
    }

    JsonParser p = {
        .s = src,
        .n = len,
        .i = 0,
        .a = arena,
        .e = err,
    };

    if (!src) {
        json_set_err(&p, "null input");
        return doc;
    }

    JsonValue* root = json_parse_value(&p);
    if (json_ok(&p)) {
        json_skip_ws(&p);
        doc.root = root;
        doc.consumed = p.i;
    }

    return doc;
}

// -----------------------------------------------------------------------------
// Digest/hash
// -----------------------------------------------------------------------------

static uint64_t json_digest_str(JsonStr s, uint64_t h)
{
    h = fnv1a_u64(s.ptr, s.len, h);
    h ^= (uint64_t)s.len;
    return mix_u64(h);
}

static uint64_t json_digest_u64(const JsonValue* v)
{
    if (!v) return 0;

    uint64_t h = 1469598103934665603ULL;
    h ^= (uint64_t)v->type;
    h *= 1099511628211ULL;

    switch (v->type) {
        case JSON_NULL:
            return mix_u64(h ^ 0xA11ULL);
        case JSON_BOOL:
            h ^= (uint64_t)(v->as.b ? 0xB001ULL : 0xB000ULL);
            return mix_u64(h);
        case JSON_NUMBER: {
            // Normalize NaN to a stable value; otherwise hash the IEEE bits.
            double d = v->as.num;
            uint64_t bits = 0;
            if (isnan(d)) {
                bits = 0x7ff8000000000000ULL;
            } else {
                memcpy(&bits, &d, sizeof(bits));
            }
            h ^= bits;
            h *= 1099511628211ULL;
            return mix_u64(h);
        }
        case JSON_STRING:
            return json_digest_str(v->as.str, h);
        case JSON_ARRAY: {
            h ^= (uint64_t)v->as.arr.count;
            h = mix_u64(h);
            for (uint32_t i = 0; i < v->as.arr.count; ++i) {
                h ^= json_digest_u64(&v->as.arr.items[i]);
                h = mix_u64(h);
            }
            return h;
        }
        case JSON_OBJECT: {
            h ^= (uint64_t)v->as.obj.count;
            h = mix_u64(h);
            for (uint32_t i = 0; i < v->as.obj.count; ++i) {
                const JsonMember* m = &v->as.obj.members[i];
                h ^= json_digest_str(m->key, 0xCBF29CE484222325ULL);
                h = mix_u64(h);
                h ^= json_digest_u64(m->value);
                h = mix_u64(h);
            }
            return h;
        }
        default:
            return mix_u64(h ^ 0xDEADULL);
    }
}

// -----------------------------------------------------------------------------
// Optional convenience wrapper for benchmarks
// -----------------------------------------------------------------------------

// Parse JSON and return a digest. If parsing fails, returns 0 and fills err.
static uint64_t json_parse_and_digest(const char* src, size_t len, JsonArena* arena, JsonError* err)
{
    if (!arena) return 0;
    json_arena_reset(arena);
    JsonDoc doc = json_parse(src, len, arena, err);
    if (!doc.root || (err && err->message)) return 0;
    return json_digest_u64(doc.root);
}

// -----------------------------------------------------------------------------
// Self-test hooks (can be enabled in debug builds)
// -----------------------------------------------------------------------------

#if !defined(NDEBUG)
static void json_smoke_tests(void)
{
    JsonArena a;
    json_arena_init(&a, 4096);

    const char* s1 = "{\"a\":1,\"b\":[true,false,null,\"x\"],\"u\":\"\\u20AC\"}";
    JsonError e = {0};
    uint64_t h1 = json_parse_and_digest(s1, strlen(s1), &a, &e);
    assert(e.message == NULL);
    assert(h1 != 0);

    const char* s2 = "[1,2,3,4,5,6,7,8,9,10]";
    e = (JsonError){0};
    uint64_t h2 = json_parse_and_digest(s2, strlen(s2), &a, &e);
    assert(e.message == NULL);
    assert(h2 != 0);

    const char* s3 = "{\"bad\": [1,2,}";
    e = (JsonError){0};
    (void)json_parse_and_digest(s3, strlen(s3), &a, &e);
    assert(e.message != NULL);

    json_arena_free(&a);
}
#endif

// You may call this from your bench harness once at startup in debug builds.
static void json_parser_debug_init(void)
{
#if !defined(NDEBUG)
    static int once = 0;
    if (!once) {
        once = 1;
        json_smoke_tests();
    }
#endif
}
