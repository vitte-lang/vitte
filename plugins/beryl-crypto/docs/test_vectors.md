# Test vectors

TODO

# Test vectors — beryl-crypto

This document is the canonical inventory of **test vectors** required for stable algorithm implementations in **beryl-crypto**.

- It is a checklist + format guide used by the test suite.
- It is not meant to embed large vector blobs inline. Prefer storing large vector sets as compact files under `tests/vectors/`.

See also:
- `docs/api_stability.md` (compatibility contract)
- `docs/security_model.md` (security requirements)

---

## 1) Conventions

### 1.1 Naming
Vector sets MUST be named:

- `<family>/<alg>/<source>/<kind>`

Examples:
- `aead/aes_gcm/nist/kat`
- `aead/aes_gcm_siv/rfc8452/kat`
- `aead/chacha20_poly1305/rfc8439/kat`
- `hash/sha256/nist/kat`
- `hash/sha1/fips/kat`
- `sign/ed25519/rfc8032/kat`
- `kex/x25519/rfc7748/kat`

### 1.2 Encoding
Vector files MUST use one of:
- JSON (preferred): stable, easy parsing
- TOML (allowed): if JSON parser isn’t available
- Raw hex pairs in `.vec` text files (fallback)

Binary fields MUST be hex-encoded lowercase without separators.

### 1.3 Determinism
Vector-driven tests MUST be deterministic:
- no RNG
- stable ordering
- stable error expectations

### 1.4 API coverage
For each algorithm, vectors MUST be exercised across:
- one-shot API
- view API (ByteSlice/MutByteSlice)
- iovec API (IoVec) when supported
- streaming API (start/update/finish) when supported

---

## 2) Directory layout

Recommended layout:

```
plugins/beryl-crypto/
  docs/
    test_vectors.md
  tests/
    vectors/
      aead/
        aes_gcm/
          nist_sp800_38d_kat.json
        aes_gcm_siv/
          rfc8452_kat.json
        chacha20_poly1305/
          rfc8439_kat.json
        xchacha20_poly1305/
          draft_kat.json
      hash/
        sha1/
          fips_180_4_kat.json
        sha256/
          fips_180_4_kat.json
      sign/
        ed25519/
          rfc8032_kat.json
      kex/
        x25519/
          rfc7748_kat.json
      cipher/
        aes/
          nist_sp800_38a_kat.json
        aes_xts/
          nist_sp800_38e_kat.json
```

The exact filenames MAY differ, but stable tests SHOULD reference them through a single manifest.

---

## 3) Vector schemas

### 3.1 AEAD common schema
Applies to:
- AES-GCM
- AES-GCM-SIV
- ChaCha20-Poly1305
- XChaCha20-Poly1305

Fields:
- `name`: string
- `key`: hex
- `nonce`: hex
- `aad`: hex
- `pt`: hex
- `ct`: hex
- `tag`: hex
- `expect`: `ok|invalid_tag|invalid_nonce|unsupported`

Optional:
- `tag_len`: integer (bytes)
- `notes`: string

Example:
```json
{
  "name": "gcm_1",
  "key": "00000000000000000000000000000000",
  "nonce": "000000000000000000000000",
  "aad": "",
  "pt": "",
  "ct": "",
  "tag": "58e2fccefa7e3061367f1d57a4e7455a",
  "expect": "ok"
}
```

### 3.2 Hash common schema
Applies to:
- SHA-1
- SHA-256
- BLAKE3 (fixed output)

Fields:
- `name`: string
- `msg`: hex
- `digest`: hex
- `expect`: `ok|unsupported`

Optional:
- `out_len`: integer (bytes) for XOF-like algorithms

Example:
```json
{
  "name": "sha256_abc",
  "msg": "616263",
  "digest": "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
  "expect": "ok"
}
```

### 3.3 HMAC schema
Fields:
- `name`: string
- `key`: hex
- `msg`: hex
- `tag`: hex
- `expect`: `ok|unsupported`

### 3.4 Ed25519 schema
Fields:
- `name`: string
- `sk`: hex (seed or expanded key; specify)
- `pk`: hex
- `msg`: hex
- `sig`: hex
- `expect`: `ok|invalid_sig|unsupported`

Example:
```json
{
  "name": "ed25519_1",
  "sk": "9d61b19deffd5a60ba844af492ec2cc4...",
  "pk": "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a",
  "msg": "",
  "sig": "e5564300c360ac729086e2cc806e828a...",
  "expect": "ok"
}
```

### 3.5 X25519 schema
Fields:
- `name`: string
- `scalar`: hex (32 bytes)
- `u`: hex (32 bytes)
- `out`: hex (32 bytes)
- `expect`: `ok|unsupported`

Example:
```json
{
  "name": "x25519_1",
  "scalar": "a546e36bf0527c9d3b16154b82465edd62144c0ac1fc5a18506a2244ba449ac4",
  "u": "e6db6867583030db3594c1a424b15f7c726624ec26b3353b10a903a6d0ab1c4c",
  "out": "c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552",
  "expect": "ok"
}
```

