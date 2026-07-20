# HIR Fixtures

These fixtures exercise the compiler HIR gates outside the in-source Vitte unit tests.

- `valid/`: single-file HIR JSON and HIR-to-MIR snapshot inputs.
- `invalid/`: parser-to-HIR recovery and empty-file diagnostics.
- `multifile/`: module-set import fixtures used by HIR tests and resolver coverage.
