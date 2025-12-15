# API stability

TODO

# API stability — beryl-crypto

This document defines the stability contract for the **beryl-crypto** plugin surface: public modules under `plugins.crypto.api.*`, their ABIs, error semantics, handle model, provider registry behavior, and compatibility guarantees.

It is written as a **normative spec**:
- **MUST / MUST NOT / SHOULD / MAY** are used in the RFC sense.
- “Public API” means anything under `plugins.crypto.api.*` that is intended for downstream code.

---

## 1) Scope

### In scope
- Algorithm families: AEAD (AES-GCM, AES-GCM-SIV, ChaCha20-Poly1305, XChaCha20-Poly1305), hashes (SHA-* / BLAKE3), signatures (Ed25519), key exchange (X25519), MACs (HMAC-*), RNG.
- Provider registry: enumerate/query/select, capabilities, priority policy.
- Handle model: slot+generation, handle kind separation.
- Byte views and I/O vectors: `ByteSlice`, `MutByteSlice`, `IoVec`, `MutIoVec`.
- Streaming APIs: start/update/finish and state machine rules.
- Error codes and stability of failure modes.
- ABI marshalling boundaries (runtime → plugin).

### Out of scope
- Concrete crypto implementations (hardware acceleration, constant-time techniques).
- Performance guarantees.
- Internal modules not exported for downstream use.

---

## 2) Stability levels

Each symbol exported by `plugins.crypto.api.*` is classified into a stability level.

### 2.1 Levels
- **Stable**
  - Backward compatible for the entire major line.
  - Breaking changes only at a major version bump.
- **Experimental**
  - Signature and semantics may change in any release.
  - Intended for early adopters.
- **Internal**
  - Not part of the public contract.
  - May change without notice.

### 2.2 How a symbol is marked
- Modules, types, functions SHOULD be documented with an `@stability` tag in doc comments (or adjacent docs) indicating `stable|experimental|internal`.
- If no marker exists:
  - `plugins.crypto.api.*` defaults to **Stable**.
  - Anything outside `plugins.crypto.api.*` defaults to **Internal**.

---

## 3) Versioning policy

### 3.1 Semantic versioning
The plugin adheres to SemVer:
- `MAJOR.MINOR.PATCH`
  - **PATCH**: bug fixes, performance improvements, documentation updates; no behavior changes beyond spec.
  - **MINOR**: backward compatible feature additions (new types, new functions, new enum variants **only if forward-compatible rule is satisfied**, see below).
  - **MAJOR**: any breaking change.

### 3.2 Forward-compat rule for enums
Adding enum variants can break exhaustive matches.

Therefore:
- Any public enum intended for downstream `match` **MUST** include an `Unknown`/`Other`/`Reserved(u32)` variant, or be documented as **non-exhaustive**.
- If neither is present, adding variants is a **major** change.

### 3.3 Feature flags and capability discovery
New capabilities **MUST** be discoverable via provider `AlgInfo`/`ProviderInfo` flags (or equivalent) before use.
Downstream code SHOULD gate new features on capability checks rather than version strings.

---

## 4) Public surface layout

### 4.1 Module naming and paths
- Public API lives under `plugins.crypto.api.*`.
- Paths are considered stable. Renaming modules is a breaking change.

### 4.2 Type stability
- Struct field order and presence:
  - If structs are passed across an ABI boundary (marshalled), their layout MUST be treated as part of ABI and versioned accordingly.
  - If purely in-language, fields MAY be extended only if downstream code does not construct them positionally.

### 4.3 Request/response objects
Where possible, APIs SHOULD use request structs to allow future extension without breaking signatures.

---

## 5) ABI / marshalling contract

### 5.1 ABI stability boundary
If the runtime supports struct marshalling:
- ABI outputs MUST prefer **struct outputs** (`*_Info` structs, `*_Result` structs) over ad-hoc text blobs.
- Text outputs MAY remain for diagnostics/logging only and MUST NOT be required for correct program behavior.