---

## 4) Required vector sources

### 4.1 AES
- AES block and modes:
  - NIST SP 800-38A (ECB/CBC/CTR)
  - NIST SP 800-38E (XTS)
- AES-GCM:
  - NIST SP 800-38D (KAT)

### 4.2 AES-GCM-SIV
- RFC 8452 (test vectors)

### 4.3 ChaCha20-Poly1305
- RFC 8439 (test vectors)

### 4.4 XChaCha20-Poly1305
- Draft / widely used libsodium-compatible vectors
  - MUST be pinned to a specific revision/source in the repository metadata.

### 4.5 SHA
- SHA-1/SHA-256:
  - FIPS 180-4 known answer tests
  - At minimum include:
    - msg="" and msg="abc"

### 4.6 Ed25519
- RFC 8032 test vectors

### 4.7 X25519
- RFC 7748 test vectors

---

## 5) Negative vectors

Stable providers MUST include negative vectors to validate error handling:

### 5.1 AEAD invalid tag
For each AEAD algorithm:
- take a valid vector and flip 1 bit of tag
- expect `invalid_tag`

### 5.2 AEAD invalid nonce length
- provide nonce of wrong length
- expect `invalid_nonce` or `invalid_input` (documented)

### 5.3 Signature invalid
- flip 1 bit of signature
- expect `invalid_sig`

### 5.4 Buffer-too-small
View APIs MUST be tested with output buffers smaller than required:
- expect `buffer_too_small`
- verify `required` when available

### 5.5 Streaming invalid sequences
Streaming APIs MUST be tested for:
- update before start
- update after finish
- finish twice (should be idempotent or stable error)

---

## 6) Cross-API equivalence tests

For each algorithm implementation, the test suite SHOULD assert:

- **one-shot** == **streaming**
  - split message into random chunk boundaries
- **contiguous** == **iovec**
  - split input into multiple segments
- **copy** == **view**
  - ensure view output matches owned output

For AEAD:
- seal/open roundtrip across all API variants

---

## 7) Minimal required vectors per algorithm

If a provider implements an algorithm as **Stable**, the minimum set is:

### 7.1 AEAD (each of AES-GCM, AES-GCM-SIV, ChaCha20-Poly1305, XChaCha20-Poly1305)
- 10 valid vectors across different sizes:
  - pt sizes: 0, 1, 16, 31, 32, 128, 1024
  - aad sizes: 0, 1, 16, 64
- 10 invalid-tag vectors
- 5 invalid-nonce vectors

### 7.2 SHA-256 / SHA-1
- 10 valid vectors:
  - include empty and "abc"
  - include 1KiB and 1MiB messages (can be derived/pseudo-generated deterministically)

### 7.3 Ed25519
- 10 vectors:
  - include empty and non-empty messages
- 10 invalid signature vectors

### 7.4 X25519
- 10 vectors:
  - include RFC 7748 vectors

---

## 8) Vector manifest (recommended)

To avoid hardcoding filenames in tests, define a single manifest file:

- `tests/vectors/manifest.json`

Example shape:
```json
{
  "aead": {
    "aes_gcm": "aead/aes_gcm/nist_sp800_38d_kat.json",
    "aes_gcm_siv": "aead/aes_gcm_siv/rfc8452_kat.json",
    "chacha20_poly1305": "aead/chacha20_poly1305/rfc8439_kat.json",
    "xchacha20_poly1305": "aead/xchacha20_poly1305/draft_kat.json"
  },
  "hash": {
    "sha1": "hash/sha1/fips_180_4_kat.json",
    "sha256": "hash/sha256/fips_180_4_kat.json"
  },
  "sign": {
    "ed25519": "sign/ed25519/rfc8032_kat.json"
  },
  "kex": {
    "x25519": "kex/x25519/rfc7748_kat.json"
  },
  "cipher": {
    "aes": "cipher/aes/nist_sp800_38a_kat.json",
    "aes_xts": "cipher/aes_xts/nist_sp800_38e_kat.json"
  }
}
```

---

## 9) Implementation notes for the test suite

### 9.1 Parsing
- Prefer JSON parser.
- Keep hex decode strict:
  - lowercase
  - even length
  - no separators

### 9.2 Provider selection
Tests SHOULD run per provider:
- enumerate providers
- filter by algorithm support
- run vector sets for each supported provider

### 9.3 Policy matrix
Tests SHOULD run with at least:
- `constant_time=false, sidechannel_hard=false` (baseline)
- `constant_time=true, sidechannel_hard=true` (hardened)

If a provider cannot satisfy hardened policy:
- it MUST return `unsupported/invalid_policy` and test MUST accept that outcome.

---

## 10) TODO checklist

- Add `tests/vectors/manifest.json`.
- Add vector files under `tests/vectors/*` (seeded from official sources).
- Add negative vector generation helper (flip-bit tag/sig).
- Add streaming random chunk splitter (seeded RNG for determinism).