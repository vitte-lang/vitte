# Vitte Standard Library (stdlib) Organization

The stdlib/ directory is organized into 5 logical categories:

## Core
Essential runtime components without allocation:
- **core/**: Core library (no_std compatible)
- **vitte-std-workspace-core/**: Workspace abstraction for core
- **coretests/**: Core library tests

## Alloc
Memory allocation and heap-based data structures:
- **alloc/**: Allocation library (requires allocator)
- **vitte-std-workspace-alloc/**: Workspace abstraction for alloc
- **alloctests/**: Alloc tests
- **compiler-builtins/**: Compiler built-in functions

## Std
Complete standard library with full OS integration:
- **std/**: Standard library (requires OS)
- **vitte-std-workspace-std/**: Workspace abstraction for std
- **proc_macro/**: Procedural macro support

## Runtime
Runtime support and panic handling:
- **unwind/**: Unwinding implementation
- **panic_abort/**: Abort-on-panic handler
- **panic_unwind/**: Unwind-on-panic handler
- **rtstartup/**: Runtime startup code
- **profiler_builtins/**: Profiler built-in functions

## Extra
Optional and platform-specific components:
- **backtrace/**: Stack backtrace support
- **portable-simd/**: SIMD support
- **std_detect/**: CPU feature detection
- **stdarch/**: Architecture-specific functionality
- **test/**: Testing framework
- **windows_targets/**: Windows target support

This organization reflects Vitte's layered architecture, allowing code to be used at different levels of abstraction depending on platform and use-case constraints.
