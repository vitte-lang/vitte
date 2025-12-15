# Security model

TODO

# Security model — beryl-crypto

This document defines the security model for the **beryl-crypto** plugin: threat model, assumptions, trust boundaries, security properties, policy knobs, side-channel posture, nonce and key lifecycle rules, audit guarantees, and expected failure behavior.

This document is **normative** for any provider that claims to implement stable surfaces under `plugins.crypto.api.*`.

- **MUST / MUST NOT / SHOULD / MAY** are used in the RFC sense.
- “Provider” means any backend implementing crypto operations for `plugins.crypto.api.*`.
- “Runtime” refers to the host environment that provides handle tables, marshalling, threading, memory utilities, and platform integration.

For API compatibility guarantees, see `docs/api_stability.md`.

---

## 1) Goals and non-goals

### 1.1 Goals
The system is designed to:
- Provide correct cryptographic results for supported algorithms.
- Prevent common catastrophic misuse (when policy requests enforcement), especially:
  - AEAD nonce reuse
  - exceeding algorithm limits
  - invalid streaming state sequences
- Provide stable and non-leaking error behavior.
- Minimize side-channel leakage **best effort** when hardening policies are enabled.
- Separate concerns between:
  - public API surface
  - internal glue
  - providers
  - runtime infrastructure

### 1.2 Non-goals
- Proving constant-time behavior across all platforms.
- Hard real-time performance guarantees.
- Defending against attackers with arbitrary code execution inside the process.
- Providing “perfect” memory secrecy (page swapping, dumps, etc.) without runtime support.

---

## 2) Threat model

### 2.1 Adversaries
The model considers:
- **Network attacker**
  - controls input ciphertexts/tags/signatures and tries to forge.
- **Local unprivileged attacker**
  - may observe timing/cache behavior externally.
  - may trigger API calls with crafted inputs.
- **Same-process adversary (limited)**
  - can call public APIs (malicious or buggy code), but cannot read arbitrary memory unless permitted by language/runtime.

### 2.2 Out-of-scope adversaries
- Kernel-level attacker.
- Hypervisor-level attacker.
- Attacker with arbitrary read/write in process memory.

---

## 3) Trust boundaries

### 3.1 Boundaries
```
Downstream code (untrusted inputs)
  ↓
plugins.crypto.api.*  (validation + policy gating)
  ↓
plugins.crypto.core.* (internal glue)
  ↓
Provider boundary     (crypto implementation)
  ↓
Runtime boundary      (handles, secure memory, marshalling, threads)
```

### 3.2 Trusted components
- The crypto provider implementation for correctness and (when claimed) side-channel properties.
- The runtime for:
  - handle safety (slot+generation)
  - memory wipe primitives
  - optional secure memory allocation
  - thread-safety primitives

### 3.3 Untrusted components
- All input data from downstream code.
- External ciphertexts/tags/signatures/keys imported from outside trust boundary.

---

## 4) Security properties

### 4.1 Correctness
Providers MUST implement algorithms per their standards:
- AES-GCM: NIST SP 800-38D
- AES-GCM-SIV: RFC 8452
- ChaCha20-Poly1305: RFC 8439
- XChaCha20-Poly1305: HChaCha20 derivation + RFC 8439 AEAD structure
- SHA-256/SHA-1: standard digest definitions
- Ed25519: standard signature behavior
- X25519: scalar clamping and correct ladder

### 4.2 Authenticity and integrity
- AEAD `open` MUST reject invalid tags.
- Signature verify MUST reject invalid signatures.
- MAC verify/compute MUST follow standard HMAC semantics if exposed.

### 4.3 Confidentiality
- AEAD `seal` MUST ensure confidentiality under the algorithm assumptions.
- For non-AEAD cipher modes (CBC/CTR), confidentiality depends on mode and correct IV/nonce use; API SHOULD require required parameters and enforce size constraints.

