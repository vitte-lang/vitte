# Security Policy

This directory contains deterministic fixture generators used for benchmarking and fuzzing the Vitte compiler/tooling.

## Supported Versions

Security fixes are applied on the `main` branch. Tagged releases inherit fixes via backports when applicable.

| Component | Supported |
|---|---|
| `compiler/bench/src/data/generators` | ✅ `main` |

## Reporting a Vulnerability

If you believe you have found a security vulnerability:

1. **Do not** open a public issue.
2. Provide a minimal reproduction:
   - generator name / script
   - command line used
   - input(s) and output(s)
   - OS + Python version
3. Prefer reporting privately via the repository security advisories.

## Threat Model

These scripts:
- execute locally (developer machines / CI)
- parse input files and write generated fixtures
- are not designed to process untrusted inputs as a network service

### Primary risks

- **Path traversal / overwrite** when output paths are derived from untrusted data.
- **Resource exhaustion** (extremely large inputs, pathological unicode, huge directory trees).
- **Log/UI spoofing** via BiDi controls, ZWJ sequences, and confusables.

## Hardening Guidelines

- Output must be constrained under `--out` using atomic staging writes.
- Avoid interpreting escapes in extracted string literals unless required.
- Treat BiDi controls as data; avoid printing raw strings to terminals without escaping.
- Prefer deterministic RNG derived from `--seed` only.

## Disclosure Timeline

We aim to acknowledge reports within 7 days and provide a fix or mitigation as soon as practical.
