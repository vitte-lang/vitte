# Compatibility Policy

This document explains how to read compatibility expectations across Vitte.

Vitte is experimental overall, so compatibility must be described narrowly and explicitly.

## Core Rule

Compatibility only exists where the project documents it clearly.

If a surface is implemented but not covered by an explicit compatibility statement, users should assume churn is allowed.

## Compatibility Domains

### Language Core

The strongest compatibility promise currently lives in the documented language core.

Primary references:

- `docs/LANGUAGE_CORE_COMPATIBILITY.md`
- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `docs/LANGUAGE_CORE_SURFACE.md`

### CLI Commands

The main documented command path is:

- `vitte parse`
- `vitte check`
- `vitte build`

These commands are public project surface.
That does not mean every flag or output detail is frozen.

Default rule:

- command existence and core documented behavior are higher-signal promises
- niche flags, formatting details, and incidental wording should not be treated as long-term compatibility guarantees unless documented as such

### Diagnostics

Diagnostics matter, but not all diagnostic details are equally protected.

Default rule:

- protected diagnostic sets explicitly covered by gates should be treated as compatibility-sensitive
- incidental wording outside protected sets may change as the project improves clarity

### Packages

Documented package families are public surface, but not automatically stable.

Use:

- `docs/PACKAGE_MATURITY.md`
- `docs/PUBLIC_SURFACE.md`

to determine whether a package family is merely visible or actually promoted toward stronger compatibility expectations.

### Compiler Internals And Generated Artifacts

The following are not public compatibility promises unless separately documented:

- compiler internal file layout
- internal IR structures
- internal package modules
- backend implementation details
- generated artifact shapes beyond explicit documented contracts

## Change Rules

When changing a surface with compatibility expectations:

1. update the docs in the same PR
2. update or add the gate that protects the behavior
3. state whether the change is protected, experimental, or internal
4. avoid release notes that imply broader compatibility than the docs support

## Safe Default

If maintainers are unsure whether a change is compatibility-sensitive, the safe default is:

- treat it as compatibility-sensitive if it affects documented protected behavior
- otherwise classify it explicitly as experimental and document the limit
