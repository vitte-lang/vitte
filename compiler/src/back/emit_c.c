#include "vittec/back/emit_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* -------------------------------------------------------------------------
 * Small utilities
 * ------------------------------------------------------------------------- */

static int vittec__ends_with(const char* s, const char* suf) {
  if(!s || !suf) return 0;
  size_t n = strlen(s);
  size_t m = strlen(suf);
  if(m > n) return 0;
  return 0 == memcmp(s + (n - m), suf, m);
}

static char* vittec__strdup(const char* s) {
  if(!s) return NULL;
  size_t n = strlen(s);
  char* p = (char*)malloc(n + 1);
  if(!p) return NULL;
  memcpy(p, s, n);
  p[n] = 0;
  return p;
}

static char* vittec__path_without_ext(const char* path) {
  /* Remove last ".c" or ".h" if present; otherwise duplicate. */
  if(!path) return NULL;
  size_t n = strlen(path);
  if(n >= 2 && vittec__ends_with(path, ".c")) {
    char* p = (char*)malloc(n - 1);
    if(!p) return NULL;
    memcpy(p, path, n - 2);
    p[n - 2] = 0;
    return p;
  }
  if(n >= 2 && vittec__ends_with(path, ".h")) {
    char* p = (char*)malloc(n - 1);
    if(!p) return NULL;
    memcpy(p, path, n - 2);
    p[n - 2] = 0;
    return p;
  }
  return vittec__strdup(path);
}

static char* vittec__path_with_ext(const char* base, const char* ext) {
  if(!base || !ext) return NULL;
  size_t n = strlen(base);
  size_t m = strlen(ext);
  char* p = (char*)malloc(n + m + 1);
  if(!p) return NULL;
  memcpy(p, base, n);
  memcpy(p + n, ext, m);
  p[n + m] = 0;
  return p;
}

static uint64_t vittec__fnv1a64(const void* data, size_t n) {
  const unsigned char* p = (const unsigned char*)data;
  uint64_t h = 1469598103934665603ull;
  for(size_t i = 0; i < n; i++) {
    h ^= (uint64_t)p[i];
    h *= 1099511628211ull;
  }
  return h;
}

static void vittec__hex8(uint32_t v, char out[9]) {
  static const char* H = "0123456789abcdef";
  for(int i = 7; i >= 0; i--) {
    out[i] = H[v & 0xF];
    v >>= 4;
  }
  out[8] = 0;
}

static void vittec__sanitize_ident(const char* in, char* out, size_t cap) {
  if(!out || cap == 0) return;
  if(!in) {
    out[0] = '_';
    out[1] = 0;
    return;
  }

  size_t j = 0;
  /* Ensure first char is valid identifier start. */
  unsigned char c0 = (unsigned char)in[0];
  if(!(isalpha(c0) || c0 == '_')) {
    out[j++] = '_';
  }

  for(size_t i = 0; in[i] && j + 1 < cap; i++) {
    unsigned char c = (unsigned char)in[i];
    if(isalnum(c) || c == '_') {
      out[j++] = (char)c;
    } else {
      out[j++] = '_';
    }
  }
  out[j] = 0;
}

static void vittec__make_header_guard(
  char out[512],
  const vittec_emit_c_options_t* opt,
  const char* base_hint
) {
  if(!out) return;
  const char* pre = (opt && opt->header_guard_prefix) ? opt->header_guard_prefix : "VITTE_";

  char base_leaf[256];
  if(base_hint && base_hint[0]) {
    vittec__sanitize_ident(base_hint, base_leaf, sizeof(base_leaf));
  } else {
    vittec__sanitize_ident("generated", base_leaf, sizeof(base_leaf));
  }

  for(size_t i = 0; base_leaf[i]; i++) {
    base_leaf[i] = (char)toupper((unsigned char)base_leaf[i]);
  }

  snprintf(out, 512, "%s%s_H", pre, base_leaf[0] ? base_leaf : "GENERATED");
}

static void vittec__sv_to_cstr(vittec_sv_t sv, char* out, size_t cap) {
  if(!out || cap == 0) return;
  size_t n = sv.len;
  if(n >= cap) n = cap - 1;
  if(sv.data && n) memcpy(out, sv.data, n);
  out[n] = 0;
}

static vittec_sv_t vittec__sv_from_cstr(const char* s) {
  if(!s) return vittec_sv("", 0);
  return vittec_sv(s, (uint64_t)strlen(s));
}

static void vittec__qualified_ident(
  char* out,
  size_t cap,
  const char* prefix,
  const char* module_name,
  const char* name,
  const char* fallback
) {
  if(!out || cap == 0) return;
  const char* mod = module_name ? module_name : "";
  const char* base = (name && name[0]) ? name : fallback;

  char smod[256];
  char sbase[256];
  vittec__sanitize_ident(mod, smod, sizeof(smod));
  vittec__sanitize_ident(base ? base : "", sbase, sizeof(sbase));

  const char* pre = prefix ? prefix : "";
  if(smod[0] && sbase[0]) {
    snprintf(out, cap, "%s%s_%s", pre, smod, sbase);
  } else if(sbase[0]) {
    snprintf(out, cap, "%s%s", pre, sbase);
  } else if(smod[0]) {
    snprintf(out, cap, "%s%s_module", pre, smod);
  } else {
    snprintf(out, cap, "%stype", pre);
  }
}

/* -------------------------------------------------------------------------
 * Simple dynamic buffer (for callback sink)
 * ------------------------------------------------------------------------- */

typedef struct vittec__buf {
  char* data;
  size_t len;
  size_t cap;
} vittec__buf_t;

static void vittec__buf_init(vittec__buf_t* b) {
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}

static void vittec__buf_free(vittec__buf_t* b) {
  if(b && b->data) free(b->data);
  if(b) { b->data = NULL; b->len = 0; b->cap = 0; }
}

static int vittec__buf_reserve(vittec__buf_t* b, size_t add) {
  if(!b) return 0;
  size_t need = b->len + add;
  if(need <= b->cap) return 1;
  size_t newcap = (b->cap == 0) ? 4096 : b->cap;
  while(newcap < need) newcap *= 2;
  char* p = (char*)realloc(b->data, newcap);
  if(!p) return 0;
  b->data = p;
  b->cap = newcap;
  return 1;
}

static int vittec__buf_write(vittec__buf_t* b, const char* s, size_t n) {
  if(!b || (!s && n)) return 0;
  if(!vittec__buf_reserve(b, n)) return 0;
  if(n) memcpy(b->data + b->len, s, n);
  b->len += n;
  return 1;
}

