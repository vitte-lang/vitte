# crypto

Path: `src/vitte/stdlib/crypto`

## Purpose

Hashing, HMAC, randomness, key derivation, symmetric primitives, and asymmetric primitives.

## Architecture Role

Use `crypto` when integrity, secrecy, or key management is the feature. This family should never be presented as generic formatting or utility code.

## Main Responsibilities

- Own cryptographic transformations and their vocabulary.
- Keep randomness and hashing distinct from casual helper logic.
- Document where security-sensitive flows begin and end.

## Module Inventory

- `asymmetric.vitl`
- `hash.vitl`
- `hashing.vitl`
- `hmac.vitl`
- `keyderivation.vitl`
- `random.vitl`
- `symmetric.vitl`
- `utils.vitl`

## Complete Integration Story

- A package manifest can be serialized first, then hashed, then optionally signed.
- A token flow can derive a key in one boundary and use it in another without mixing concerns.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
