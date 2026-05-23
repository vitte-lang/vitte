# Vitte WASM Backend

Path: src/vitte/compiler/backends/wasm

## Purpose

The wasm backend owns WebAssembly target lowering and executable module generation.

This backend is responsible for:
- WASM target configuration
- MIR lowering to WASM operations
- linear memory integration
- WASM ABI handling
- module section generation
- import/export binding generation
- runtime integration
- wasm artifact emission

This backend must not own:
- semantic analysis
- type checking
- borrow checking
- frontend parsing
- generic MIR construction

The wasm backend consumes validated MIR and produces deterministic WebAssembly modules.

## Canonical Pipeline

MIR
→ wasm lowering
→ wasm IR
→ section generation
→ relocation/import resolution
→ wasm module emission
→ runtime integration

## Responsibilities

The wasm backend owns:
- wasm target selection
- wasm32 / wasm64 configuration
- import/export mapping
- memory layout
- stack lowering
- runtime glue generation
- symbol visibility
- section ordering
- wasm binary emission

## Supported Targets

Possible targets:
- wasm32-unknown-unknown
- wasm32-wasi
- wasm64-unknown-unknown

Each target must define:
- pointer width
- ABI model
- runtime expectations
- import namespace contracts
- memory model
- object emission strategy

## Invariants

- wasm generation must be deterministic
- section ordering must remain stable
- symbol export rules must be explicit
- memory layout must be validated
- unsupported wasm features must fail explicitly
- runtime integration must be validated
- import resolution must be deterministic
- stack layout must remain reproducible

## Critical Rule

Silent fallback is forbidden.

Invalid:
- silently switching wasm target
- disabling unsupported instructions automatically
- changing ABI implicitly
- silently removing exports
- silently injecting runtime behavior

All backend fallback paths must produce explicit diagnostics.

## Recommended Structure

wasm/
├── mod.vit
├── target.vit
├── abi.vit
├── runtime.vit
├── memory.vit
├── section.vit
├── imports.vit
├── exports.vit
├── lowering.vit
├── emitter.vit
├── linker.vit
└── README.md

## WASM Contracts

Each wasm target must explicitly define:
- endian model
- pointer size
- stack model
- linear memory policy
- alignment rules
- export policy
- runtime ABI
- trap behavior

Example:

Target:
- wasm32-wasi

Contracts:
- little endian
- 32-bit pointers
- WASI ABI
- linear memory enabled
- explicit import namespace
- deterministic section ordering

## ABI Contracts

The wasm ABI layer defines:
- parameter lowering
- return lowering
- stack management
- indirect call handling
- function table policy
- runtime entrypoints

ABI mismatches must fail validation.

## Memory Contracts

The memory layer owns:
- stack base
- heap base
- global offsets
- alignment rules
- memory growth policy
- data segment placement

Invalid memory layouts must fail before emission.

## Import / Export Contracts

The backend must explicitly validate:
- imported functions
- imported globals
- imported memories
- exported symbols
- namespace compatibility

Missing imports must produce deterministic diagnostics.

## Runtime Integration

The runtime layer owns:
- startup glue
- allocator hooks
- panic handling
- trap integration
- runtime initialization
- wasi integration

Runtime payloads must match:
- target ABI
- pointer width
- memory model

## Object / Module Emission

The emitter owns:
- section serialization
- opcode encoding
- symbol serialization
- debug section emission
- relocation metadata
- wasm binary layout

Generated modules must be deterministic byte-for-byte for identical inputs.

## Reproducible Commands

Build WASM module:

vittec build app.vit --target wasm32-unknown-unknown

Build WASI module:

vittec build app.vit --target wasm32-wasi

Emit textual WAT:

vittec build app.vit --emit-wat

Emit debug sections:

vittec build app.vit --target wasm32-wasi --debug

Validate generated module:

wasm-validate build/app.wasm

Run with wasmtime:

wasmtime build/app.wasm

## Testing Strategy

Recommended tests:
- deterministic wasm snapshots
- import/export validation
- ABI lowering verification
- runtime compatibility checks
- memory layout verification
- section ordering tests
- opcode encoding validation
- WASI startup tests
- relocation validation
- trap behavior verification

## Backend Diagnostics

Examples:
- WASM_E_UNSUPPORTED_TARGET
- WASM_E_INVALID_IMPORT
- WASM_E_INVALID_EXPORT
- WASM_E_INVALID_MEMORY_LAYOUT
- WASM_E_UNSUPPORTED_OPCODE
- WASM_E_RUNTIME_MISMATCH
- WASM_E_INVALID_SECTION
- WASM_E_LINK_FAILURE

## Design Philosophy

The wasm backend is a deterministic module generation system.

It should remain:
- ABI-explicit
- runtime-safe
- deterministic
- validation-heavy
- section-stable
- import-aware
- linker-aware

The backend must never guess runtime or ABI behavior.