static int vittec__buf_printf(vittec__buf_t* b, const char* fmt, ...) {
  if(!b || !fmt) return 0;
  va_list ap;
  va_start(ap, fmt);
  char tmp[1024];
  int k = vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  if(k < 0) return 0;
  if((size_t)k < sizeof(tmp)) return vittec__buf_write(b, tmp, (size_t)k);

  /* slow path */
  size_t need = (size_t)k + 1;
  char* big = (char*)malloc(need);
  if(!big) return 0;
  va_start(ap, fmt);
  vsnprintf(big, need, fmt, ap);
  va_end(ap);
  int ok = vittec__buf_write(b, big, (size_t)k);
  free(big);
  return ok;
}

/* -------------------------------------------------------------------------
 * Vitte core AST -> C emission helpers (best-effort, no sema)
 * ------------------------------------------------------------------------- */

static int vittec__is_c_ident_char(unsigned char c, int first) {
  if(first) return (c == '_') || isalpha((int)c);
  return (c == '_') || isalnum((int)c);
}

static int vittec__is_simple_c_ident(const char* s) {
  if(!s || !s[0]) return 0;
  if(!vittec__is_c_ident_char((unsigned char)s[0], 1)) return 0;
  for(size_t i = 1; s[i]; i++) {
    if(!vittec__is_c_ident_char((unsigned char)s[i], 0)) return 0;
  }
  return 1;
}

static void vittec__emit_indent(vittec__buf_t* b, int indent) {
  if(!b || indent <= 0) return;
  for(int i = 0; i < indent; i++) vittec__buf_write(b, "  ", 2);
}

static void vittec__emit_sanitized_ident(vittec__buf_t* b, const char* in) {
  char tmp[512];
  vittec__sanitize_ident(in ? in : "", tmp, sizeof(tmp));
  vittec__buf_printf(b, "%s", tmp[0] ? tmp : "_");
}

static void vittec__emit_numeric_literal(vittec__buf_t* b, const char* s) {
  /* Vitte allows '_' separators in numbers; C17 does not. Strip them. */
  if(!b) return;
  if(!s) { vittec__buf_write(b, "0", 1); return; }
  for(size_t i = 0; s[i]; i++) {
    if(s[i] == '_') continue;
    vittec__buf_write(b, &s[i], 1);
  }
}

static void vittec__emit_c_string_literal(vittec__buf_t* b, const char* s) {
  if(!b) return;
  vittec__buf_write(b, "\"", 1);
  if(s) {
    for(const unsigned char* p = (const unsigned char*)s; *p; p++) {
      unsigned char c = *p;
      switch(c) {
        case '\\': vittec__buf_write(b, "\\\\", 2); break;
        case '\"': vittec__buf_write(b, "\\\"", 2); break;
        case '\n': vittec__buf_write(b, "\\n", 2); break;
        case '\r': vittec__buf_write(b, "\\r", 2); break;
        case '\t': vittec__buf_write(b, "\\t", 2); break;
        default:
          if(c < 0x20) {
            char esc[8];
            snprintf(esc, sizeof(esc), "\\x%02x", (unsigned)c);
            vittec__buf_write(b, esc, strlen(esc));
          } else {
            vittec__buf_write(b, (const char*)&c, 1);
          }
          break;
      }
    }
  }
  vittec__buf_write(b, "\"", 1);
}

static const char* vittec__builtin_c_type(const char* ty) {
  if(!ty || !ty[0]) return NULL;
  if(!strcmp(ty, "void")) return "void";
  if(!strcmp(ty, "bool")) return "bool";
  if(!strcmp(ty, "i8")) return "int8_t";
  if(!strcmp(ty, "i16")) return "int16_t";
  if(!strcmp(ty, "i32")) return "int32_t";
  if(!strcmp(ty, "i64")) return "int64_t";
  if(!strcmp(ty, "u8")) return "uint8_t";
  if(!strcmp(ty, "u16")) return "uint16_t";
  if(!strcmp(ty, "u32")) return "uint32_t";
  if(!strcmp(ty, "u64")) return "uint64_t";
  if(!strcmp(ty, "usize")) return "size_t";
  if(!strcmp(ty, "isize")) return "ptrdiff_t";
  if(!strcmp(ty, "f32")) return "float";
  if(!strcmp(ty, "f64")) return "double";
  if(!strcmp(ty, "str") || !strcmp(ty, "string")) return "const char*";
  return NULL;
}

static const vitte_codegen_type* vittec__find_type(
  const vitte_codegen_unit* unit,
  const char* module_hint,
  const char* name
) {
  if(!unit || !name || !name[0]) return NULL;
  for(size_t i = 0; i < unit->type_count; i++) {
    const vitte_codegen_type* ty = &unit->types[i];
    if(!ty->name || strcmp(ty->name, name) != 0) continue;
    if(module_hint && module_hint[0]) {
      const char* m = vitte_codegen_type_module_name(ty);
      if(m && m[0] && strcmp(m, module_hint) == 0) return ty;
    } else {
      return ty;
    }
  }
  for(size_t i = 0; i < unit->type_count; i++) {
    const vitte_codegen_type* ty = &unit->types[i];
    if(ty->name && strcmp(ty->name, name) == 0) return ty;
  }
  return NULL;
}

static void vittec__emit_type_ref(
  vittec__buf_t* b,
  const vittec_emit_c_options_t* opt,
  const vitte_codegen_unit* unit,
  const char* module_hint,
  const char* ty_name
) {
  const char* builtin = vittec__builtin_c_type(ty_name);
  if(builtin) {
    vittec__buf_printf(b, "%s", builtin);
    return;
  }

  const vitte_codegen_type* ty = vittec__find_type(unit, module_hint, ty_name);
  if(ty) {
    const char* prefix = (opt && opt->namespace_prefix) ? opt->namespace_prefix : "vitte_";
    char ident[512];
    vittec__qualified_ident(ident, sizeof(ident), prefix, vitte_codegen_type_module_name(ty), ty->name, "type");
    vittec__buf_printf(b, "%s", ident);
    return;
  }

  vittec__buf_printf(b, "void*");
}

static const vitte_ast* vittec__find_return_type_node(const vitte_ast* fn_ast) {
  for(const vitte_ast* ch = fn_ast ? fn_ast->first_child : NULL; ch; ch = ch->next) {
    if(ch->kind == VITTE_AST_TYPE_REF && ch->aux_text && strcmp(ch->aux_text, "return") == 0) return ch;
  }
  return NULL;
}

