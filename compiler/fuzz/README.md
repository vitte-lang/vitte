# fuzz corpus (bench) — enterprise

Adversarial inputs for compiler pipeline performance and error-path profiling.

Primary consumers:
- lexer benchmarks (tokenization, trivia/comments)
- parser benchmarks (deep nesting, recovery, precedence)
- lowering benchmarks (surface/phrase desugaring)
- sema benchmarks (name lookup, shadowing, module graphs)
- diagnostics benchmarks (many errors, formatting, spans)
- incremental mode experiments (many small files)

## Structure

- `lex/`      token stress: literals, comments, long lines, token density
- `parse/`    block nesting, precedence, match arms, recovery-friendly errors
- `lower/`    surface constructs to desugar heavily
- `sema/`     namespaces, shadowing, repetitive declarations
- `invalid/`  intentionally broken sources to exercise diag + recovery paths
- `unicode/`  UTF-8 identifiers and strings (conservative set)
- `path/`     import/module path edge cases
- `meta/`     manifests, checksums, size stats, coverage map
- `generators/` scripts to regenerate large fixtures deterministically

Policy:
- deterministic, stable output; no timestamps inside sources
- LF newlines
- keep files readable; generated files documented in `meta/manifest.json`

