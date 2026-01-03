// SPDX-License-Identifier: MIT
// c_name_mangle.c
//
// Name mangling for the C backend.
//
// Goals:
//  - Produce stable, deterministic C identifiers.
//  - Avoid collisions with C keywords and runtime/internal symbols.
//  - Encode module paths, namespaces, and symbol kinds.
//  - Provide reversible-ish encoding for debugging (best-effort).
//
// Typical usage in the C backend:
//  - Mangle function names, global variables, types, enum variants.
//  - Optionally emit short names for local temporaries.
//
// Pairing:
//  - c_name_mangle.h should declare the functions implemented here.
//  - It may define types like steel_str_view or a builder interface.
//  - This implementation uses only plain C strings and a small growable buffer.
//
// If your header differs, keep these implementations and add wrappers in the header.

#include "c_name_mangle.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifndef STEEL_MANGLE_PREFIX
  // Prefix for all backend-emitted global identifiers.
  #define STEEL_MANGLE_PREFIX "vitte_"
#endif

#ifndef STEEL_MANGLE_TMP_PREFIX
  #define STEEL_MANGLE_TMP_PREFIX "t_"
#endif

#ifndef STEEL_MANGLE_MAX_IDENT
  #define STEEL_MANGLE_MAX_IDENT 1024
#endif

//------------------------------------------------------------------------------
// Small growable string builder
//------------------------------------------------------------------------------

typedef struct steel_sb
{
    char*  data;
    size_t len;
    size_t cap;
} steel_sb;