static const vitte_ast* vittec__find_block_child(const vitte_ast* node) {
  for(const vitte_ast* ch = node ? node->first_child : NULL; ch; ch = ch->next) {
    if(ch->kind == VITTE_AST_BLOCK) return ch;
  }
  return NULL;
}

static int vittec__expr_prec(const vitte_ast* e) {
  if(!e) return 0;
  switch(e->kind) {
    case VITTE_AST_EXPR_LITERAL:
    case VITTE_AST_EXPR_IDENT:
    case VITTE_AST_EXPR_PATH:
    case VITTE_AST_EXPR_CALL:
      return 100;
    case VITTE_AST_EXPR_UNARY:
      return 90;
    case VITTE_AST_EXPR_BINARY:
      break;
    default:
      return 0;
  }

  switch(e->binary_op) {
    case VITTE_BINOP_OR: return 10;
    case VITTE_BINOP_AND: return 20;
    case VITTE_BINOP_EQ:
    case VITTE_BINOP_NEQ: return 30;
    case VITTE_BINOP_LT:
    case VITTE_BINOP_LTE:
    case VITTE_BINOP_GT:
    case VITTE_BINOP_GTE: return 40;
    case VITTE_BINOP_ADD:
    case VITTE_BINOP_SUB: return 50;
    case VITTE_BINOP_MUL:
    case VITTE_BINOP_DIV:
    case VITTE_BINOP_MOD: return 60;
    default: return 0;
  }
}

static const char* vittec__binop_c(vitte_binary_op op) {
  switch(op) {
    case VITTE_BINOP_ADD: return "+";
    case VITTE_BINOP_SUB: return "-";
    case VITTE_BINOP_MUL: return "*";
    case VITTE_BINOP_DIV: return "/";
    case VITTE_BINOP_MOD: return "%";
    case VITTE_BINOP_EQ: return "==";
    case VITTE_BINOP_NEQ: return "!=";
    case VITTE_BINOP_LT: return "<";
    case VITTE_BINOP_LTE: return "<=";
    case VITTE_BINOP_GT: return ">";
    case VITTE_BINOP_GTE: return ">=";
    case VITTE_BINOP_AND: return "&&";
    case VITTE_BINOP_OR: return "||";
    default: return "/*op*/";
  }
}

static const char* vittec__unop_c(vitte_unary_op op) {
  switch(op) {
    case VITTE_UNOP_NEG: return "-";
    case VITTE_UNOP_POS: return "+";
    case VITTE_UNOP_NOT: return "!";
    default: return "";
  }
}

typedef enum vittec__infer_ty {
  VITTEC__TY_UNKNOWN = 0,
  VITTEC__TY_I64,
  VITTEC__TY_U64,
  VITTEC__TY_F64,
  VITTEC__TY_BOOL,
  VITTEC__TY_CSTR
} vittec__infer_ty_t;

static vittec__infer_ty_t vittec__infer_expr_ty(const vitte_ast* e) {
  if(!e) return VITTEC__TY_UNKNOWN;
  switch(e->kind) {
    case VITTE_AST_EXPR_LITERAL:
      switch(e->literal.kind) {
        case VITTE_LITERAL_INT: return VITTEC__TY_I64;
        case VITTE_LITERAL_FLOAT: return VITTEC__TY_F64;
        case VITTE_LITERAL_STRING: return VITTEC__TY_CSTR;
        case VITTE_LITERAL_BOOL: return VITTEC__TY_BOOL;
        default: return VITTEC__TY_UNKNOWN;
      }
    case VITTE_AST_EXPR_UNARY:
      if(e->unary_op == VITTE_UNOP_NOT) return VITTEC__TY_BOOL;
      return vittec__infer_expr_ty(e->first_child);
    case VITTE_AST_EXPR_BINARY: {
      switch(e->binary_op) {
        case VITTE_BINOP_EQ:
        case VITTE_BINOP_NEQ:
        case VITTE_BINOP_LT:
        case VITTE_BINOP_LTE:
        case VITTE_BINOP_GT:
        case VITTE_BINOP_GTE:
        case VITTE_BINOP_AND:
        case VITTE_BINOP_OR:
          return VITTEC__TY_BOOL;
        default: {
          vittec__infer_ty_t lt = vittec__infer_expr_ty(e->first_child);
          vittec__infer_ty_t rt = vittec__infer_expr_ty(e->first_child ? e->first_child->next : NULL);
          if(lt == VITTEC__TY_F64 || rt == VITTEC__TY_F64) return VITTEC__TY_F64;
          if(lt == VITTEC__TY_CSTR || rt == VITTEC__TY_CSTR) return VITTEC__TY_CSTR;
          return VITTEC__TY_I64;
        }
      }
    }
    default:
      return VITTEC__TY_UNKNOWN;
  }
}

static void vittec__emit_expr(
  vittec__buf_t* b,
  const vitte_ast* e,
  int parent_prec
) {
  if(!b) return;
  if(!e) { vittec__buf_write(b, "0", 1); return; }

  int my_prec = vittec__expr_prec(e);
  int need_paren = (my_prec && my_prec < parent_prec);
  if(need_paren) vittec__buf_write(b, "(", 1);

  switch(e->kind) {
    case VITTE_AST_EXPR_IDENT:
      vittec__emit_sanitized_ident(b, e->text);
      break;
    case VITTE_AST_EXPR_PATH:
      vittec__emit_sanitized_ident(b, e->text);
      break;
    case VITTE_AST_EXPR_LITERAL:
      switch(e->literal.kind) {
        case VITTE_LITERAL_INT:
        case VITTE_LITERAL_FLOAT:
          vittec__emit_numeric_literal(b, e->text);
          break;
        case VITTE_LITERAL_STRING:
          vittec__emit_c_string_literal(b, e->text ? e->text : "");
          break;
        case VITTE_LITERAL_BOOL:
          vittec__buf_write(b, e->literal.bool_value ? "true" : "false", e->literal.bool_value ? 4 : 5);
          break;
        default:
          vittec__buf_write(b, "0", 1);
          break;
      }
      break;
    case VITTE_AST_EXPR_UNARY: {
      const char* op = vittec__unop_c(e->unary_op);
      vittec__buf_printf(b, "%s", op);
      vittec__emit_expr(b, e->first_child, 90);
      break;
    }
    case VITTE_AST_EXPR_BINARY: {
      const vitte_ast* lhs = e->first_child;
      const vitte_ast* rhs = lhs ? lhs->next : NULL;
      const char* op = vittec__binop_c(e->binary_op);
      int prec = vittec__expr_prec(e);
      vittec__emit_expr(b, lhs, prec);
      vittec__buf_printf(b, " %s ", op);
      vittec__emit_expr(b, rhs, prec + 1);
      break;
    }
    case VITTE_AST_EXPR_CALL: {
      vittec__emit_sanitized_ident(b, e->text);
      vittec__buf_write(b, "(", 1);
      size_t ai = 0;
      for(const vitte_ast* arg = e->first_child; arg; arg = arg->next) {
        if(ai++) vittec__buf_write(b, ", ", 2);
        vittec__emit_expr(b, arg, 0);
      }
      vittec__buf_write(b, ")", 1);
      break;
    }
    default:
      vittec__buf_write(b, "0", 1);
      break;
  }

  if(need_paren) vittec__buf_write(b, ")", 1);
}

