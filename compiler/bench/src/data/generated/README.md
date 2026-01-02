# generated

Generated datasets for bench suites.

This folder is meant to be refreshed by scripts and should be treated as a build artifact
committed only when you want stable CI fixtures.

Subfolders:
- `lex/`     token-dense sources
- `parse/`   deep block sources, large match/expr corpora
- `lower/`   phrase/surface heavy constructs
- `sema/`    many decls/modules to stress name lookup
- `proj/`    multi-file synthetic projects
- `meta/`    manifest + checksums + sizes
- `scripts/` refresh scripts (deterministic)

Policy:
- deterministic generation (fixed seed)
- LF newlines