### 4.4 Misuse resistance (when enabled)
When strict policies are enabled, the implementation MUST attempt to prevent or detect common misuse.

---

## 5) Policy knobs and enforcement

Policies are the primary mechanism to trade performance vs security.

### 5.1 Common policy fields
Providers SHOULD expose policy structs with at least:
- `constant_time: bool`
- `sidechannel_hard: bool`
- `strict_nonce: bool` (AEAD)
- `strict_lengths: bool` (reject any non-canonical lengths)
- `limits: Limits` (msg_max, aad_max, out_max)

### 5.2 Policy interpretation
- If `constant_time` is set:
  - Provider MUST avoid secret-dependent branches/memory access where feasible.
  - Tag/signature comparisons MUST be constant-time where feasible.
- If `sidechannel_hard` is set:
  - Provider MUST apply stricter avoidance of secret-dependent behavior.
  - Provider MUST wipe temporary buffers.
- If the provider cannot satisfy a requested policy:
  - It MUST either:
    - return `Unsupported` / `InvalidPolicy`, or
    - advertise reduced guarantees via capability flags and document behavior.

---

## 6) Inputs, validation, and limits

### 6.1 General validation rules
- All APIs MUST validate:
  - key length
  - nonce/IV length
  - tag length (where configurable)
  - output buffer size (views/iovec)
  - streaming state
- Providers MUST reject invalid inputs with stable error codes.

### 6.2 Limits
Each algorithm MUST enforce defined limits:
- `msg_max`
- `aad_max` (AEAD)
- `out_max` (digest/tag/output)

Violations MUST return `MessageTooLarge` (or equivalent stable error).

### 6.3 Buffer-too-small semantics
When using view APIs with caller-allocated output:
- Providers MUST return `BufferTooSmall`.
- Providers SHOULD include:
  - `required` size, or
  - `written` bytes if partially written (only if safe and documented).

---

## 7) Nonce model (AEAD)

Nonce misuse is catastrophic for many AEAD schemes.

### 7.1 Requirements
- Providers MUST document nonce requirements per algorithm:
  - AES-GCM: typically 12 bytes (96-bit), other sizes need GHASH-derived J0
  - AES-GCM-SIV: 12 bytes (typical), per RFC 8452 processing
  - ChaCha20-Poly1305: 12 bytes (IETF)
  - XChaCha20-Poly1305: 24 bytes

### 7.2 Strict nonce policy
If `strict_nonce` is enabled:
- Provider MUST attempt to detect nonce reuse per key:
  - If detected: return `NonceReuseDetected`.
- Provider MUST attempt to detect nonce exhaustion (counter/sequence limits):
  - If exhausted: return `NonceExhausted`.

Detection strategies MAY include:
- provider-managed nonce sequence tracking
- monotonic counters stored with key/context
- probabilistic reuse detection (documented clearly)

If enforcement cannot be provided:
- Provider MUST advertise this limitation and MUST NOT claim strict enforcement.

---

## 8) Key lifecycle and sensitive memory

### 8.1 Key material handling
- Key bytes MUST be treated as secret.
- Providers MUST minimize copying of key material.
- Temporary key-related buffers MUST be wiped after use.

### 8.2 Key storage
- If runtime supports secure memory:
  - Providers SHOULD store keys in protected allocations.
- If not available:
  - Providers MUST still wipe transient buffers and internal structures on destroy.

### 8.3 Destroy semantics
- Key/context/stream destroy MUST be idempotent.
- After destroy:
  - any use MUST fail with stable errors.
  - internal state MUST be wiped.

---

## 9) Handle safety and use-after-free prevention

### 9.1 Slot + generation
- All handles MUST be validated via slot+generation.
- Destroy MUST increment generation.
- Lookup MUST be O(1) and thread-safe.

### 9.2 Kind separation
Handle namespaces MUST be disjoint:
- Provider
- Key
- Context
- Stream

Using the wrong kind MUST fail.

---

## 10) Side-channels