### 5.2 Evolution rules
- Adding new fields to ABI structs MUST:
  - either be appended with a `size` field / version tag,
  - or be guarded by a `struct_size` negotiated at runtime.
- Removing or retyping fields is a major change.

---

## 6) Error model

### 6.1 Stable error taxonomy
Errors are part of the contract. At minimum, these categories MUST exist (names may differ, semantics must match):
- `Ok`
- `Unsupported` (feature not available)
- `InvalidInput` / `InvalidOutput` (shape/size violations)
- `BufferTooSmall` (with written/required where applicable)
- `InvalidKey` / `InvalidNonce` / `InvalidIv` / `InvalidTag` / `InvalidSignature` (algorithm-specific)
- `NonceReuseDetected` / `NonceExhausted` (AEAD nonce policy)
- `MessageTooLarge` (limits exceeded)
- `InvalidState` (streaming state machine violations)
- `ProviderUnavailable` / `KeyUnavailable` (handle invalid, destroyed, provider removed)
- `InternalError` (unexpected failure)

### 6.2 Determinism and stability
- For the same inputs and same provider/flags, errors SHOULD be deterministic.
- Security-related failure messages MUST NOT leak secrets.
- Verification failures MUST return stable failures (no timing side-channels beyond best effort).

### 6.3 Constant-time compare
- Tag/signature comparisons MUST be constant-time where feasible.
- If a provider cannot guarantee CT, it MUST advertise this via capability flags.

---

## 7) Handle model (slot + generation)

### 7.1 Overview
Handles identify provider instances, keys, contexts, and streams.

### 7.2 Requirements
- Handles MUST be validated with **slot + generation**.
- Lookup SHOULD be O(1).
- Destroy MUST be idempotent.
- Use-after-destroy MUST be detectable and return a stable error.

### 7.3 Namespaces
Handle kinds MUST be disjoint:
- Provider handle
- Key handle
- Context handle
- Stream handle

A handle of the wrong kind MUST be rejected.

---

## 8) Providers and registry

### 8.1 Enumerate / query / select
Providers MUST support:
- `enumerate`: list providers
- `query`: obtain `ProviderInfo` + list of algorithms (or query per algorithm)
- `select`: choose provider for subsequent operations

### 8.2 Selection policy
Selection MUST be explicit or deterministic:
- If user specifies provider id: choose it or return `ProviderUnavailable`.
- If user specifies `default`: follow a deterministic priority order.

Suggested priority order (informative):
1. Hardware-backed provider (AES-NI/ARMv8 crypto ext, CLMUL/PMULL)
2. Verified constant-time software provider
3. Remote/KMS provider
4. Fallback software provider

### 8.3 Capability flags
Providers MUST expose capability flags, including:
- thread safety
- sandboxed
- constant-time guarantees
- streaming support
- iovec support
- nonce policy enforcement

---

## 9) Byte views and I/O vectors

### 9.1 Views
`ByteSlice` / `MutByteSlice` are zero-copy views.

Rules:
- A view MUST point to valid memory for the duration of the call.
- Output views MUST have clearly defined ownership:
  - Either caller-allocated output buffers (preferred), or
  - provider-managed buffers with explicit lifetime rules.

### 9.2 IoVec
Scatter/gather inputs MUST behave as if concatenated in order.
For outputs, providers MAY require a minimum segment size; if so they MUST document it and return `BufferTooSmall`.

---

## 10) Streaming APIs

### 10.1 State machine
Streaming MUST implement a strict state machine:
- `start` → zero or more `update` → `finish`

Rules:
- `finish` SHOULD be idempotent.
- `destroy` MUST be safe at any state.
- Calling `update` after `finish` MUST return `InvalidState`.

### 10.2 Chunking
- Hash streaming MUST accept arbitrary chunk sizes.
- AEAD streaming SHOULD accept arbitrary chunk sizes; if chunking constraints exist, provider MUST advertise them.
- CBC streaming constraints (block-multiple updates) MUST be enforced and documented.

---

## 11) Limits and policy enforcement

