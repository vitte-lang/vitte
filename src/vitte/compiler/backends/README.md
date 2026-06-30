# Vitte Compiler Backends

Path: src/vitte/compiler/backends

## Purpose

This directory currently hosts backend adapters, experimental families, and
support surfaces that sit alongside the canonical production backend tree in
`src/vitte/compiler/backend/*`.

It is not the single production source of truth for the runtime compiler path.

## Current Role

`src/vitte/compiler/backends/*` currently contains:

- `llvm_bindings`
  - LLVM toolchain integration and object emission helpers used by the canonical
    backend pipeline
- `vitte_emit`
  - adapter surface that explicitly declares
    `canonical=src/vitte/compiler/backend/*`
- `wasm`
  - dedicated WASM-oriented backend family and docs
- legacy or parallel emit-oriented helper modules such as `c_emit` and
  `llvm_emit`

`llvm_emit` is a legacy experimental surface. It is not the production LLVM
backend and must not be presented as the canonical codegen route.

## Contract

The canonical production compiler path continues to route through
`src/vitte/compiler/backend/*`.

Anything under `src/vitte/compiler/backends/*` must therefore be treated as one
of:

- adapter
- support layer
- experimental family
- migration surface not yet wired as the sole backend implementation

This layer is responsible for:
- target configuration
- ABI selection
- code generation
- instruction lowering
- object generation
- linker integration
- executable artifact production

This layer must not own:
- parsing
- semantic analysis
- borrow checking
- frontend diagnostics
- HIR construction

The backend consumes validated MIR and produces executable artifacts.

## Canonical Pipeline

MIR
→ target lowering
→ backend IR
→ machine lowering
→ object generation
→ linker
→ executable / library

## Responsibilities

The backend layer owns:
- target triples
- endianness
- calling conventions
- register allocation
- object file format selection
- runtime payload integration
- relocation generation
- symbol export policy

The backend must guarantee deterministic artifact generation for identical inputs.

## Invariants

- backend execution must be deterministic
- target configuration must be explicit
- ABI selection must never be implicit
- object format must match target contracts
- linker invocations must be reproducible
- relocation generation must be validated
- runtime payload compatibility must be checked
- unsupported targets must fail explicitly

## Critical Rule

Silent backend fallback is forbidden.

Silent ownership transfer is also forbidden:

- do not move the real compiler path from `backend/*` to `backends/*` without a
  matching gate update
- do not present adapter surfaces as canonical implementation surfaces
- do not let support modules become runtime-critical without explicit audit

Invalid:
- switching architecture automatically
- changing ABI silently
- replacing unsupported object formats
- silently disabling relocations
- auto-selecting another linker without diagnostics

Backend failures must produce explicit diagnostics.

## Recommended Structure

backends/
├── mod.vit
├── target.vit
├── abi.vit
├── endian.vit
├── object_format.vit
├── codegen/
├── lowering/
├── machine/
├── object/
├── linker/
├── runtime/
└── README.md

## Target Contracts

Each backend target must define:
- architecture
- pointer width
- endianness
- ABI
- object format
- relocation model
- calling convention
- alignment requirements

Example:

Target:
- x86_64-linux-gnu

Contracts:
- little endian
- ELF64
- System V ABI
- LP64 data model

## ABI Contracts

The ABI layer defines:
- parameter passing
- return value layout
- stack alignment
- register preservation
- variadic handling
- aggregate passing rules

ABI mismatches must fail validation.

## Object Format Contracts

Supported formats may include:
- ELF
- COFF
- PE
- Mach-O

Object generation must validate:
- section alignment
- symbol tables
- relocation tables
- export visibility
- debug sections

## Linker Contracts

The linker layer owns:
- runtime startup integration
- static linking
- dynamic linking
- runtime object inclusion
- library resolution
- final executable layout

Link failures must remain reproducible.

## Runtime Payload Validation

Runtime integration must validate:
- architecture compatibility
- ABI compatibility
- symbol availability
- startup entrypoints
- runtime initialization ordering

Invalid runtime payloads must fail before executable generation.

## Reproducible Commands

Build native executable:

vittec build app.vit

Emit object file:

vittec build app.vit --emit-object

Emit assembly:

vittec build app.vit --emit-asm

Cross compile:

vittec build kernel.vit --target x86_64-linux-gnu

Build shared library:

vittec build runtime.vit --shared

Static link:

vittec build runtime.vit --static

## Testing Strategy

Recommended tests:
- deterministic object snapshots
- ABI validation
- relocation verification
- linker reproducibility
- runtime payload validation
- cross-target consistency
- section layout verification
- symbol export validation
- executable startup tests

## Backend Diagnostics

Backend diagnostics should include:
- unsupported target
- ABI mismatch
- relocation overflow
- invalid runtime payload
- linker failure
- missing symbols
- unsupported instruction lowering
- invalid alignment

Examples:
- BACKEND_E_UNSUPPORTED_TARGET
- ABI_E_INVALID_CALL_CONV
- LINK_E_MISSING_SYMBOL
- OBJECT_E_INVALID_RELOCATION

## Design Philosophy

The backend is a strict lowering and artifact generation system.

It should remain:
- deterministic
- target-explicit
- ABI-safe
- reproducible
- validation-heavy
- linker-aware
- runtime-aware

The backend must never guess critical target behavior.