typedef struct vittec__strset {
  char** names;
  size_t len;
  size_t cap;
} vittec__strset_t;

static void vittec__strset_free(vittec__strset_t* s) {
  if(!s) return;
  for(size_t i = 0; i < s->len; i++) free(s->names[i]);
  free(s->names);
  s->names = NULL;
  s->len = 0;
  s->cap = 0;
}

static int vittec__strset_has(const vittec__strset_t* s, const char* name) {
  if(!s || !name) return 0;
  for(size_t i = 0; i < s->len; i++) {
    if(s->names[i] && strcmp(s->names[i], name) == 0) return 1;
  }
  return 0;
}

static int vittec__strset_add(vittec__strset_t* s, const char* name) {
  if(!s || !name || !name[0]) return 0;
  if(vittec__strset_has(s, name)) return 1;
  if(s->len == s->cap) {
    size_t nc = s->cap ? s->cap * 2 : 16;
    char** nn = (char**)realloc(s->names, nc * sizeof(char*));
    if(!nn) return 0;
    s->names = nn;
    s->cap = nc;
  }
  s->names[s->len++] = vittec__strdup(name);
  return s->names[s->len - 1] != NULL;
}

static void vittec__emit_stmt(
  vittec__buf_t* b,
  const vitte_ast* s,
  int indent,
  vittec__strset_t* locals
);

static void vittec__emit_block(
  vittec__buf_t* b,
  const vitte_ast* block,
  int indent,
  vittec__strset_t* locals
) {
  vittec__buf_write(b, "{\n", 2);
  for(const vitte_ast* st = block ? block->first_child : NULL; st; st = st->next) {
    vittec__emit_stmt(b, st, indent + 1, locals);
  }
  vittec__emit_indent(b, indent);
  vittec__buf_write(b, "}\n", 2);
}

static void vittec__emit_stmt(
  vittec__buf_t* b,
  const vitte_ast* s,
  int indent,
  vittec__strset_t* locals
) {
  if(!b || !s) return;

  switch(s->kind) {
    case VITTE_AST_CORE_STMT_ASSIGN: {
      const vitte_ast* target = s->first_child;
      const vitte_ast* value = target ? target->next : NULL;

      char name[512];
      name[0] = 0;
      if(target && (target->kind == VITTE_AST_EXPR_IDENT || target->kind == VITTE_AST_EXPR_PATH)) {
        vittec__sanitize_ident(target->text ? target->text : "", name, sizeof(name));
      }

      vittec__emit_indent(b, indent);

      int emit_decl = 0;
      if(name[0] && vittec__is_simple_c_ident(name) && locals && !vittec__strset_has(locals, name)) {
        emit_decl = 1;
      }

      if(emit_decl) {
        vittec__infer_ty_t it = vittec__infer_expr_ty(value);
        const char* cty = "int64_t";
        if(it == VITTEC__TY_F64) cty = "double";
        else if(it == VITTEC__TY_BOOL) cty = "bool";
        else if(it == VITTEC__TY_CSTR) cty = "const char*";
        else if(it == VITTEC__TY_U64) cty = "uint64_t";
        vittec__buf_printf(b, "%s %s = ", cty, name);
        vittec__emit_expr(b, value, 0);
        vittec__buf_write(b, ";\n", 2);
        (void)vittec__strset_add(locals, name);
      } else {
        vittec__emit_expr(b, target, 0);
        vittec__buf_write(b, " = ", 3);
        vittec__emit_expr(b, value, 0);
        vittec__buf_write(b, ";\n", 2);
      }
      break;
    }
    case VITTE_AST_CORE_STMT_EXPR:
      vittec__emit_indent(b, indent);
      vittec__emit_expr(b, s->first_child, 0);
      vittec__buf_write(b, ";\n", 2);
      break;
    case VITTE_AST_CORE_STMT_RETURN:
      vittec__emit_indent(b, indent);
      if(s->first_child) {
        vittec__buf_write(b, "return ", 7);
        vittec__emit_expr(b, s->first_child, 0);
        vittec__buf_write(b, ";\n", 2);
      } else {
        vittec__buf_write(b, "return;\n", 8);
      }
      break;
    case VITTE_AST_CORE_STMT_IF: {
      const vitte_ast* br = s->first_child;
      int first = 1;
      while(br) {
        const vitte_ast* cond = br->first_child;
        const vitte_ast* blk = NULL;
        if(cond && cond->kind == VITTE_AST_BLOCK) {
          blk = cond;
          cond = NULL;
        } else {
          blk = cond ? cond->next : NULL;
        }
        vittec__emit_indent(b, indent);
        if(first) {
          if(cond) {
            vittec__buf_write(b, "if (", 4);
            vittec__emit_expr(b, cond, 0);
            vittec__buf_write(b, ") ", 2);
            vittec__emit_block(b, blk, indent, locals);
          } else {
            vittec__buf_write(b, "if (1) ", 7);
            vittec__emit_block(b, blk, indent, locals);
          }
          first = 0;
        } else {
          if(cond) {
            vittec__buf_write(b, "else if (", 9);
            vittec__emit_expr(b, cond, 0);
            vittec__buf_write(b, ") ", 2);
            vittec__emit_block(b, blk, indent, locals);
          } else {
            vittec__buf_write(b, "else ", 5);
            vittec__emit_block(b, blk, indent, locals);
            break;
          }
        }
        br = br->next;
      }
      break;
    }
    case VITTE_AST_CORE_STMT_FOR: {
      const vitte_ast* start = s->first_child;
      const vitte_ast* end = start ? start->next : NULL;
      const vitte_ast* maybe_step = end ? end->next : NULL;
      const vitte_ast* body = maybe_step;
      const vitte_ast* step = NULL;
      if(body && body->kind != VITTE_AST_BLOCK) {
        step = body;
        body = step->next;
      }
      if(!body || body->kind != VITTE_AST_BLOCK) break;

      char ivar[256];
      vittec__sanitize_ident((s->text && s->text[0]) ? s->text : "i", ivar, sizeof(ivar));

      vittec__emit_indent(b, indent);
      vittec__buf_printf(b, "for (int64_t %s = (int64_t)(", ivar);
      vittec__emit_expr(b, start, 0);
      vittec__buf_write(b, "); ", 3);
      vittec__buf_printf(b, "%s < (int64_t)(", ivar);
      vittec__emit_expr(b, end, 0);
      vittec__buf_write(b, "); ", 3);
      vittec__buf_printf(b, "%s += (int64_t)(", ivar);
      if(step) vittec__emit_expr(b, step, 0);
      else vittec__buf_write(b, "1", 1);
      vittec__buf_write(b, ")) ", 3);
      vittec__emit_block(b, body, indent, locals);
      break;
    }
    default:
      vittec__emit_indent(b, indent);
      vittec__buf_write(b, "/* TODO: unsupported stmt */\n", 29);
      break;
  }
}

