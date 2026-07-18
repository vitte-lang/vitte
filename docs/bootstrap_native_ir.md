# Bootstrap Native IR v1

`native_ir_v1` is the textual intermediate representation used by the
bootstrap-native seed compiler for the early `build-native` path. It is small by
design: the format describes only the compiler identity strings, optional named
string constants, and the native process exit code needed by seed-generated
fixtures and compiler payloads.

The canonical implementation is `toolchain/seed/vittec0.seed`. Golden coverage
lives in `tests/bootstrap_native`.

`native_ir_v1` is the stable bootstrap contract. The shell emitted by
`build-native` is the current POSIX bootstrap backend, not the IR contract
itself. The backend may be replaced later as long as `native_ir_v1` snapshots and
documented command behavior remain stable.

## Versioning

`native_ir_v1` is additive-only. New records or accepted source forms may be
added when older v1 readers can ignore or reject them safely under the existing
fail-closed rules.

Any breaking format change must use a new first-line tag, starting with
`native_ir_v2`. Breaking changes include changing the meaning of an existing
record, removing a required record, changing record order requirements, or
changing diagnostics in a way that v1 snapshots can no longer defend.

`native_ir_v1` snapshots remain authoritative as long as seed artifacts
read or emit v1. Introducing a future v2 must keep v1 snapshots and drift checks
until the stage0 trust root no longer depends on v1.

## Format

The IR is UTF-8 text with LF line endings. The first line is the version tag:

```text
native_ir_v1
```

Remaining lines are key/value records:

```text
const.string.<NAME>=<string value>
const.int.<NAME>=<integer>
proc.int.<name>=<integer>
version_text=<string value>
banner_text=<string value>
main.return=<integer>
```

Rules:

- `const.string.<NAME>` records are optional and appear in source declaration
  order.
- `const.int.<NAME>` records are optional and appear in source declaration
  order with string constants.
- `proc.int.<name>` records are optional and appear in source declaration order.
  In v1 they are record-only: generated shell compilers do not expose these
  procedures as callable commands yet.
- `NAME` is an uppercase bootstrap constant identifier:
  `[A-Z_][A-Z0-9_]*`.
- `version_text`, `banner_text`, and `main.return` appear exactly once.
- String values are the decoded contents of bootstrap string literals. The v1
  subset does not support escapes or embedded double quotes.
- Integer values are signed decimal integers.
- `main.return` is a signed decimal integer. If no supported `main` procedure is
  present, it defaults to `0`.

Example:

```text
native_ir_v1
const.string.VERSION_TEXT=vittec native fixture 0.1.0
const.string.BANNER_TEXT=vittec native fixture
const.int.EXIT_CODE=0
version_text=vittec native fixture 0.1.0
banner_text=vittec native fixture
main.return=0
```

## Source Subset

`native_ir_v1` is emitted only for the bootstrap-native subset:

- `space <path>`
- `const NAME: string = "..."`
- `const NAME: int = 123`
- `proc name() -> string { give "..." }`
- `proc name() -> string { give NAME }`
- `proc name() -> int { give 123 }`
- `proc name() -> int { give NAME }`
- `proc main(args: list[string]) -> int { give <integer> }`
- `proc main(args: list[string]) -> int { give NAME }`
- `use <path>` / `share <path>` / `import <path>` at top level when
  `const BOOTSTRAP_FULL_COMPILER: int = 1` is present
- `proc main(args: list[string]) -> int { ... }` with local `let`, direct call
  expressions, and `if` control flow when `BOOTSTRAP_FULL_COMPILER: int = 1`
  is present
- `export *`

Only `version_text()` and `banner_text()` are currently materialized as compiler
identity procedures. Other string procedures are rejected until the native
bootstrap surface grows intentionally.

Integer procedures are accepted and emitted as `proc.int.*` records to pin the
lowering shape, but they are record-only in v1. They are not callable from the
generated shell compiler and do not affect process behavior unless their value
is used through `main.return`.

When `BOOTSTRAP_FULL_COMPILER` is enabled, the richer `main` body is accepted as
source coverage for the real compiler entry point, but v1 still lowers it to the
same shell contract and keeps `main.return=0`.

## Non-Goals v1

The v1 subset intentionally does not support:

- string escapes or embedded double quotes,
- multi-line declarations or multi-line procedure signatures,
- `bool`, list, map, user-defined, or composite constants,
- procedure calls or expression evaluation outside the guarded full-compiler
  `main`,
- statements other than a single supported `give` inside each procedure, except
  for the guarded full-compiler `main`,
