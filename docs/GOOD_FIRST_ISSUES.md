# Good First Issues

This page turns "good first contributions" into concrete entry points.

## Best Starting Areas

### Diagnostics clarity

Good first issue shape:

- improve wording
- improve span accuracy
- add or tighten one regression fixture

Start with:

- `src/compiler/frontend/diagnostics.cpp`
- `tests/diag_snapshots/`
- `docs/DEBUGGING_COMPILER.md`

### Docs and onboarding

Good first issue shape:

- clarify one confused path
- improve one example
- tighten one index page

Start with:

- `README.md`
- `docs/GETTING_STARTED.md`
- `docs/FIRST_PROJECT.md`
- `docs/README.md`

### Focused regression tests

Good first issue shape:

- add one missing valid case
- add one missing invalid case
- improve one snapshot expectation

Start with:

- `docs/TEST_STRATEGY.md`
- `docs/TEST_MAP.md`
- `tests/`

### Tooling ergonomics

Good first issue shape:

- improve one helper script
- tighten one report
- improve one help or doc path

Start with:

- `tools/doctor.sh`
- `tools/quickstart_check.sh`
- `docs/MAKE_TARGETS.md`

## Avoid As A First Issue

- broad language surface changes
- large package governance refactors
- moving many scripts at once
- changing compatibility promises without tests and docs in the same PR