/* -------------------------------------------------------------------------
 * Options
 * ------------------------------------------------------------------------- */

void vittec_emit_c_options_init(vittec_emit_c_options_t* opt) {
  if(!opt) return;
  memset(opt, 0, sizeof(*opt));
  opt->size = (uint32_t)sizeof(*opt);
  opt->mode = VITTEC_EMIT_C_MODE_STUBS;
  opt->mangle = VITTEC_EMIT_C_MANGLE_SIMPLE;
  opt->runtime = VITTEC_EMIT_C_RUNTIME_NONE;
  opt->emit_header = 0;
  opt->emit_line_directives = 0;
  opt->emit_debug_comments = 0;
  opt->namespace_prefix = "vitte_";
  opt->header_guard_prefix = "VITTE_";
  opt->runtime_header = NULL;
  opt->write_file = NULL;
  opt->write_file_user = NULL;
}

static vittec_emit_c_options_t vittec__opt_or_default(const vittec_emit_c_options_t* in) {
  vittec_emit_c_options_t o;
  vittec_emit_c_options_init(&o);
  if(in) {
    /* Copy only known fields if caller uses a different size. */
    uint32_t n = in->size ? in->size : (uint32_t)sizeof(*in);
    if(n > (uint32_t)sizeof(o)) n = (uint32_t)sizeof(o);
    memcpy(&o, in, n);

    /* Ensure defaults for critical pointers if absent. */
    if(!o.namespace_prefix) o.namespace_prefix = "vitte_";
    if(!o.header_guard_prefix) o.header_guard_prefix = "VITTE_";
  }
  return o;
}

/* -------------------------------------------------------------------------
 * Deterministic name mangling
 * ------------------------------------------------------------------------- */

static void vittec__mangle(
  char* out,
  size_t cap,
  const vittec_emit_c_options_t* opt,
  vittec_sv_t module_name,
  vittec_sv_t fn_name
) {
  if(!out || cap == 0) return;
  out[0] = 0;

  char mod[256];
  char name[256];
  vittec__sv_to_cstr(module_name, mod, sizeof(mod));
  vittec__sv_to_cstr(fn_name, name, sizeof(name));

  char smod[256];
  char sname[256];
  vittec__sanitize_ident(mod, smod, sizeof(smod));
  vittec__sanitize_ident(name, sname, sizeof(sname));

  const char* pre = (opt && opt->namespace_prefix) ? opt->namespace_prefix : "vitte_";

  if(opt && opt->mangle == VITTEC_EMIT_C_MANGLE_HASHED) {
    /* hashed = <pre><mod>_<name>__<hash8> */
    char full[600];
    int k = snprintf(full, sizeof(full), "%s%s::%s", mod, (mod[0] ? "" : ""), name);
    if(k < 0) k = 0;
    uint64_t h = vittec__fnv1a64(full, (size_t)k);
    uint32_t h32 = (uint32_t)(h ^ (h >> 32));
    char hx[9];
    vittec__hex8(h32, hx);

    if(smod[0]) {
      snprintf(out, cap, "%s%s_%s__%s", pre, smod, sname, hx);
    } else {
      snprintf(out, cap, "%s%s__%s", pre, sname, hx);
    }
  } else {
    /* simple = <pre><mod>_<name> */
    if(smod[0]) {
      snprintf(out, cap, "%s%s_%s", pre, smod, sname);
    } else {
      snprintf(out, cap, "%s%s", pre, sname);
    }
  }
}

/* -------------------------------------------------------------------------
 * Emission helpers
 * ------------------------------------------------------------------------- */

static int vittec__write_file_or_path(
  const vittec_emit_c_options_t* opt,
  const char* path,
  const void* data,
  size_t size
) {
  if(!path || !data) return VITTEC_EMIT_C_EINVAL;

  if(opt && opt->write_file) {
    int rc = opt->write_file(opt->write_file_user, path, data, size);
    return (rc == 0) ? VITTEC_EMIT_C_OK : VITTEC_EMIT_C_EIO;
  }

  FILE* f = fopen(path, "wb");
  if(!f) return VITTEC_EMIT_C_EIO;
  if(size && fwrite(data, 1, size, f) != size) {
    fclose(f);
    return VITTEC_EMIT_C_EIO;
  }
  fclose(f);
  return VITTEC_EMIT_C_OK;
}