- general-purpose Vitte parsing or semantic analysis.

## Invariants

- The output must be deterministic for a fixed source file and seed artifact.
- Constants are emitted before derived procedure records.
- `proc.int.*` records are emitted for snapshot stability only in v1; emission
  must not imply a public callable shell command.
- Procedure references to named constants are resolved before IR emission.
- Missing `banner_text()` or `version_text()` is an error.
- `export *` is required so bootstrap sources keep an explicit public surface.
- Unsupported top-level items and unsupported procedure bodies must fail closed.
- Binaries generated from this IR must preserve `--version`
  output, generated `--help` output, and default command exit behavior.

## Generated Command Surface

The current backend emits POSIX shell compilers with this command surface:

- `--version`: takes no input, prints the generated compiler version text, exits
  `0`.
- `--help`, `-h`, `help`: takes no input, prints the generated banner, version
  line, and command list, exits `0`.
- `parse`: accepts `--src <file>` or the first positional file argument. It
  validates the file with the bootstrap-native parser, prints `parse ok:
  <file>` on success, exits non-zero on diagnostics or missing input.
- `check`: accepts one positional file path. It currently checks that the file
  exists and, for sources under `space vitte/bootstrap/...`, validates the file
  with the bootstrap-native parser. Missing input, unreadable files, or
  bootstrap-native diagnostics fail.
- `build-native`: requires `--src <file>` and `--out <file>`. It emits the
  current POSIX shell backend for the source IR and marks the output executable.
- `dump-native-ir`: requires `--src <file>`. It prints `native_ir_v1` text to
  stdout or fails with diagnostics.
- `dump-native-shell`: requires `--src <file>`. It prints the generated POSIX
  shell backend to stdout. This is a review aid for backend snapshots, not a
  separate IR contract.
- `build`: accepts a source file and `-o`/`--out`. It compiles the native subset
  directly and may delegate unsupported full-language forms only through the
  explicitly configured generic backend.
- default command: with no command, prints the generated banner and exits with
  `main.return`.
- unknown command: prints `[<tool>][error] unknown command: <cmd>` to stderr
  and exits `2`. `<tool>` is generated from the first word of `version_text()`,
  so the seed reports `[vittec0][error]`.

## Diagnostics

Diagnostics are line-oriented stderr records:

```text
<file>:<line>:<column>: <CODE>: <message>
```

Current bootstrap-native diagnostic codes include:

- `E_BOOTSTRAP_CONST_SIGNATURE`: constant declaration is outside
  `const NAME: string = "..."` or `const NAME: int = 123`.
- `E_BOOTSTRAP_CONST_TYPE`: a procedure references a constant with the wrong
  bootstrap type.
- `E_BOOTSTRAP_PROC_SIGNATURE`: non-`main` procedure signature is outside
  `proc name() -> string {`.
- `E_BOOTSTRAP_MAIN_SIGNATURE`: `main` signature is outside
  `proc main(args: list[string]) -> int {`.
- `E_BOOTSTRAP_UNKNOWN_CONST`: a procedure gives an unknown string constant.
- `E_BOOTSTRAP_UNKNOWN_PROC`: a string procedure is not supported by v1.
- `E_BOOTSTRAP_PROC_BODY`: procedure body is not one supported `give`.
- `E_BOOTSTRAP_MAIN_BODY`: `main` does not give an integer.
- `E_BOOTSTRAP_UNCLOSED_PROC`: a procedure body is missing its closing brace.
- `E_BOOTSTRAP_TOP_LEVEL`: unsupported top-level item.
- `E_BOOTSTRAP_EXPORT`: missing `export *`.
- `E_BOOTSTRAP_VERSION`: missing `version_text()`.
- `E_BOOTSTRAP_BANNER`: missing `banner_text()`.

Bad-input snapshots in `tests/bootstrap_native/*.err.must` pin the user-visible
diagnostic contract.

## Hash Stability

`tests/bootstrap_native/shell.*.must` pins generated bootstrap shell output. A
change is acceptable only when one of these contracts intentionally changes:

- the `native_ir_v1` source subset,
- IR-to-shell emission,
- command behavior of generated binaries,
- seed artifact bytes copied into generated binaries,
- canonical seed behavior or compiler entry.

When generated shell changes intentionally, update the relevant IR, diagnostic,
and shell snapshots in the same change. The verification command is:

```sh
tools/bootstrap_native_snapshots.sh
```

`make bootstrap-native-drift-check` enforces that sensitive seed/compiler/native
build changes carry the matching emission and IR/diagnostic snapshots.