### 11.1 Limits
Algorithms MUST define `Limits` (message max, aad max, output max) and enforce them.
Overflow/limit violations MUST return `MessageTooLarge`.

### 11.2 Nonce policy for AEAD
If `strict_nonce` is enabled:
- Nonce reuse MUST be detected best-effort and return `NonceReuseDetected`.
- Nonce exhaustion MUST be detected and return `NonceExhausted`.

If provider cannot enforce, it MUST advertise limitation.

---

## 12) Security hardening requirements

### 12.1 Side-channel hardening flags
If policy includes `sidechannel_hard` or `constant_time`:
- Providers MUST avoid secret-dependent branches/memory access where feasible.
- Providers MUST wipe temporary buffers (`memwipe`) after use.

### 12.2 Memory hygiene
- Key material MUST be treated as sensitive.
- Providers SHOULD store keys in protected memory when possible (mlock/guard pages) but this is not required.
- Destroying key/context/stream MUST wipe internal state.

### 12.3 Audit
If audit is enabled:
- Providers SHOULD emit stable `AuditEvent` records for:
  - key import/generate/destroy
  - context create/destroy
  - AEAD seal/open failures (without leaking secrets)

---

## 13) Algorithm-specific stability notes

### 13.1 AES-GCM
- Tag length MUST default to 16 bytes unless configured.
- NIST SP 800-38D test vectors MUST pass.

### 13.2 AES-GCM-SIV
- RFC 8452 test vectors MUST pass.
- POLYVAL implementation MUST be constant-time where feasible.

### 13.3 ChaCha20-Poly1305 / XChaCha20-Poly1305
- RFC 8439 vectors MUST pass.
- XChaCha20 uses 24-byte nonce; HChaCha20 derivation MUST match known vectors.

### 13.4 BLAKE3
- Fixed digest default 32 bytes.
- XOF output sizes MUST be supported if exposed.

### 13.5 Ed25519
- Verification MUST be constant-time best-effort.
- Deterministic signature behavior MUST match the standard.

### 13.6 X25519
- Scalars MUST be clamped.
- Fixed-base and variable-base APIs MAY exist; if absent, generic shared-secret MUST work.

---

## 14) Test compatibility contract

### 14.1 Required vectors
For Stable APIs, the following MUST be covered by tests:
- AES: NIST SP 800-38A/38D/38E where applicable
- ChaCha20-Poly1305: RFC 8439
- AES-GCM-SIV: RFC 8452
- SHA-1/SHA-256: known digests for "" and "abc" at minimum
- Ed25519/X25519: standard test vectors

### 14.2 Fuzzing
Stable surfaces SHOULD be fuzzed for:
- malformed inputs (sizes, invalid tags, invalid signatures)
- streaming state machine sequences

---

## 15) Deprecation policy

### 15.1 Deprecation window
- Stable APIs SHOULD not be removed without at least one minor release where they are marked deprecated.
- Deprecated APIs MUST remain available until the next major release.

### 15.2 Deprecation mechanism
- Deprecation SHOULD be indicated in docs and (if supported) via compiler attributes.
- Replacements MUST be documented.

---

## 16) Backward compatibility matrix

A release MUST document compatibility across:
- Vitte compiler version
- runtime marshalling support level
- supported providers (software/hw/remote)

Recommended format:
- `vittec` version range
- runtime ABI level (struct marshalling: yes/no)
- provider list with capabilities

---

## 17) Benchmarks and performance expectations

Benchmarks under `plugins/beryl-crypto/benches/*`:
- MUST NOT be treated as API.
- MAY change freely.
- SHOULD remain buildable on supported targets.

---

## 18) Change control and review

Any change to Stable API must include:
- update to this document (if semantics change)
- tests demonstrating compatibility
- migration notes for downstream code

---

## 19) Glossary

- **Provider**: backend implementation of a crypto algorithm set.
- **Policy**: flags controlling hardening/limits/nonce behavior.
- **View**: borrowed slice passed across the API without copying.
- **IoVec**: scatter/gather list of slices.
- **Slot+generation**: handle validation technique preventing stale references.