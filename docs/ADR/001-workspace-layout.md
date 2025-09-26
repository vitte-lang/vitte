# ADR-001 — Layout du workspace

- **Décision** : Workspace multi-crates (`crates/*`, `std/`, `modules/*`, `tests/`, `benchmarks/`).
- **Motivation** : séparations nettes, builds ciblés, docs par brique.
- **Conséquences** : un peu plus de glue, mais scalabilité ++.
