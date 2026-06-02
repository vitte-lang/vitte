# tests

Path: `toolchain/tests`

## Purpose

Bootstrap and build toolchain for staged compiler self-hosting.

## Notes

- Preserve reproducibility constraints.
- Keep stage contracts and verification scripts in sync.
- Avoid compatibility shims that bypass core compiler flow.

# toolchain/tests

Path: `toolchain/tests`

## Purpose

Validation and verification infrastructure for the Vitte compiler toolchain.

This directory contains staged compiler tests, bootstrap verification logic, reproducibility checks, compiler validation workflows, and self-hosting reliability infrastructure.

The goal is to guarantee deterministic compiler behavior across bootstrap stages and toolchain generations.

## Responsibilities

The test infrastructure focuses on:

- staged compiler verification,
- bootstrap integrity validation,
- compiler reproducibility checks,
- self-hosting validation,
- regression detection,
- diagnostics verification,
- parser and semantic consistency,
- backend validation,
- runtime compatibility checks.

## Testing philosophy

The toolchain test system emphasizes:

- deterministic execution,
- reproducible outputs,
- explicit validation stages,
- stable compiler transitions,
- minimal hidden behavior,
- strict verification workflows.

The objective is to maintain confidence across all compiler generations and bootstrap stages.

## Bootstrap validation

Bootstrap validation includes:

- stage-to-stage compiler verification,
- generated artifact consistency,
- compiler output reproducibility,
- semantic equivalence validation,
- parser stability checks,
- diagnostics consistency.

## Test categories

Typical validation areas include:

- lexer tests,
- parser tests,
- AST/HIR/MIR validation,
- semantic analysis tests,
- diagnostics tests,
- borrow checking validation,
- backend verification,
- runtime integration tests,
- bootstrap regression tests.

## Reproducibility goals

The toolchain infrastructure aims to ensure:

- deterministic builds,
- stable compiler outputs,
- reproducible bootstrap stages,
- predictable diagnostics,
- architecture consistency,
- long-term compiler reliability.

## Design principles

- Explicit verification over hidden behavior
- Reproducibility over convenience
- Deterministic compiler evolution
- Stable bootstrap transitions
- Maintainable validation infrastructure
- Progressive compiler hardening

## Repository role

This directory is part of the larger Vitte compiler validation ecosystem and contributes to long-term compiler stability, self-hosting reliability, and toolchain consistency.