static void steel_sb_init(steel_sb* b)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void steel_sb_free(steel_sb* b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static bool steel_sb_reserve(steel_sb* b, size_t extra)
{
    size_t need = b->len + extra + 1;
    if (need <= b->cap) return true;

    size_t new_cap = (b->cap == 0) ? 64 : b->cap;
    while (new_cap < need)
    {
        size_t next = new_cap * 2;
        if (next < new_cap) return false;
        new_cap = next;
    }

    char* p = (char*)realloc(b->data, new_cap);
    if (!p) return false;
    b->data = p;
    b->cap = new_cap;
    return true;
}

static bool steel_sb_push_ch(steel_sb* b, char c)
{
    if (!steel_sb_reserve(b, 1)) return false;
    b->data[b->len++] = c;
    b->data[b->len] = 0;
    return true;
}

static bool steel_sb_push_str(steel_sb* b, const char* s)
{
    if (!s) return true;
    size_t n = strlen(s);
    if (!steel_sb_reserve(b, n)) return false;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = 0;
    return true;
}

static bool steel_sb_push_u32_hex(steel_sb* b, uint32_t v)
{
    // fixed 8 hex digits for stability
    static const char* h = "0123456789abcdef";
    if (!steel_sb_reserve(b, 8)) return false;
    for (int i = 7; i >= 0; i--)
    {
        uint8_t nyb = (uint8_t)((v >> (i * 4)) & 0xF);
        b->data[b->len++] = h[nyb];
    }
    b->data[b->len] = 0;
    return true;
}

static char* steel_sb_take_cstr(steel_sb* b)
{
    if (!b->data)
    {
        char* z = (char*)malloc(1);
        if (z) z[0] = 0;
        return z;
    }
    char* p = b->data;
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    return p;
}

//------------------------------------------------------------------------------
// Hash (FNV-1a 32) for suffixing when needed
//------------------------------------------------------------------------------

static uint32_t steel_fnv1a32(const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; i++)
    {
        h ^= (uint32_t)p[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t steel_hash_str32(const char* s)
{
    if (!s) return 0;
    return steel_fnv1a32(s, strlen(s));
}

//------------------------------------------------------------------------------
// C keyword set
//------------------------------------------------------------------------------

static bool steel_is_c_keyword(const char* s)
{
    // C11 keywords + common extensions used by compilers.
    // Keep this list stable.
    static const char* kw[] = {
        "auto","break","case","char","const","continue","default","do","double","else",
        "enum","extern","float","for","goto","if","inline","int","long","register",
        "restrict","return","short","signed","sizeof","static","struct","switch","typedef",
        "union","unsigned","void","volatile","while",
        "_Alignas","_Alignof","_Atomic","_Bool","_Complex","_Generic","_Imaginary",
        "_Noreturn","_Static_assert","_Thread_local",
        // Common compiler builtins that are unsafe to generate as globals.
        "__attribute__","__declspec","__pragma","__asm","__volatile__","__inline__","__restrict__",
    };

    for (size_t i = 0; i < sizeof(kw)/sizeof(kw[0]); i++)
        if (strcmp(s, kw[i]) == 0) return true;
    return false;
}

//------------------------------------------------------------------------------
// Identifier encoding
//------------------------------------------------------------------------------

static inline bool steel_is_ident_start(unsigned char c)
{
    return (c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool steel_is_ident_cont(unsigned char c)
{
    return steel_is_ident_start(c) || (c >= '0' && c <= '9');
}

static bool steel_mangle_seg(steel_sb* b, const char* seg)
{
    // Encode one logical segment into valid C identifier chars.
    // Strategy:
    //  - Keep ASCII alnum and '_' as-is.
    //  - Replace other bytes with _xHH (hex)
    //  - Ensure first character of entire identifier is [A-Za-z_]

    if (!seg || !seg[0])
    {
        return steel_sb_push_str(b, "_empty");
    }

    const unsigned char* p = (const unsigned char*)seg;
    for (; *p; p++)
    {
        unsigned char c = *p;
        if (steel_is_ident_cont(c))
        {
            if (!steel_sb_push_ch(b, (char)c)) return false;
        }
        else
        {
            static const char* hx = "0123456789abcdef";
            if (!steel_sb_push_str(b, "_x")) return false;
            if (!steel_sb_push_ch(b, hx[(c >> 4) & 0xF])) return false;
            if (!steel_sb_push_ch(b, hx[c & 0xF])) return false;
        }
    }

    return true;
}

// Since C has no lambdas, implement path splitting without nested functions.
static bool steel_mangle_path_impl(steel_sb* b, const char* module_path)
{
    if (!module_path || !module_path[0])
        return steel_sb_push_str(b, "root");

    const char* p = module_path;
    if (p[0] && p[1] == ':') p += 2; // windows drive

    char seg[256];
    size_t seg_len = 0;

    for (;;)
    {
        char c = *p;
        bool at_end = (c == 0);

        bool is_sep = false;
        // separators: '.', '/', '\\'
        if (c == '.' || c == '/' || c == '\\') is_sep = true;
        // namespace separator: '::'
        if (c == ':' && p[1] == ':') is_sep = true;

        if (at_end || is_sep)
        {
            if (seg_len > 0)
            {
                seg[seg_len] = 0;
                if (b->len != 0)
                {
                    if (!steel_sb_push_str(b, "__")) return false;
                }
                if (!steel_mangle_seg(b, seg)) return false;
                seg_len = 0;
            }

            if (at_end) break;

            // consume separator
            if (c == ':' && p[1] == ':') p += 2;
            else p += 1;
            continue;
        }

        // accumulate segment bytes (truncate per segment but keep stable by hashing later)
        if (seg_len + 1 < sizeof(seg))
        {
            seg[seg_len++] = c;
        }
        else
        {
            // Segment too long: skip remaining bytes but keep advancing.
            // We'll add a hash suffix later for the entire identifier.
        }

        p++;
    }

    if (b->len == 0)
        return steel_sb_push_str(b, "root");

    return true;
}

static bool steel_ensure_ident_start(steel_sb* b)
{
    if (!b) return false;
    if (b->len == 0) return steel_sb_push_ch(b, '_');

    unsigned char c = (unsigned char)b->data[0];
    if (steel_is_ident_start(c)) return true;

    // Prefix underscore.
    if (!steel_sb_reserve(b, 1)) return false;
    memmove(b->data + 1, b->data, b->len + 1);
    b->data[0] = '_';
    b->len++;
    return true;
}

static bool steel_maybe_keyword_escape(steel_sb* b)
{
    if (!b) return false;
    if (b->len == 0) return true;

    // If exactly a keyword, prefix underscore.
    if (steel_is_c_keyword(b->data))
    {
        if (!steel_sb_reserve(b, 1)) return false;
        memmove(b->data + 1, b->data, b->len + 1);
        b->data[0] = '_';
        b->len++;
    }

    return true;
}

static bool steel_maybe_shorten_and_hash(steel_sb* b)
{
    if (!b) return false;
    if (b->len <= STEEL_MANGLE_MAX_IDENT) return true;

    // Keep prefix slice + add hash suffix.
    uint32_t h = steel_fnv1a32(b->data, b->len);

    // Truncate but keep room for "__h" + 8hex
    const char* suffix = "__h";
    size_t need_suffix = 3 + 8; // "__h" + 8

    size_t keep = STEEL_MANGLE_MAX_IDENT;
    if (keep > need_suffix) keep -= need_suffix;

    b->data[keep] = 0;
    b->len = keep;

    if (!steel_sb_push_str(b, suffix)) return false;
    if (!steel_sb_push_u32_hex(b, h)) return false;

    return true;
}

//------------------------------------------------------------------------------
// Public entry points
//------------------------------------------------------------------------------

// Allocate a new mangled name string; caller frees with free().
char* steel_c_mangle_global(const char* module_path, const char* kind_tag, const char* name)
{
    steel_sb b;
    steel_sb_init(&b);

    // Prefix all symbols.
    if (!steel_sb_push_str(&b, STEEL_MANGLE_PREFIX)) goto fail;

    // Kind tag (fn, g, ty, en, v, ...)
    if (kind_tag && kind_tag[0])
    {
        if (!steel_sb_push_str(&b, kind_tag)) goto fail;
        if (!steel_sb_push_str(&b, "__")) goto fail;
    }

    // Module path segments
    if (!steel_mangle_path_impl(&b, module_path)) goto fail;

    // Separator to symbol name
    if (!steel_sb_push_str(&b, "__")) goto fail;

    // Symbol name
    if (!steel_mangle_seg(&b, name ? name : "")) goto fail;

    // Ensure valid start
    if (!steel_ensure_ident_start(&b)) goto fail;

    // Escape keywords
    if (!steel_maybe_keyword_escape(&b)) goto fail;

    // Bound length with hash suffix
    if (!steel_maybe_shorten_and_hash(&b)) goto fail;

    return steel_sb_take_cstr(&b);

fail:
    steel_sb_free(&b);
    return NULL;
}

// Mangle a type name with an optional signature suffix.
// signature may be NULL; if provided, it is hashed and appended.
char* steel_c_mangle_type(const char* module_path, const char* type_name, const char* signature)
{
    steel_sb b;
    steel_sb_init(&b);

    if (!steel_sb_push_str(&b, STEEL_MANGLE_PREFIX)) goto fail;
    if (!steel_sb_push_str(&b, "ty__")) goto fail;

    if (!steel_mangle_path_impl(&b, module_path)) goto fail;
    if (!steel_sb_push_str(&b, "__")) goto fail;
    if (!steel_mangle_seg(&b, type_name ? type_name : "")) goto fail;

    if (signature && signature[0])
    {
        uint32_t h = steel_hash_str32(signature);
        if (!steel_sb_push_str(&b, "__s")) goto fail;
        if (!steel_sb_push_u32_hex(&b, h)) goto fail;
    }

    if (!steel_ensure_ident_start(&b)) goto fail;
    if (!steel_maybe_keyword_escape(&b)) goto fail;
    if (!steel_maybe_shorten_and_hash(&b)) goto fail;

    return steel_sb_take_cstr(&b);

fail:
    steel_sb_free(&b);
    return NULL;
}

// Mangle a function name; if signature provided, include a hash.
char* steel_c_mangle_fn(const char* module_path, const char* fn_name, const char* signature)
{
    steel_sb b;
    steel_sb_init(&b);

    if (!steel_sb_push_str(&b, STEEL_MANGLE_PREFIX)) goto fail;
    if (!steel_sb_push_str(&b, "fn__")) goto fail;

    if (!steel_mangle_path_impl(&b, module_path)) goto fail;
    if (!steel_sb_push_str(&b, "__")) goto fail;
    if (!steel_mangle_seg(&b, fn_name ? fn_name : "")) goto fail;

    if (signature && signature[0])
    {
        uint32_t h = steel_hash_str32(signature);
        if (!steel_sb_push_str(&b, "__s")) goto fail;
        if (!steel_sb_push_u32_hex(&b, h)) goto fail;
    }

    if (!steel_ensure_ident_start(&b)) goto fail;
    if (!steel_maybe_keyword_escape(&b)) goto fail;
    if (!steel_maybe_shorten_and_hash(&b)) goto fail;

    return steel_sb_take_cstr(&b);

fail:
    steel_sb_free(&b);
    return NULL;
}

// Mangle a global variable.
char* steel_c_mangle_global_var(const char* module_path, const char* name)
{
    return steel_c_mangle_global(module_path, "g", name);
}

// Mangle an enum variant: module::Enum::Variant
char* steel_c_mangle_enum_variant(const char* module_path, const char* enum_name, const char* variant_name)
{
    steel_sb b;
    steel_sb_init(&b);

    if (!steel_sb_push_str(&b, STEEL_MANGLE_PREFIX)) goto fail;
    if (!steel_sb_push_str(&b, "ev__")) goto fail;

    if (!steel_mangle_path_impl(&b, module_path)) goto fail;
    if (!steel_sb_push_str(&b, "__")) goto fail;

    if (!steel_mangle_seg(&b, enum_name ? enum_name : "")) goto fail;
    if (!steel_sb_push_str(&b, "__")) goto fail;
    if (!steel_mangle_seg(&b, variant_name ? variant_name : "")) goto fail;

    if (!steel_ensure_ident_start(&b)) goto fail;
    if (!steel_maybe_keyword_escape(&b)) goto fail;
    if (!steel_maybe_shorten_and_hash(&b)) goto fail;

    return steel_sb_take_cstr(&b);

fail:
    steel_sb_free(&b);
    return NULL;
}

// Temporary/local symbol names (not global). Deterministic given an index.
// Example: t_0000002a
char* steel_c_mangle_tmp(uint32_t idx)
{
    steel_sb b;
    steel_sb_init(&b);

    if (!steel_sb_push_str(&b, STEEL_MANGLE_TMP_PREFIX)) goto fail;
    if (!steel_sb_push_u32_hex(&b, idx)) goto fail;

    return steel_sb_take_cstr(&b);

fail:
    steel_sb_free(&b);
    return NULL;
}

// Optional: demangle best-effort for debug.
// Writes a readable string into out; returns true if it looks like ours.
bool steel_c_demangle(const char* mangled, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return false;
    out[0] = 0;
    if (!mangled) return false;

    if (strncmp(mangled, STEEL_MANGLE_PREFIX, strlen(STEEL_MANGLE_PREFIX)) != 0)
        return false;

    // Replace "__" with "::" and decode _xHH into bytes (printable only).
    const char* p = mangled + strlen(STEEL_MANGLE_PREFIX);
    size_t w = 0;

    while (*p && w + 1 < out_cap)
    {
        if (p[0] == '_' && p[1] == '_' )
        {
            if (w + 2 >= out_cap) break;
            out[w++] = ':';
            out[w++] = ':';
            p += 2;
            continue;
        }

        // No lambdas in C: decode manually
        if (p[0] == '_' && p[1] == 'x' && isxdigit((unsigned char)p[2]) && isxdigit((unsigned char)p[3]))
        {
            unsigned char c2 = (unsigned char)p[2];
            unsigned char c3 = (unsigned char)p[3];
            int v2 = (c2 <= '9') ? (c2 - '0') : (tolower(c2) - 'a' + 10);
            int v3 = (c3 <= '9') ? (c3 - '0') : (tolower(c3) - 'a' + 10);
            int byte = (v2 << 4) | v3;
            char ch = (byte >= 0x20 && byte <= 0x7E) ? (char)byte : '?';
            out[w++] = ch;
            p += 4;
            continue;
        }

        out[w++] = *p++;
    }

    out[w] = 0;
    return true;
}