### 10.1 Scope
Side-channel hardening addresses:
- timing differences
- cache access patterns
- branch prediction differences

### 10.2 Best-effort statement
Without full platform control, side-channel resistance is **best effort**.
Providers MUST NOT claim stronger guarantees than they implement.

### 10.3 Constant-time comparisons
- Tags and signatures MUST be compared in constant time when feasible.
- Padding validation (CBC/PKCS7) MUST avoid secret-dependent branches where feasible.

### 10.4 Secret-independent memory access
When `sidechannel_hard` is enabled:
- Providers MUST avoid secret-indexed table lookups (e.g., AES T-tables) where feasible.
- Providers SHOULD prefer bitsliced/AES-NI/ARMv8 crypto ext implementations.

---

## 11) Streaming security

### 11.1 State machine safety
- Streaming APIs MUST implement a strict state machine:
  - `start` → `update*` → `finish`
- Invalid sequences MUST return `InvalidState`.

### 11.2 AEAD streaming
- Tag MUST only be produced at `finish`.
- `open` MUST verify tag before releasing plaintext when feasible.
  - If API requires writing plaintext during update, it MUST specify whether plaintext is provisional and must be discarded on final failure.

### 11.3 CBC streaming
- If mode requires block-multiple updates:
  - it MUST enforce it.
  - `finish` handles padding rules.

---

## 12) Provider capability claims

Providers MUST expose capability flags that reflect security properties:
- thread safety
- sandboxed execution
- constant-time support
- sidechannel_hard support
- strict nonce enforcement support
- iovec support
- streaming support

Claims MUST be accurate.

---

## 13) Remote / KMS providers

Remote providers introduce additional risks.

### 13.1 Trust model
- Remote providers MUST clearly state what secrets leave the process.
- If keys are remote-managed:
  - plaintext and ciphertext may transit to remote systems depending on design.

### 13.2 Wrap/unwrap
If provider supports key wrapping:
- Wrapped formats MUST be versioned.
- Wrap/unwrap MUST authenticate metadata and protect integrity.
- Provider SHOULD include key usage constraints inside the wrapped blob.

### 13.3 Replay/rollback
Remote providers SHOULD protect against:
- replayed wrapped keys
- rollback to old key states

---

## 14) Error behavior and information leakage

### 14.1 Stable errors
Errors MUST be stable and not leak secret data.

### 14.2 Verification failures
- Tag/signature failures MUST return a generic verification failure.
- Errors MUST NOT reveal which part failed.

### 14.3 Timing leakage
Providers SHOULD normalize timing for verification failures when feasible.

---

## 15) Audit

If audit is enabled:
- Providers SHOULD emit stable `AuditEvent` records for:
  - provider selection
  - key generate/import/destroy
  - context create/destroy
  - AEAD open failures (no sensitive payloads)

Audit MUST NOT include:
- plaintext
- key bytes
- nonces (optional; may include nonce length or hash if required)
- tags/signatures

---

## 16) Required tests

Stable providers MUST pass:
- vector tests for each implemented algorithm (see `docs/api_stability.md`)
- negative tests:
  - invalid lengths
  - buffer too small
  - invalid tags/signatures
  - invalid streaming sequences
- differential tests (where multiple providers exist)
- fuzzing of malformed inputs and streaming sequences (recommended)

---

## 17) Operational guidance (for downstream users)

### 17.1 Recommended defaults
- Prefer AEAD modes over raw cipher modes.
- Keep `strict_nonce` enabled where available.
- Keep `constant_time` enabled for verification operations.

### 17.2 Nonce handling
- Never reuse nonce with the same key.
- Prefer random nonces only when the scheme allows and policy supports safe bounds.
- Prefer deterministic counters with persistent storage if using long-lived keys.

### 17.3 Key rotation
- Rotate keys before reaching message limits.
- Use provider limits and returned errors to drive rotation.

---

## 18) Change control

Any change that affects security properties MUST:
- update this document
- update tests
- document impact in release notes