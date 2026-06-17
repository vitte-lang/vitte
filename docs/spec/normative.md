# Vitte Normative Specification Skeleton

This document defines the required normative sections for stabilizing Vitte beyond the current bootstrap subset. Each section is intentionally concise for now, but it is binding: implementation, diagnostics, fixtures, and generated documentation must converge here instead of inventing incompatible rules.

## Memory Model

Vitte memory semantics are ownership-first. Moves invalidate the previous owner, shared and mutable aliases are mutually exclusive, and references must not outlive their owning region. Unsafe code may extend the expressible surface, but it does not erase diagnostics at safe boundaries.

## Modules And Imports

Modules are named by canonical `space` paths. Imports must resolve deterministically, cycles are rejected unless an explicitly documented future phase supports them, and public surfaces must be exported intentionally.

## ABI

ABI declarations must specify calling convention, layout expectations, target family, and symbol visibility. A backend may reject an ABI profile if layout, pointer width, object format, or linker contract is not known.

## Unsafe

Unsafe operations are lexically explicit and must remain auditable. The checker must track escape paths from unsafe values back into safe code and attach diagnostics to the boundary where the guarantee is lost.

## Traits And Generics

Trait implementations must be coherent for a given type and trait pair. Generic constraints must be resolved before codegen. Associated types are part of the trait contract and must be normalized consistently during type checking.

## Effects And Async

Effects describe observable capabilities such as IO, allocation, concurrency, and unsafe interaction. Async suspension points must preserve borrow and lifetime invariants across the suspended region.

## Macros And Comptime

Macros and comptime execution must be deterministic for a given input, compiler version, feature set, and target profile. Expansion must preserve source mapping for diagnostics.

## Version Compatibility

Language editions define compatibility boundaries. Stable features require parser, semantic checks, diagnostics, codegen or explicit rejection, tests, and documentation. Experimental features must be marked as such.
