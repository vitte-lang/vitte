# stdlib tests

Path: `src/vitte/stdlib/tests`

## Purpose

Aggregated smoke coverage for the top-level standard library families.

## Notes

- Prefer calling existing module `*_selftest()` entry points.
- Keep this suite broad and shallow: it should validate surface readiness, not duplicate unit logic.
- Family-specific smoke files can live next to the module when deeper coverage is needed.
