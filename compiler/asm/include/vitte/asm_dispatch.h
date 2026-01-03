// asm_dispatch.h
//
// Vitte ASM dispatch layer (public header).
//
// This header defines a small, stable dispatch interface used by the assembler/
// encoder pipeline to route opcodes to backend-specific emitters.
//
// Design goals:
// - Keep ABI minimal and C-friendly.
// - Allow multiple backends (x86_64, aarch64, wasm, etc.).
// - Be robust: callers can detect missing handlers without crashing.
//
// Notes:
// - This is not a JIT; dispatch routes "emit" functions that append bytes to an
//   output sink.
// - Thread-safety: a dispatch table is mutable; treat it as thread-confined or
//   protect it externally.

#ifndef VITTE_ASM_DISPATCH_H
#define VITTE_ASM_DISPATCH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Versioning
// -----------------------------------------------------------------------------

#define VITTE_ASM_DISPATCH_ABI_VERSION 1

// -----------------------------------------------------------------------------
// Visibility / calling convention helpers
// -----------------------------------------------------------------------------

#if defined(_WIN32) && !defined(__clang__)
  #define VITTE_ASM_CALL __cdecl
#else
  #define VITTE_ASM_CALL
#endif

#if defined(_WIN32)
  #if defined(VITTE_ASM_BUILD_DLL)
    #define VITTE_ASM_API __declspec(dllexport)
  #elif defined(VITTE_ASM_USE_DLL)
    #define VITTE_ASM_API __declspec(dllimport)
  #else
    #define VITTE_ASM_API
  #endif
#else
  #define VITTE_ASM_API
#endif

// -----------------------------------------------------------------------------
// Result codes
// -----------------------------------------------------------------------------

typedef enum vitte_asm_rc {
  VITTE_ASM_OK = 0,
  VITTE_ASM_ERR = 1,
  VITTE_ASM_EINVAL = 2,
  VITTE_ASM_ENOMEM = 3,
  VITTE_ASM_ENOTSUP = 4,
  VITTE_ASM_EBOUNDS = 5
} vitte_asm_rc;

// Convert a result code to a stable string.
VITTE_ASM_API const char* VITTE_ASM_CALL vitte_asm_rc_str(vitte_asm_rc rc);

// -----------------------------------------------------------------------------
// Target / backend
// -----------------------------------------------------------------------------

typedef enum vitte_asm_arch {
  VITTE_ASM_ARCH_UNKNOWN = 0,
  VITTE_ASM_ARCH_X86_64 = 1,
  VITTE_ASM_ARCH_AARCH64 = 2,
  VITTE_ASM_ARCH_WASM32 = 3,
  VITTE_ASM_ARCH_WASM64 = 4
} vitte_asm_arch;

typedef struct vitte_asm_target {
  vitte_asm_arch arch;
  uint32_t features;   // backend-defined bitset (e.g., SSE2/AVX2/NEON)
  uint32_t reserved;
} vitte_asm_target;

// -----------------------------------------------------------------------------
// Byte sink (output)
// -----------------------------------------------------------------------------

typedef struct vitte_asm_sink {
  uint8_t* data;       // buffer (owned by caller)
  size_t len;          // bytes written
  size_t cap;          // total capacity

  // Optional: user callbacks for "streaming" sinks.
  void* user;
  // Called when the sink needs to grow. Return true if growth succeeded and
  // the sink pointers/cap updated.
  bool (*grow)(struct vitte_asm_sink* sink, size_t min_cap);
} vitte_asm_sink;

// Append raw bytes to sink. Uses grow() if needed.
VITTE_ASM_API vitte_asm_rc VITTE_ASM_CALL vitte_asm_sink_write(
  vitte_asm_sink* sink,
  const void* bytes,
  size_t n
);

// Convenience: write a single byte.
VITTE_ASM_API vitte_asm_rc VITTE_ASM_CALL vitte_asm_sink_put8(
  vitte_asm_sink* sink,
  uint8_t b
);

// -----------------------------------------------------------------------------
// Instruction model (minimal)
// -----------------------------------------------------------------------------

