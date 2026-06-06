# Concurrency Memory Model (Foundation)

Date: May 22, 2026

This document defines the compiler-level foundation for concurrent execution.

## Scope

- Applies to threaded code paths and async runtime interaction points.
- Defines minimum guarantees required by analysis, MIR lowering, and backend codegen.
- Serves as the contract for future `Send/Sync`-like checks and atomic lowering.

## 1) Atomics

- Atomic operations are indivisible with respect to other atomic operations on the same location.
- Supported ordering classes (foundation contract):
  - `Relaxed`
  - `Acquire`
  - `Release`
  - `AcqRel`
  - `SeqCst`
- Mixed atomic and non-atomic access to the same location without synchronization is undefined behavior in the language model.

## 2) Synchronization

- Synchronization primitives establish happens-before edges.
- Minimum synchronization sources:
  - lock/unlock pairs
  - thread spawn/join boundaries
  - channel send/receive handoff
  - release/acquire atomic pairs
- Compiler transformations must preserve these edges.

## 3) Thread Visibility

- Writes that happen-before a synchronization event become visible to threads that synchronize-after that event.
- Without happens-before, visibility is not guaranteed.
- Optimizations are allowed to reorder non-synchronizing operations if observable behavior under this model is preserved.

## 4) Ordering Rules

- `Relaxed`: atomicity only; no ordering/visibility guarantees beyond per-location coherence.
- `Acquire`: prevents later reads/writes from moving before the acquire.
- `Release`: prevents earlier reads/writes from moving after the release.
- `AcqRel`: combines acquire and release semantics for read-modify-write operations.
- `SeqCst`: global total order for seq-cst operations, in addition to acquire/release constraints.

## 5) Data Race Guarantees

- Programs accepted under safe concurrency rules must be data-race free by construction.
- Data race definition:
  - two concurrent accesses to the same location,
  - at least one write,
  - no synchronization/happens-before edge.
- Unsafe/FFI blocks may violate this and must be diagnosed/tracked separately.

## Compiler Integration Notes

- HIR/MIR passes may introduce suspension/resume edges for async/coroutines.
- These edges are control-flow constructs and do not, by themselves, imply cross-thread synchronization.
- Backend lowering must map atomic orderings to target-specific instructions/barriers that satisfy this contract.
