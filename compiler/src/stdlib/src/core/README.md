# stdlib/core

`core` is the minimal, allocation-agnostic foundation of the standard library.

Principles:
- stable API surface (compiler + runtime depend on it)
- minimal dependencies (no syscalls by default)
- explicit error/option modeling
- portable: pure library semantics

Modules:
- prelude: common exports for user code
- option/result: algebraic error handling primitives
- iter: iterator adapters (baseline)
- slice/str: primitives for arrays and strings
- num: numeric helpers
- mem: low-level memory helpers (safe wrappers)
- collections: small datastructures (Vec, Map stubs)
- fmt: formatting building blocks (string builder style)
- hash: hashing primitives (non-crypto)
- unicode: basic unicode helpers (stubs)
- json: minimal JSON value model (optional)
- tests/golden/bench: stability infrastructure