static void vittec__emit_preamble_buf(vittec__buf_t* b, const vittec_emit_c_options_t* opt) {
  vittec__buf_printf(b, "/* generated by vittec (C emitter) */\n");

  if(opt && opt->emit_debug_comments) {
    vittec__buf_printf(b, "/* mode=%s mangle=%s runtime=%s */\n",
      (opt->mode == VITTEC_EMIT_C_MODE_FULL) ? "full" : "stubs",
      (opt->mangle == VITTEC_EMIT_C_MANGLE_HASHED) ? "hashed" : "simple",
      (opt->runtime == VITTEC_EMIT_C_RUNTIME_VITTE) ? "vitte" : "none");
  }

  vittec__buf_printf(b, "#include <stdint.h>\n");
  vittec__buf_printf(b, "#include <stddef.h>\n");
  vittec__buf_printf(b, "#include <stdbool.h>\n");

  if(opt && opt->runtime == VITTEC_EMIT_C_RUNTIME_VITTE && opt->runtime_header && opt->runtime_header[0]) {
    vittec__buf_printf(b, "#include \"%s\"\n", opt->runtime_header);
  }

  /* Keep emitted C runnable without an external runtime (best-effort). */
  if(opt && opt->mode == VITTEC_EMIT_C_MODE_FULL && opt->runtime == VITTEC_EMIT_C_RUNTIME_NONE) {
    vittec__buf_printf(b, "#include <stdio.h>\n\n");
    vittec__buf_printf(b, "static void vitte__say_i64(long long v) { printf(\"%%lld\\\\n\", v); }\n");
    vittec__buf_printf(b, "static void vitte__say_u64(unsigned long long v) { printf(\"%%llu\\\\n\", v); }\n");
    vittec__buf_printf(b, "static void vitte__say_f64(double v) { printf(\"%%g\\\\n\", v); }\n");
    vittec__buf_printf(b, "static void vitte__say_cstr(const char* s) { printf(\"%%s\\\\n\", s ? s : \"\"); }\n");
    vittec__buf_printf(b, "#ifndef VITTE_NO_BUILTINS\n");
    vittec__buf_printf(b, "#define say(x) _Generic((x), \\\n");
    vittec__buf_printf(b, "  const char*: vitte__say_cstr, char*: vitte__say_cstr, \\\n");
    vittec__buf_printf(b, "  double: vitte__say_f64, float: vitte__say_f64, \\\n");
    vittec__buf_printf(b, "  unsigned long long: vitte__say_u64, unsigned long: vitte__say_u64, unsigned: vitte__say_u64, \\\n");
    vittec__buf_printf(b, "  long long: vitte__say_i64, long: vitte__say_i64, int: vitte__say_i64, \\\n");
    vittec__buf_printf(b, "  default: vitte__say_i64)(x)\n");
    vittec__buf_printf(b, "#endif\n");
  }

  vittec__buf_printf(b, "\n");
}

