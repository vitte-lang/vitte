// vitte/src/asm/include/vitte/asm.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Vitte ASM — API C (frontend assembleur + encodeur + diagnostics)
//
// Objectifs:
//   - Assembler du texte -> bytes (ex: .o-like en mémoire ou blob exécutable VM)
//   - Supporter plusieurs cibles via "backend" (x86_64, aarch64, vitte-vm, ...)
//   - Fournir diagnostics structurés (erreurs/warnings + positions source)
//   - API stable et minimale pour intégration dans vittec / outils
// ============================================================================

// ---------------------------
// Versioning / ABI
// ---------------------------

#define VITTE_ASM_ABI_MAJOR 0
#define VITTE_ASM_ABI_MINOR 1
#define VITTE_ASM_ABI_PATCH 0

typedef struct vitte_asm_version_t {
  uint16_t major;
  uint16_t minor;
  uint16_t patch;
} vitte_asm_version_t;

vitte_asm_version_t vitte_asm_version(void);

// ---------------------------
// Status / Errors
// ---------------------------

typedef enum vitte_asm_status_t {
  VITTE_ASM_OK = 0,

  // generic
  VITTE_ASM_EINVAL = 1,
  VITTE_ASM_ENOMEM = 2,
  VITTE_ASM_EINTERNAL = 3,
  VITTE_ASM_EUNSUPPORTED = 4,

  // io / input
  VITTE_ASM_EINPUT = 10,
  VITTE_ASM_EUTF8 = 11,

  // parsing / lexing
  VITTE_ASM_ELEX = 20,
  VITTE_ASM_EPARSE = 21,

  // semantic / assemble
  VITTE_ASM_ESEM = 30,
  VITTE_ASM_ERESOLVE = 31,
  VITTE_ASM_EENCODE = 32,

  // limits
  VITTE_ASM_ELIMIT = 40
} vitte_asm_status_t;

const char* vitte_asm_status_str(vitte_asm_status_t st);

// ---------------------------
// Targets / Modes
// ---------------------------

typedef enum vitte_asm_target_t {
  VITTE_ASM_TARGET_NONE = 0,
  VITTE_ASM_TARGET_VITTE_VM = 1,
  VITTE_ASM_TARGET_X86_64 = 2,
  VITTE_ASM_TARGET_AARCH64 = 3
} vitte_asm_target_t;

typedef enum vitte_asm_endian_t {
  VITTE_ASM_ENDIAN_LITTLE = 0,
  VITTE_ASM_ENDIAN_BIG = 1
} vitte_asm_endian_t;

typedef enum vitte_asm_cpu_mode_t {
  VITTE_ASM_CPU_MODE_DEFAULT = 0,
  VITTE_ASM_CPU_MODE_32 = 1,
  VITTE_ASM_CPU_MODE_64 = 2
} vitte_asm_cpu_mode_t;

// ---------------------------
// Source positions / spans
// ---------------------------

typedef struct vitte_asm_loc_t {
  uint32_t line;    // 1-based
  uint32_t column;  // 1-based
  uint32_t offset;  // 0-based byte offset in source
} vitte_asm_loc_t;

typedef struct vitte_asm_span_t {
  vitte_asm_loc_t begin;
  vitte_asm_loc_t end;
} vitte_asm_span_t;

typedef enum vitte_asm_diag_severity_t {
  VITTE_ASM_DIAG_NOTE = 0,
  VITTE_ASM_DIAG_WARN = 1,
  VITTE_ASM_DIAG_ERROR = 2
} vitte_asm_diag_severity_t;

typedef struct vitte_asm_diag_t {
  vitte_asm_diag_severity_t severity;
  vitte_asm_status_t code;     // catégorie principale
  vitte_asm_span_t span;       // zone source si dispo
  const char* message;         // UTF-8, owned by diag store
} vitte_asm_diag_t;

// ---------------------------
// Byte buffer (output)
// ---------------------------

typedef struct vitte_asm_bytes_t {
  uint8_t* data;   // owned by this struct; free with vitte_asm_bytes_free
  size_t len;
  size_t cap;
} vitte_asm_bytes_t;

void vitte_asm_bytes_init(vitte_asm_bytes_t* out);
void vitte_asm_bytes_free(vitte_asm_bytes_t* out);

// ---------------------------
// Symbol map (optionnel)
// ---------------------------

typedef struct vitte_asm_symbol_t {
  const char* name;  // owned by symbol store
  uint64_t value;    // address/offset
  uint32_t flags;    // backend-specific (global/local/section/...)
} vitte_asm_symbol_t;

typedef struct vitte_asm_symtab_t vitte_asm_symtab_t;

size_t vitte_asm_symtab_len(const vitte_asm_symtab_t* t);
const vitte_asm_symbol_t* vitte_asm_symtab_get(const vitte_asm_symtab_t* t, size_t i);
void vitte_asm_symtab_free(vitte_asm_symtab_t* t);

// ---------------------------
// Options
// ---------------------------

