# Architecture

TODO

# Architecture — beryl-crypto

This document describes the architecture of the **beryl-crypto** plugin: module layout, runtime boundary, provider model, handle tables, views/iovec, streaming, policy enforcement, and test/bench strategy.

This is a **design/implementation guide** (not a user manual). For the public compatibility contract, see `docs/api_stability.md`.

---

## 1) Goals

### Primary
- Provide a **stable** public surface under `plugins.crypto.api.*`.
- Support multiple backends via **providers** (software / hardware / remote-KMS).
- Offer **zero-copy** variants via views (`ByteSlice`, `MutByteSlice`) and **scatter/gather** (`IoVec`, `MutIoVec`).
- Provide a consistent **streaming** model across families.
- Enforce security policies (constant-time best-effort, side-channel hardening, nonce policy, limits).

### Non-goals
- Guarantee specific performance targets.
- Mandate one “best” backend; selection is policy-driven.
- Expose internal implementation details as stable API.

---

## 2) High-level layering

```
Downstream code
  ↓
plugins.crypto.api.*   (public surface: requests, views, errors, policies)
  ↓
plugins.crypto.core.*  (internal glue: dispatch, validation, shared utilities)
  ↓
Provider boundary      (software/hw/remote providers)
  ↓
Runtime boundary       (handle tables, marshalling, secure memory, threading)
```

### 2.1 Public API layer (`plugins.crypto.api.*`)
- Defines:
  - requests/responses and ABI-friendly structs
  - policy structs (constant_time, sidechannel_hard, strict_nonce, limits)
  - common view types (`ByteSlice`, `MutByteSlice`, `IoVec`, `MutIoVec`)
  - stable error taxonomy
- Does **not** depend on concrete implementations.

### 2.2 Core internal layer (`plugins.crypto.core.*`)
- Dispatch, validation, and common logic:
  - input validation (lengths, limits, state machine)
  - nonce policy glue (provider capability checks)
  - iovec flattening helpers (where needed)
  - constant-time helpers and memwipe wrappers
- This layer is **internal** and may change freely.

### 2.3 Provider layer
- Concrete algorithm implementations:
  - software provider (portable constant-time best-effort)
  - hardware providers (AES-NI / ARMv8 / CLMUL/PMULL)
  - remote/KMS providers (wrap/unwrap, key isolation)
- Providers expose:
  - `ProviderInfo` (id, name, flags)
  - `AlgInfo` (alg id, limits, flags)
  - operation entrypoints (seal/open, hash, sign, kex, rng, …)

### 2.4 Runtime layer
- Supplies infrastructure and platform integration:
  - handle tables (slot+generation)
  - threading / synchronization primitives
  - secure memory (optional: mlock/guard pages)
  - marshalling support (struct ABI out)
  - monotonic time / cycle counter for benches

---

## 3) Module map (recommended)

### 3.1 Public API families
- `plugins.crypto.api.types`
  - `ByteSlice`, `MutByteSlice`, `IoVec`, `MutIoVec`
  - small ABI helper structs
- `plugins.crypto.api.errors`
  - stable error enums and helpers
- `plugins.crypto.api.registry`
  - enumerate/query/select provider interfaces
- `plugins.crypto.api.rand`
  - CSPRNG and fill APIs
- `plugins.crypto.api.hash`
  - SHA-*, BLAKE3 (fixed + xof)
- `plugins.crypto.api.mac`
  - HMAC-* and other MACs
- `plugins.crypto.api.aead`
  - AES-GCM, AES-GCM-SIV, ChaCha20-Poly1305, XChaCha20-Poly1305
- `plugins.crypto.api.cipher`
  - AES block + modes (ECB/CBC/CTR/XTS) if exposed as non-AEAD
- `plugins.crypto.api.sign`
  - Ed25519
- `plugins.crypto.api.kex`
  - X25519
- `plugins.crypto.api.subtle`
  - constant-time compares, ct-bool normalize, memwipe hooks

### 3.2 Provider packages
- `plugins.crypto.providers.software.*`
- `plugins.crypto.providers.hw.*`
- `plugins.crypto.providers.remote.*`

### 3.3 Benches
- `plugins/beryl-crypto/benches/*`
  - non-API harnesses; may change freely

---

## 4) Registry and provider selection

### 4.1 Registry responsibilities
The registry is the single entrypoint for:
- provider enumeration
- algorithm discovery and capability querying
- deterministic selection policy for `default`

### 4.2 Selection workflow
1. Downstream requests `provider_select(provider_id, alg_id)`.
2. Registry resolves provider (explicit id) or chooses `default` based on priority.
3. Registry checks algorithm availability and compatibility flags.
4. Registry returns a provider handle (or stores selection in context creation).

### 4.3 Priority policy
Recommended deterministic order:
1. hardware-backed provider
2. verified constant-time software provider
3. remote/KMS provider
4. fallback software

Policy MUST be overridable by explicit provider id.

---

## 5) Handle tables: slot + generation

### 5.1 Why
- Prevents stale references after destroy
- Enables O(1) lookup
- Supports idempotent destroy

### 5.2 Structure
A handle is logically:
- `kind` (Provider/Key/Context/Stream)
- `slot` (index)
- `generation` (monotonic per slot)

Lookup algorithm:
1. decode handle
2. check kind
3. load entry at slot
4. compare generation
5. return pointer/ref or error

### 5.3 Thread-safety
- Tables SHOULD support concurrent reads.
- Create/destroy MUST synchronize generation updates.
- Use-after-destroy returns stable `*_Unavailable` or `InvalidHandle`.

### 5.4 Poisoning
After destroy:
- entry payload is cleared
- generation increments
- optional poison marker used for debugging