static void vittec__emit_header_buf(
  vittec__buf_t* b,
  const vittec_emit_c_options_t* opt,
  const char* guard,
  const vitte_codegen_unit* unit
) {
  vittec__buf_printf(b, "#ifndef %s\n#define %s\n\n", guard, guard);
  vittec__buf_printf(b, "#include <stdint.h>\n#include <stddef.h>\n#include <stdbool.h>\n\n");
  vittec__buf_printf(b, "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

  if(opt && opt->emit_debug_comments) {
    vittec__buf_printf(b, "/* generated header: bootstrap prototypes */\n\n");
  }

  if(unit && unit->functions && unit->function_count) {
    for(size_t i = 0; i < unit->function_count; i++) {
      const vitte_codegen_function* fn = &unit->functions[i];
      char cname[512];
      vittec_sv_t mod = vittec__sv_from_cstr(vitte_codegen_function_module_name(fn));
      vittec_sv_t name = vittec__sv_from_cstr(fn->name);
      vittec__mangle(cname, sizeof(cname), opt, mod, name);
      const vitte_ast* ret = vittec__find_return_type_node(fn->ast);
      const char* module_hint = vitte_codegen_function_module_name(fn);

      vittec__emit_type_ref(b, opt, unit, module_hint, ret ? ret->text : "int");
      vittec__buf_printf(b, " %s(", cname);

      size_t pc = 0;
      for(const vitte_ast* ch = fn->ast ? fn->ast->first_child : NULL; ch; ch = ch->next) {
        if(ch->kind != VITTE_AST_PARAM) continue;
        const vitte_ast* ty = ch->first_child;
        if(pc++) vittec__buf_write(b, ", ", 2);
        vittec__emit_type_ref(b, opt, unit, module_hint, ty ? ty->text : "int");
        vittec__buf_write(b, " ", 1);
        vittec__emit_sanitized_ident(b, ch->text);
      }
      if(pc == 0) vittec__buf_write(b, "void", 4);
      vittec__buf_printf(b, ");\n");
    }
  }

  vittec__buf_printf(b, "\n#ifdef __cplusplus\n} /* extern \"C\" */\n#endif\n\n");
  vittec__buf_printf(b, "#endif /* %s */\n", guard);
}

static void vittec__emit_modules_buf(vittec__buf_t* b, const vitte_codegen_unit* unit) {
  vittec__buf_printf(b, "/* modules */\n");
  if(!unit || unit->module_count == 0) {
    vittec__buf_printf(b, "/*   (none) */\n\n");
    return;
  }
  for(size_t i = 0; i < unit->module_count; i++) {
    const vitte_codegen_module* mod = &unit->modules[i];
    const char* name = mod->name ? mod->name : "(anonymous)";
    vittec__buf_printf(
      b,
      "/*   module %s (%u..%u) */\n",
      name,
      (unsigned)mod->span.lo,
      (unsigned)mod->span.hi
    );
  }
  vittec__buf_printf(b, "\n");
}

static void vittec__emit_types_buf(
  vittec__buf_t* b,
  const vittec_emit_c_options_t* opt,
  const vitte_codegen_unit* unit
) {
  vittec__buf_printf(b, "/* types */\n");
  if(!unit || unit->type_count == 0) {
    vittec__buf_printf(b, "/*   (none) */\n\n");
    return;
  }

  const char* prefix = (opt && opt->namespace_prefix) ? opt->namespace_prefix : "vitte_";
  for(size_t i = 0; i < unit->type_count; i++) {
    const vitte_codegen_type* ty = &unit->types[i];
    const char* module_name = vitte_codegen_type_module_name(ty);

    char ident[512];
    vittec__qualified_ident(ident, sizeof(ident), prefix, module_name, ty->name, "type");
    vittec__buf_printf(b, "typedef struct %s {\n", ident);
    if(ty->ast && ty->ast->first_child) {
      for(const vitte_ast* field = ty->ast->first_child; field; field = field->next) {
        if(field->kind != VITTE_AST_FIELD_DECL) continue;
        const vitte_ast* fty = field->first_child;
        vittec__buf_write(b, "  ", 2);
        vittec__emit_type_ref(b, opt, unit, module_name, fty ? fty->text : "int");
        vittec__buf_write(b, " ", 1);
        vittec__emit_sanitized_ident(b, field->text);
        vittec__buf_write(b, ";\n", 2);
      }
    } else {
      vittec__buf_printf(b, "  /* (no fields) */\n");
    }
    vittec__buf_printf(b, "} %s;\n\n", ident);
  }
}

static void vittec__emit_functions_buf(
  vittec__buf_t* b,
  const vittec_emit_c_options_t* opt,
  const vitte_codegen_unit* unit
) {
  if(!unit) {
    vittec__buf_printf(b, "/* (no codegen unit) */\n\n");
    return;
  }

  if(opt && opt->emit_debug_comments) {
    vittec__buf_printf(b, "/* functions: %zu */\n\n", unit->function_count);
  }

  if(unit->functions && unit->function_count) {
    for(size_t i = 0; i < unit->function_count; i++) {
      const vitte_codegen_function* fn = &unit->functions[i];

      char cname[512];
      vittec_sv_t mod = vittec__sv_from_cstr(vitte_codegen_function_module_name(fn));
      vittec_sv_t name = vittec__sv_from_cstr(fn->name);
      vittec__mangle(cname, sizeof(cname), opt, mod, name);

      const char* mod_label = (mod.len && mod.data && mod.data[0]) ? mod.data : "(root)";
      const char* fn_label = (fn->name && fn->name[0]) ? fn->name : "(anon)";

      vittec__buf_printf(
        b,
        "/* %s::%s params=%zu stmts=%zu */\n",
        mod_label,
        fn_label,
        fn->param_count,
        fn->stmt_count
      );

      if(opt && opt->emit_line_directives) {
        vittec__buf_printf(
          b,
          "/* span: %u..%u */\n",
          (unsigned)fn->span.lo,
          (unsigned)fn->span.hi
        );
      }

      if(opt && opt->emit_debug_comments) {
        vittec__buf_printf(
          b,
          "/* body stmts=%zu */\n",
          fn->stmt_count
        );
      }

      if(opt && opt->mode == VITTEC_EMIT_C_MODE_FULL) {
        const vitte_ast* ret = vittec__find_return_type_node(fn->ast);
        const char* module_hint = vitte_codegen_function_module_name(fn);

        vittec__emit_type_ref(b, opt, unit, module_hint, ret ? ret->text : "int");
        vittec__buf_printf(b, " %s(", cname);

        size_t pc = 0;
        for(const vitte_ast* ch = fn->ast ? fn->ast->first_child : NULL; ch; ch = ch->next) {
          if(ch->kind != VITTE_AST_PARAM) continue;
          const vitte_ast* ty = ch->first_child;
          if(pc++) vittec__buf_write(b, ", ", 2);
          vittec__emit_type_ref(b, opt, unit, module_hint, ty ? ty->text : "int");
          vittec__buf_write(b, " ", 1);
          vittec__emit_sanitized_ident(b, ch->text);
        }
        if(pc == 0) vittec__buf_write(b, "void", 4);
        vittec__buf_write(b, ") ", 2);

        vittec__strset_t locals = {0};
        const vitte_ast* block = fn->block;
        if(block && block->kind == VITTE_AST_BLOCK) {
          vittec__emit_block(b, block, 0, &locals);
        } else {
          vittec__buf_write(b, "{\n  return 0;\n}\n", 18);
        }
        vittec__buf_write(b, "\n", 1);
        vittec__strset_free(&locals);
      } else {
        /* Bootstrap signature: int(void). */
        vittec__buf_printf(b, "int %s(void) {\n", cname);
        vittec__buf_printf(b, "  /* TODO(stubs): lower Vitte body to C */\n");
        vittec__buf_printf(b, "  (void)0;\n");
        vittec__buf_printf(b, "  return 0;\n");
        vittec__buf_printf(b, "}\n\n");
      }
    }
  }
}

static void vittec__emit_main_buf(
  vittec__buf_t* b,
  const vittec_emit_c_options_t* opt,
  const vitte_codegen_unit* unit
) {
  size_t entry_count = unit ? unit->entrypoint_count : 0;
  vittec__buf_printf(b, "/* entrypoints: %zu */\n", entry_count);

  if(unit && entry_count) {
    const vitte_codegen_entrypoint* ep = &unit->entrypoints[0];
    const char* module_name = ep->module_path ? ep->module_path : "(root)";
    const char* symbol = ep->symbol ? ep->symbol : "main";

    for(size_t i = 0; i < entry_count; i++) {
      const vitte_codegen_entrypoint* cur = &unit->entrypoints[i];
      vittec__buf_printf(
        b,
        "/*   [%zu] %s::%s */\n",
        i,
        cur->module_path ? cur->module_path : "(root)",
        cur->symbol ? cur->symbol : "(anon)"
      );
    }

    char cname[512];
    vittec_sv_t mod_sv = vittec__sv_from_cstr(module_name);
    vittec_sv_t sym_sv = vittec__sv_from_cstr(symbol);
    vittec__mangle(cname, sizeof(cname), opt, mod_sv, sym_sv);

    vittec__buf_printf(b, "int main(void) {\n");
    if(opt && opt->mode == VITTEC_EMIT_C_MODE_FULL) {
      const vitte_ast* pblock = vittec__find_block_child(ep->ast);
      if(pblock && pblock->kind == VITTE_AST_BLOCK && pblock->first_child) {
        vittec__buf_printf(b, "  /* program block */\n");
        vittec__strset_t locals = {0};
        for(const vitte_ast* st = pblock->first_child; st; st = st->next) {
          vittec__emit_stmt(b, st, 1, &locals);
        }
        vittec__strset_free(&locals);
        vittec__buf_printf(b, "  return 0;\n");
      } else {
        vittec__buf_printf(b, "  /* fallback: call lowered entrypoint */\n");
        vittec__buf_printf(b, "  return %s();\n", cname);
      }
    } else {
      vittec__buf_printf(b, "  /* bootstrap: call lowered entrypoint */\n");
      vittec__buf_printf(b, "  return %s();\n", cname);
    }
    vittec__buf_printf(b, "}\n");
  } else {
    vittec__buf_printf(b, "int main(void) {\n");
    vittec__buf_printf(b, "  /* bootstrap: no entrypoints detected */\n");
    vittec__buf_printf(b, "  return 0;\n");
    vittec__buf_printf(b, "}\n");
  }
}

static int vittec__emit_unit_buffers(
  const vitte_codegen_unit* unit,
  const vittec_emit_c_options_t* opt,
  const char* header_include_leaf,
  const char* header_guard_hint,
  vittec__buf_t* cbuf,
  vittec__buf_t* hbuf
) {
  if(!unit || !opt || !cbuf) return VITTEC_EMIT_C_EINVAL;

  vittec__emit_preamble_buf(cbuf, opt);

  if(opt->emit_header) {
    const char* inc = (header_include_leaf && header_include_leaf[0]) ? header_include_leaf : "generated.h";
    vittec__buf_printf(cbuf, "#include \"%s\"\n\n", inc);
  }

  vittec__emit_modules_buf(cbuf, unit);
  vittec__emit_types_buf(cbuf, opt, unit);
  vittec__emit_functions_buf(cbuf, opt, unit);
  vittec__emit_main_buf(cbuf, opt, unit);

  if(opt->emit_header) {
    if(!hbuf) return VITTEC_EMIT_C_EINVAL;
    char guard[512];
    vittec__make_header_guard(guard, opt, header_guard_hint);
    vittec__emit_header_buf(hbuf, opt, guard, unit);
  }

  return VITTEC_EMIT_C_OK;
}

void vittec_emit_c_buffer_init(vittec_emit_c_buffer_t* buf) {
  if(!buf) return;
  buf->c_data = NULL;
  buf->c_size = 0;
  buf->h_data = NULL;
  buf->h_size = 0;
}

void vittec_emit_c_buffer_reset(vittec_emit_c_buffer_t* buf) {
  if(!buf) return;
  if(buf->c_data) free(buf->c_data);
  if(buf->h_data) free(buf->h_data);
  vittec_emit_c_buffer_init(buf);
}

static int vittec__transfer_buffer(vittec__buf_t* src, char** out_data, size_t* out_size) {
  if(!src || !out_data || !out_size) return VITTEC_EMIT_C_EINVAL;
  if(!vittec__buf_reserve(src, 1)) return VITTEC_EMIT_C_EINTERNAL;
  src->data[src->len] = 0;
  *out_data = src->data;
  *out_size = src->len;
  src->data = NULL;
  src->len = 0;
  src->cap = 0;
  return VITTEC_EMIT_C_OK;
}

int vittec_emit_c_buffer(
  const vitte_codegen_unit* unit,
  vittec_emit_c_buffer_t* out,
  const vittec_emit_c_options_t* opt_in
) {
  if(!unit || !out) return VITTEC_EMIT_C_EINVAL;

  vittec_emit_c_buffer_reset(out);

  vittec_emit_c_options_t opt = vittec__opt_or_default(opt_in);

  vittec__buf_t cbuf;
  vittec__buf_init(&cbuf);
  vittec__buf_t hbuf;
  vittec__buf_init(&hbuf);

  const char* header_leaf = NULL;
  const char* guard_hint = NULL;
  if(opt.emit_header) {
    header_leaf = "generated.h";
    guard_hint = "generated";
  }

  int rc = vittec__emit_unit_buffers(unit, &opt, header_leaf, guard_hint, &cbuf, opt.emit_header ? &hbuf : NULL);
  if(rc != VITTEC_EMIT_C_OK) {
    vittec__buf_free(&cbuf);
    vittec__buf_free(&hbuf);
    return rc;
  }

  rc = vittec__transfer_buffer(&cbuf, &out->c_data, &out->c_size);
  if(rc != VITTEC_EMIT_C_OK) {
    vittec__buf_free(&cbuf);
    vittec__buf_free(&hbuf);
    vittec_emit_c_buffer_reset(out);
    return rc;
  }

  if(opt.emit_header) {
    rc = vittec__transfer_buffer(&hbuf, &out->h_data, &out->h_size);
    if(rc != VITTEC_EMIT_C_OK) {
      vittec__buf_free(&cbuf);
      vittec__buf_free(&hbuf);
      vittec_emit_c_buffer_reset(out);
      return rc;
    }
  }

  vittec__buf_free(&cbuf);
  vittec__buf_free(&hbuf);
  return VITTEC_EMIT_C_OK;
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

int vittec_emit_c_file(const vitte_codegen_unit* unit, const char* out_path) {
  return vittec_emit_c_file_ex(unit, out_path, NULL);
}

int vittec_emit_c_file_ex(
  const vitte_codegen_unit* unit,
  const char* out_path,
  const vittec_emit_c_options_t* opt_in
) {
  if(!unit || !out_path || !out_path[0]) return VITTEC_EMIT_C_EINVAL;

  vittec_emit_c_options_t opt = vittec__opt_or_default(opt_in);

  char* base = NULL;
  char* c_path = NULL;
  char* h_path = NULL;
  const char* header_leaf = NULL;
  const char* guard_hint = NULL;

  if(opt.emit_header) {
    base = vittec__path_without_ext(out_path);
    if(!base) return VITTEC_EMIT_C_EINTERNAL;
    c_path = vittec__path_with_ext(base, ".c");
    h_path = vittec__path_with_ext(base, ".h");
    if(!c_path || !h_path) {
      if(base) free(base);
      if(c_path) free(c_path);
      if(h_path) free(h_path);
      return VITTEC_EMIT_C_EINTERNAL;
    }

    const char* inc_leaf = h_path;
    for(const char* p = h_path; *p; p++) {
      if(*p == '/' || *p == '\\') inc_leaf = p + 1;
    }
    header_leaf = inc_leaf;

    const char* guard_leaf = base;
    for(const char* p = base; guard_leaf && *p; p++) {
      if(*p == '/' || *p == '\\') guard_leaf = p + 1;
    }
    guard_hint = guard_leaf;
  } else {
    c_path = vittec__strdup(out_path);
    if(!c_path) return VITTEC_EMIT_C_EINTERNAL;
  }

  vittec__buf_t cbuf;
  vittec__buf_init(&cbuf);
  vittec__buf_t hbuf;
  vittec__buf_init(&hbuf);

  int rc = vittec__emit_unit_buffers(unit, &opt, header_leaf, guard_hint, &cbuf, opt.emit_header ? &hbuf : NULL);
  if(rc != VITTEC_EMIT_C_OK) goto cleanup;

  rc = vittec__write_file_or_path(&opt, c_path, cbuf.data ? cbuf.data : "", cbuf.len);
  if(rc != VITTEC_EMIT_C_OK) goto cleanup;

  if(opt.emit_header) {
    rc = vittec__write_file_or_path(&opt, h_path, hbuf.data ? hbuf.data : "", hbuf.len);
    if(rc != VITTEC_EMIT_C_OK) goto cleanup;
  }

cleanup:
  vittec__buf_free(&cbuf);
  vittec__buf_free(&hbuf);
  if(base) free(base);
  if(c_path) free(c_path);
  if(h_path) free(h_path);
  return rc;
}
