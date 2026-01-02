# data generators (bench) — enterprise

Deterministic generators to create corpora under:
- `bench/src/data/generated/` (stable CI fixtures)
- optional refresh of `bench/src/data/fuzz/` large files

Principles:
- deterministic (fixed seed unless overridden)
- LF newlines
- avoid timestamps in generated sources
- provide a manifest + checksums for CI

Tools:
- `refresh_all.py`       regenerate *all* datasets into `../generated`
- `gen_large_vitte.py`   single large file (lines-based)
- `gen_tokens.py`        token-dense arithmetic
- `gen_project.py`       multi-file synthetic project tree
- `gen_invalid.py`       invalid/error-path fixtures
- `gen_unicode.py`       UTF-8 idents/strings fixtures (conservative set)
- `gen_stats.py`         produces size & sha256 manifest for a dataset root
