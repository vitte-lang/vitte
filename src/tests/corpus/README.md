# Vitte Corpus

`make -C compiler corpus` is the required compiler corpus gate.

It covers:

- `check` for mono-file and multi-file inputs
- `emit-c` for mono-file and multi-file inputs
- `build` for mono-file and multi-file inputs
- import chains, rich imports, module aliases, explicit exports, and export-star
- negative diagnostics for glob collisions, unsupported `use module::symbol`, and imported call type mismatches

Fixtures listed as `unavailable` in `corpus_expectations.tsv` may be absent
from a checkout; the corpus runner reports those cases as skipped. Missing
fixtures with any other expectation remain an error.

Unavailable fixtures with an expected diagnostic code use the
`CODE|reason` form and are checked by the normal corpus gate. They are
reported as known gaps only when that exact diagnostic is produced; if the
fixture starts compiling successfully, it is reported as newly supported.
`make test-unavailable` additionally probes unavailable entries that do not
yet have an expected diagnostic code. Unexpected failures still fail the
target.
Each compiler invocation has a 15-second limit so a resolver regression cannot
hang the corpus indefinitely.

Fixtures classified as `obsolete` are listed for historical context only and
are skipped even by `make test-unavailable`; they are not compiler acceptance
tests for the current language/runtime.

`make test-runtime` executes runtime fixtures supported by the current
frontend. Missing fixture files are reported as skips.