typedef struct vitte_asm_options_t {
  vitte_asm_target_t target;

  // CPU / encoding hints
  vitte_asm_cpu_mode_t cpu_mode;
  vitte_asm_endian_t endian;

  // Features
  uint8_t enable_warnings;     // 0/1
  uint8_t treat_warnings_as_errors;
  uint8_t allow_unknown_mnemonics; // permissif (pour prototypage)
  uint8_t emit_symtab;         // produire une table des symboles si supporté

  // Limits
  uint32_t max_diags;          // 0 = illimité (attention mémoire)
  uint32_t max_output_bytes;   // 0 = illimité

  // Include paths (préprocesseur léger / directives .include)
  const char* const* include_dirs;
  size_t include_dirs_len;
} vitte_asm_options_t;

void vitte_asm_options_default(vitte_asm_options_t* opt);

// ---------------------------
// Context (arena + diag store)
// ---------------------------

typedef struct vitte_asm_ctx_t vitte_asm_ctx_t;

typedef void* (*vitte_asm_alloc_fn)(void* user, size_t size);
typedef void* (*vitte_asm_realloc_fn)(void* user, void* ptr, size_t size);
typedef void (*vitte_asm_free_fn)(void* user, void* ptr);

typedef struct vitte_asm_allocator_t {
  void* user;
  vitte_asm_alloc_fn alloc;
  vitte_asm_realloc_fn realloc;
  vitte_asm_free_fn free;
} vitte_asm_allocator_t;

vitte_asm_allocator_t vitte_asm_allocator_default(void);

vitte_asm_ctx_t* vitte_asm_ctx_new(const vitte_asm_allocator_t* a);
void vitte_asm_ctx_free(vitte_asm_ctx_t* ctx);

void vitte_asm_ctx_reset(vitte_asm_ctx_t* ctx); // reset arena/diags (garde l'allocateur)

// Diagnostics
size_t vitte_asm_diag_count(const vitte_asm_ctx_t* ctx);
const vitte_asm_diag_t* vitte_asm_diag_get(const vitte_asm_ctx_t* ctx, size_t i);
void vitte_asm_diag_clear(vitte_asm_ctx_t* ctx);

// ---------------------------
// Assemble API
// ---------------------------

// Input: source text UTF-8 (non forcément nul-terminé).
// Output: bytes dans `out_bytes` (owned, free via vitte_asm_bytes_free).
// Symtab: si opt.emit_symtab=1, `*out_symtab` reçoit une table (free via vitte_asm_symtab_free).
//
// Retour:
//   - VITTE_ASM_OK si encodage OK
//   - sinon code d'erreur + diags consultables via vitte_asm_diag_*
//
// Note: `source_name` est utilisé pour les diags (peut être NULL).
vitte_asm_status_t vitte_asm_assemble(
    vitte_asm_ctx_t* ctx,
    const vitte_asm_options_t* opt,
    const char* source_name,
    const uint8_t* source_utf8,
    size_t source_len,
    vitte_asm_bytes_t* out_bytes,
    vitte_asm_symtab_t** out_symtab);

// Convenience: assemble depuis C-string nul-terminée
vitte_asm_status_t vitte_asm_assemble_cstr(
    vitte_asm_ctx_t* ctx,
    const vitte_asm_options_t* opt,
    const char* source_name,
    const char* source_cstr,
    vitte_asm_bytes_t* out_bytes,
    vitte_asm_symtab_t** out_symtab);

// ---------------------------
// Streaming writer (optionnel)
// ---------------------------

typedef vitte_asm_status_t (*vitte_asm_write_fn)(void* user, const uint8_t* data, size_t len);

typedef struct vitte_asm_writer_t {
  void* user;
  vitte_asm_write_fn write;
} vitte_asm_writer_t;

// Encode directement vers un writer (utile pour gros outputs).
// Peut aussi remplir symtab si activé.
vitte_asm_status_t vitte_asm_assemble_to_writer(
    vitte_asm_ctx_t* ctx,
    const vitte_asm_options_t* opt,
    const char* source_name,
    const uint8_t* source_utf8,
    size_t source_len,
    const vitte_asm_writer_t* writer,
    vitte_asm_symtab_t** out_symtab);

// ---------------------------
// Feature / backend queries
// ---------------------------

typedef struct vitte_asm_backend_info_t {
  vitte_asm_target_t target;
  const char* name;
  const char* description;
  uint32_t features; // bitset backend-specific
} vitte_asm_backend_info_t;

size_t vitte_asm_backends_count(void);
const vitte_asm_backend_info_t* vitte_asm_backend_get(size_t i);

uint8_t vitte_asm_backend_supports(vitte_asm_target_t t);

// ---------------------------
// Debug helpers
// ---------------------------

// Dump bytes en hex dans un buffer texte (UTF-8).
// Retourne le nombre de bytes écrits (hors '\0'). Si buf==NULL, retourne la taille requise.
size_t vitte_asm_hex_dump(const uint8_t* data, size_t len, char* buf, size_t buf_len);

#ifdef __cplusplus
} // extern "C"
#endif
