# Vitte Compiler Driver

Path: `src/vitte/compiler/driver`

---

## Purpose

The `driver` layer is responsible for orchestrating the high-level compiler
execution pipeline.

`src/vitte/compiler/driver/compiler.vit` is the runtime command dispatcher used
behind the public facade in `src/vitte/compiler/main.vit`.

This module owns:
- compiler session startup
- command parsing
- pipeline sequencing
- compilation mode dispatch
- artifact emission coordination
- top-level error propagation

This layer must NOT contain:
- lexer implementation
- parser implementation
- MIR transformations
- borrow checking logic
- backend machine lowering
- platform-specific linker logic

The driver coordinates subsystems but does not implement them.

## Runtime Core Contract

The runtime driver core currently lives in:

- `src/vitte/compiler/main.vit`
- `src/vitte/compiler/driver/compiler.vit`
- `src/vitte/compiler/driver/compile.vit`

The contract is:

- `main.vit` stays the public compiler entrypoint
- `driver/compiler.vit` stays the CLI/runtime orchestration core
- `driver/compile.vit` stays the lower-level compile/build execution surface

The runtime core may depend on canonical compiler surfaces such as:

- `frontend/*`
- `analysis/*`
- `middle/*`
- `backend/*`
- `infrastructure/session/*`

It must not silently grow direct dependencies on parallel or legacy adapter
surfaces when a canonical backend/runtime surface already exists.

---

# Responsibilities

The driver is the canonical entry point between:
- CLI tools
- build systems
- IDE integrations
- test harnesses
- compiler pipelines

The driver transforms:
- command line arguments
- build requests
- frontend/backend configuration

into:
- deterministic compiler executions

---

# Architecture

```text
CLI
 ↓
driver
 ↓
session
 ↓
frontend
 ↓
HIR
 ↓
MIR
 ↓
backend
 ↓
artifacts
```

---

# Invariants

The driver must preserve the following invariants:

## Deterministic execution

Same:
- source inputs
- configuration
- target
- compiler version

must produce:
- same diagnostics
- same MIR
- same backend IR
- same output artifacts

---

## Stateless orchestration

The driver should avoid hidden mutable global state.

All execution state should flow through:
- DriverSession
- DriverConfig
- PipelineState

---

## Explicit failure propagation

Subsystem failures must propagate through structured diagnostics.

Avoid:
- silent failures
- implicit recoveries
- hidden panic paths

---

## Stable pipeline ordering

The canonical pipeline order is:

```text
parse
→ ast validation
→ sema
→ typeck
→ hir lowering
→ mir lowering
→ borrowck
→ optimizations
→ backend
→ link
```

The driver is responsible for enforcing this ordering.

---

# Recommended Structure

```text
driver/
├── mod.vit
├── config.vit
├── command.vit
├── session.vit
├── pipeline.vit
├── result.vit
└── README.md
```

---

# Core Concepts

## DriverConfig

Immutable compiler configuration.

Contains:
- target
- optimization mode
- emit flags
- paths
- diagnostics options

---

## DriverSession

Owns:
- current compilation state
- loaded modules
- diagnostics
- interners
- compilation caches

A session should represent exactly one compiler execution.

---

## PipelineState

Represents:
- current compiler phase
- produced intermediate forms
- artifact validity

Must support:
- deterministic invalidation
- incremental evolution
- explicit ownership

---

# Reproducible Commands

## Check a file

```bash
vittec check examples/hello.vit
```

---

## Build executable

```bash
vittec build examples/hello.vit
```

---

## Emit MIR

```bash
vittec build examples/hello.vit --emit-mir
```

---

## Emit HIR

```bash
vittec build examples/hello.vit --emit-hir
```

---

## Release build

```bash
vittec build examples/hello.vit --release
```

---

## Cross compile

```bash
vittec build kernel.vit --target x86_64-linux
```

---

# Example Flow

```text
input.vit
 ↓
DriverCommand
 ↓
DriverConfig
 ↓
DriverSession
 ↓
PipelineExecution
 ↓
CompilationArtifacts
```

---

# Contracts

## Driver ↔ Frontend

The frontend guarantees:
- valid AST construction
- stable spans
- deterministic parsing

The driver guarantees:
- valid configuration
- ordered execution
- diagnostic propagation

---

## Driver ↔ MIR

MIR generation must only occur after:
- successful sema
- successful type checking

The driver must enforce this contract.

---

## Driver ↔ Backend

The backend receives:
- validated MIR
- resolved target configuration
- stable symbol metadata

The backend must not re-run semantic analysis.

---

# Error Handling Policy

Driver code should prefer:
- structured diagnostics
- explicit result types
- recoverable failures

Avoid:
- fatal exits inside subsystems
- hidden process termination
- backend-owned diagnostics formatting

---

# Testing Strategy

Recommended tests:
- deterministic output tests
- pipeline ordering tests
- invalid CLI argument tests
- diagnostic propagation tests
- multi-module compilation tests

---

# Future Extensions

Possible future responsibilities:
- incremental compilation driver
- distributed compilation
- cache reuse
- parallel frontend scheduling
- IDE daemon mode
- watch mode
- self-host bootstrap orchestration

These extensions should remain external orchestration layers and must not
pollute core driver responsibilities.

---

# Design Philosophy

The driver should remain:
- thin
- deterministic
- orchestration-oriented
- backend-agnostic
- reproducible
- testable

The driver is the compiler coordinator, not the compiler implementation.
