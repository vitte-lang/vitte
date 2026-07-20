# Vitte Standard Library Architecture

The Vitte standard library is a compiled Vitte source tree rooted at
`src/vitte/stdlib`. Its architecture is defined by
`src/vitte/stdlib/stdlib_architecture.json` and validated by
`make stdlib-gate`.

## Levels

The dependency hierarchy is strict:

1. `core`
2. `alloc`
3. `std`
4. `platform`
5. `experimental`

Lower levels cannot depend on higher levels. `experimental` can use stable
lower layers, but stable layers cannot import `experimental`.

## Core

`core` has no operating-system dependency and cannot use dynamic allocation.
It owns primitive constants, simple forms, fixed-size helpers, and algorithms
that work in freestanding builds.

## Alloc

`alloc` owns allocator-aware memory and collection APIs. It may depend on
`core`, but it must remain portable and must not require an operating system.

## Std

`std` owns portable high-level modules such as math, strings, encoding, JSON,
crypto, compression, regex, and data helpers. It may depend on `core` and
`alloc`.

## Platform

`platform` owns host, kernel, IO, path, process, network, threading, runtime,
and FFI boundaries. Platform modules are the only standard-library level that
may directly encode operating-system behavior.

## Experimental

`experimental` owns unstable APIs and test-only surfaces. No stable layer may
depend on it.

## CI Rules

The stdlib gate fails when:

- a module imports a higher level;
- the dependency graph contains a cycle;
- `core` imports operating-system modules;
- `core` uses dynamic allocation patterns;
- the architecture manifest is missing or malformed;
- stdlib sources are not Vitte source files.