---

## 6) Byte views and IoVec

### 6.1 Views
- `ByteSlice`: read-only borrowed view
- `MutByteSlice`: mutable borrowed view

Rules:
- Views MUST reference valid memory for the duration of the call.
- Providers MUST NOT retain pointers beyond the call unless explicitly documented.

### 6.2 IoVec
- `IoVec`: list of `ByteSlice` segments
- `MutIoVec`: list of `MutByteSlice` segments

Semantics:
- Inputs behave as concatenation.
- For outputs, providers may require contiguous output for tags/digests; if so, return `BufferTooSmall`.

### 6.3 Flattening strategy
- Prefer provider-native iovec support when capability flag indicates.
- Else, core may flatten into a temporary buffer (with memwipe after use) if policy allows.

---

## 7) Policies and limits

### 7.1 Common policy flags
- `constant_time`: best-effort constant-time behavior
- `sidechannel_hard`: stronger hardening (no secret-dependent branches/memory when feasible)
- `strict_nonce`: detect reuse/exhaustion when possible

### 7.2 Limits
Each algorithm should expose limits:
- `msg_max`, `aad_max`, `out_max`, `key_lens`, `nonce_lens`, `tag_lens`

Core MUST validate against limits before calling providers (unless provider advertises it validates internally).

---

## 8) Streaming architecture

### 8.1 Shared state machine
All streaming APIs follow:
- `start` → `update*` → `finish`

Required properties:
- `finish` idempotent (preferred)
- `destroy` safe in any state
- invalid sequences return `InvalidState`

### 8.2 Hash streaming
- Accepts arbitrary chunks.
- Maintains internal block buffer + total length.

### 8.3 AEAD streaming
Two main designs:
- **incremental** (CTR + incremental authenticator like GHASH/POLYVAL)
- **chunked** (provider-defined chunk MAC)

Contracts:
- tag is only available at `finish`.
- open verifies tag and returns stable failure without leaking.

### 8.4 CBC-like streaming
If block-multiple chunking is required:
- enforce it in validation
- `finish` handles padding rules

---

## 9) Algorithm family notes

### 9.1 AEAD
- AES-GCM
  - uses AES block + GHASH
  - benefits from CLMUL/PMULL when available
- AES-GCM-SIV (RFC 8452)
  - uses AES-CTR + POLYVAL
  - requires strict handling of nonce processing and subkey derivation
- ChaCha20-Poly1305 (RFC 8439)
  - ChaCha20 stream + Poly1305
- XChaCha20-Poly1305
  - HChaCha20 to derive subkey, 24-byte nonce

### 9.2 Hashes
- SHA-256, SHA-1
  - compression function + padding
- BLAKE3
  - chunk chaining + XOF mode

### 9.3 Signatures
- Ed25519
  - deterministic signatures
  - verify should be constant-time best-effort

### 9.4 Key exchange
- X25519
  - scalar clamping
  - fixed-base/variable-base optional fast paths

---

## 10) Side-channel hardening and memwipe

### 10.1 Constant-time and secret-independent behavior
When `constant_time` or `sidechannel_hard` is set:
- avoid branches based on secret data where feasible
- avoid secret-indexed memory access where feasible
- use constant-time compares for tags/signatures/padding where feasible

### 10.2 Memory wiping
- Wipe temporary buffers:
  - round keys
  - keystream blocks
  - GHASH/POLYVAL accumulators
  - flattened iovec buffers
- Wipe on destroy for keys/contexts/streams.

The `plugins.crypto.api.subtle` module should expose a single `memwipe` primitive used consistently.

---

## 11) Audit and observability

If audit is enabled (either globally or per-policy):
- emit stable `AuditEvent` records for:
  - key generate/import/destroy
  - context create/destroy
  - provider select
  - AEAD open failures (without leaking secret details)

Audit MUST NOT include plaintext, key bytes, or tags.

---

## 12) Testing strategy

### 12.1 Vector tests (required)
- AES-GCM: NIST SP 800-38D
- AES-GCM-SIV: RFC 8452
- ChaCha20-Poly1305: RFC 8439
- SHA-1/SHA-256: known digests ("", "abc") at minimum
- Ed25519/X25519: standard vectors

### 12.2 Differential tests
- software provider vs hw provider for same inputs
- one-shot vs streaming vs view variants

### 12.3 Property tests
- encrypt→decrypt roundtrip
- streaming equivalence
- iovec equivalence vs concatenated

### 12.4 Fuzzing
- malformed sizes
- invalid tags/signatures
- streaming state machine sequences

---

## 13) Benchmarks

Benches under `plugins/beryl-crypto/benches/*`:
- are **not** API
- should remain buildable
- should measure:
  - one-shot vs streaming
  - view vs copy
  - iovec vs contiguous
  - provider comparisons

Bench harnesses should use:
- monotonic timer (ns) and optional cycle counter
- warmup and stable sinks to avoid dead-code elimination

---

## 14) Build and feature configuration

Recommended feature knobs:
- enable/disable providers (software/hw/remote)
- enable/disable algorithms by family
- enable audit
- enable benches

Providers SHOULD advertise capabilities dynamically so downstream code can gate behavior.

---

## 15) Roadmap (implementation checklist)

- Provider registry enumerate/query/select using `ProviderInfo`/`AlgInfo` structs.
- Runtime handle tables: slot+generation, thread-safe.
- Views + iovec support end-to-end (no-copy where possible).
- Streaming for hash and AEAD with strict state machines.
- Policy enforcement: constant_time, sidechannel_hard, strict_nonce.
- Secure memory hooks (optional, runtime-assisted).
- Audit event channel and stable event schema.
