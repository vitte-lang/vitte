// C:\Users\vince\Documents\GitHub\vitte\fuzz\targets\fuze_vm_decode.c
// Fuzz target: VM bytecode decoder / verifier entrypoint (libFuzzer/AFL compatible)
//
// This target is intentionally decoupled from the actual Vitte VM implementation.
// Hook points are provided so you can wire to your real decoder:
//
//   - Define VITTE_VM_DECODE_AVAILABLE=1 and provide:
//       int vitte_vm_decode(const uint8_t* data, size_t size);
//       int vitte_vm_verify(const uint8_t* data, size_t size);   (optional)
//       int vitte_vm_disasm(const uint8_t* data, size_t size);   (optional)
//
// If those symbols are not present, this target will still exercise
// structured parsing logic (headers/varints) as a placeholder.
//
// Build modes:
//  - libFuzzer: compile with -fsanitize=fuzzer,address and link VM libs
//  - AFL++: compile with AFL instrumentation and link, use fuzz_driver_afl.c
//
// Output: no printing; crashes only on internal invariant violations.

#include <stddef.h>
#include <stdint.h>

#include "fuzz/fuzz.h"
#include "fuzz/fuzz_trace.h"

#ifndef VITTE_VM_DECODE_AVAILABLE
#define VITTE_VM_DECODE_AVAILABLE 0
#endif

#if VITTE_VM_DECODE_AVAILABLE
// User-provided hooks
int vitte_vm_decode(const uint8_t* data, size_t size);
int vitte_vm_verify(const uint8_t* data, size_t size); // optional
int vitte_vm_disasm(const uint8_t* data, size_t size); // optional
#endif

// -----------------------------------------------------------------------------
// Placeholder “format” probes (keeps coverage even before wiring real VM)
// -----------------------------------------------------------------------------
// We interpret the input as:
//   u32 magic (LE) | u16 version (LE) | uvar sections | repeated:
//     u8 tag | uvar len | bytes[len]
//
// This is NOT the real format; it just provides structured code paths.

static int
vm_decode_placeholder(const uint8_t* data, size_t size) {
  fuzz_reader r = fuzz_reader_from(data, size);

  uint32_t magic = 0;
  uint16_t ver = 0;

  if (!fuzz_reader_read_u32_le(&r, &magic))
    return 0;
  if (!fuzz_reader_read_u16_le(&r, &ver))
    return 0;

  FUZZ_TRACE_TAG_U32("vm:magic", magic);
  FUZZ_TRACE_TAG_U32("vm:ver", ver);

  // Alternate endianness probe (helps coverage on wrong-endian code paths).
  if ((magic & 0xFFu) == 0u && fuzz_reader_remaining(&r) >= 2) {
    uint16_t be = 0;
    fuzz_reader rr = r;
    if (fuzz_reader_read_u16_be(&rr, &be)) {
      FUZZ_TRACE_TAG_U32("vm:probe_be16", (uint32_t)be);
    }
  }

  // Mild constraints to bias interesting paths
  if ((magic & 0xFFFFu) == 0x564Du) { // 'VM' pattern
    FUZZ_TRACE_TAG("vm:magic_hit");
  }

  uint64_t sections = 0;
  if (!fuzz_reader_read_uvar(&r, &sections, 10))
    return 0;
  if (sections > 1024)
    sections = 1024;

  for (uint64_t i = 0; i < sections; ++i) {
    uint8_t tag = 0;
    if (!fuzz_reader_read_u8(&r, &tag))
      break;

    uint64_t len64 = 0;
    if (!fuzz_reader_read_uvar(&r, &len64, 10))
      break;

    size_t len = (size_t)len64;
    if ((uint64_t)len != len64)
      break;

    // Cap per-section length to remaining
    if (len > fuzz_reader_remaining(&r))
      len = fuzz_reader_remaining(&r);

    fuzz_bytes_view payload;
    if (!fuzz_reader_read_bytes_view(&r, len, &payload))
      break;

    FUZZ_TRACE_TAG_U32("vm:section_tag", (uint32_t)tag);
    FUZZ_TRACE_TAG_U32("vm:section_len",
                       (uint32_t)(payload.size > 0xFFFFFFFFu ? 0xFFFFFFFFu : payload.size));

    // Cheap hashing of section content to vary paths
    uint32_t h = fuzz_fnv1a32(payload.data, payload.size);
    FUZZ_TRACE_U32(tag, h);

    // Nested "section list" interpretation (pure placeholder, but exercises
    // bounded varint + sub-read logic).
    if ((tag % 5u) == 0u && payload.size > 0) {
      fuzz_reader sub = fuzz_reader_from(payload.data, payload.size);
      uint64_t inner = 0;
      if (fuzz_reader_read_uvar(&sub, &inner, 4)) {
        if (inner > 32)
          inner = 32;
        for (uint64_t j = 0; j < inner && fuzz_reader_remaining(&sub) > 0; ++j) {
          uint8_t itag = 0;
          if (!fuzz_reader_read_u8(&sub, &itag))
            break;
          fuzz_bytes_view inner_payload;
          if (!fuzz_reader_read_len_prefixed(&sub, 64, &inner_payload))
            break;
          FUZZ_TRACE_TAG_U32("vm:inner_tag", (uint32_t)itag);
          FUZZ_TRACE_TAG_U32("vm:inner_len", (uint32_t)inner_payload.size);
        }
      }
    }

    // Some branchy behavior
    if ((tag % 7u) == 0u && payload.size >= 4) {
      fuzz_reader rr = fuzz_reader_from(payload.data, payload.size);
      uint32_t x = 0;
      if (fuzz_reader_read_u32_le(&rr, &x)) {
        FUZZ_TRACE_TAG_U32("vm:tag7:x", x);
      }
    }
  }

  return 0;
}

// -----------------------------------------------------------------------------
// FUZZ TARGET
// -----------------------------------------------------------------------------

FUZZ_TARGET(fuze_vm_decode_target) {
#if FUZZ_DRIVER_RETURN_ON_EMPTY
  if (size == 0)
    return 0;
#endif

  // Deterministic mode byte (optional) to choose paths
  fuzz_reader r = fuzz_reader_from(data, size);
  uint8_t mode = 0;
  (void)fuzz_reader_read_u8(&r, &mode);

  FUZZ_TRACE_TAG_U32("vm:mode", (uint32_t)mode);

#if VITTE_VM_DECODE_AVAILABLE
  // In “real” mode, pass original buffer to the VM decoder.
  // Keep it simple: call decode, optionally verify/disasm depending on mode bits.
  (void)vitte_vm_decode(data, size);

  if (mode & 1u) {
    // If verifier exists, call it; if not linked, the symbol will cause link error.
    // Keep optional via ifdef in your build if needed.
    (void)vitte_vm_verify(data, size);
  }
  if (mode & 2u) {
    (void)vitte_vm_disasm(data, size);
  }
#else
  // Placeholder parser
  (void)vm_decode_placeholder(data, size);
#endif

  return 0;
}

// -----------------------------------------------------------------------------
// Driver binding
// -----------------------------------------------------------------------------
// For libFuzzer builds that compile this TU directly, enable the driver by
// defining FUZZ_DRIVER_TARGET here.
#define FUZZ_DRIVER_TARGET fuze_vm_decode_target
#include "fuzz/fuzz_driver.h"