// Operand kinds are intentionally minimal; backends can interpret registers and
// addressing via backend-specific encoding in the payload fields.

typedef enum vitte_asm_op_kind {
  VITTE_ASM_OP_NONE = 0,
  VITTE_ASM_OP_REG  = 1,
  VITTE_ASM_OP_IMM  = 2,
  VITTE_ASM_OP_MEM  = 3,
  VITTE_ASM_OP_LABEL = 4
} vitte_asm_op_kind;

typedef struct vitte_asm_operand {
  vitte_asm_op_kind kind;
  uint8_t width_bits;     // 0 = backend default
  uint16_t reserved;

  // Generic payload fields (backend-defined interpretation):
  uint64_t a;
  uint64_t b;
} vitte_asm_operand;

// Opcodes are backend-defined. The dispatch layer treats them as u16.
//
// Convention (recommended):
// - 0..255 reserved for common op families.
// - >=256 for backend/private.

typedef struct vitte_asm_insn {
  uint16_t opcode;
  uint8_t  op_count;      // number of operands in ops[]
  uint8_t  flags;         // backend-defined
  uint32_t reserved;

  vitte_asm_operand ops[4];
} vitte_asm_insn;

// -----------------------------------------------------------------------------
// Dispatch
// -----------------------------------------------------------------------------

typedef struct vitte_asm_emit_ctx {
  vitte_asm_target target;
  vitte_asm_sink*  sink;

  // Optional user data forwarded to handlers.
  void* user;
} vitte_asm_emit_ctx;

// Handler signature: emit encoded bytes for an instruction.
// Return:
// - VITTE_ASM_OK on success
// - VITTE_ASM_ENOTSUP if opcode not supported by this backend
// - other codes for malformed operands, sink errors, etc.

typedef vitte_asm_rc (VITTE_ASM_CALL *vitte_asm_emit_fn)(
  vitte_asm_emit_ctx* ctx,
  const vitte_asm_insn* insn
);

// Dispatch table: map opcode -> emitter.
//
// For simplicity and speed, we use a fixed 1024-entry table.
// If your opcode space grows, bump VITTE_ASM_DISPATCH_TABLE_SIZE with an ABI
// version increment.

#define VITTE_ASM_DISPATCH_TABLE_SIZE 1024u

typedef struct vitte_asm_dispatch {
  uint32_t abi_version;   // must be VITTE_ASM_DISPATCH_ABI_VERSION
  vitte_asm_target target;

  // Table entries may be NULL.
  vitte_asm_emit_fn table[VITTE_ASM_DISPATCH_TABLE_SIZE];
} vitte_asm_dispatch;

// Initialize a dispatch table for a target. Table is zeroed.
VITTE_ASM_API void VITTE_ASM_CALL vitte_asm_dispatch_init(
  vitte_asm_dispatch* d,
  vitte_asm_target target
);

// Set an emitter for an opcode.
VITTE_ASM_API vitte_asm_rc VITTE_ASM_CALL vitte_asm_dispatch_set(
  vitte_asm_dispatch* d,
  uint16_t opcode,
  vitte_asm_emit_fn fn
);

// Get an emitter for an opcode (may return NULL).
VITTE_ASM_API vitte_asm_emit_fn VITTE_ASM_CALL vitte_asm_dispatch_get(
  const vitte_asm_dispatch* d,
  uint16_t opcode
);

// Dispatch an instruction to the emitter.
VITTE_ASM_API vitte_asm_rc VITTE_ASM_CALL vitte_asm_dispatch_emit(
  const vitte_asm_dispatch* d,
  vitte_asm_sink* sink,
  const vitte_asm_insn* insn,
  void* user
);

// Helper: returns true if an opcode has a handler installed.
VITTE_ASM_API bool VITTE_ASM_CALL vitte_asm_dispatch_has(
  const vitte_asm_dispatch* d,
  uint16_t opcode
);

// Optional: install a baseline set of handlers for the given target.
// Implementations may be provided by backend libraries.
VITTE_ASM_API vitte_asm_rc VITTE_ASM_CALL vitte_asm_dispatch_install_defaults(
  vitte_asm_dispatch* d
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_ASM_DISPATCH_H
