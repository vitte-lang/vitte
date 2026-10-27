# Bootstrap configuration

`config.c` implements the configuration ABI declared in `config.h` for the
standalone C bootstrap executable. The native compiler model lives in
`src/compiler/config/config.vitte`; both sides share the same enum values,
record layout and public function names.

## Behavior

Configuration values are merged by source precedence:

```text
default < system < user < project < environment < command-line < override
```

Defaults are deterministic: jobs and maximum errors are nonzero, diagnostics
use the default format with automatic color, runtime safety checks are
enabled, and deterministic and incremental compilation are enabled. Typed
setter strings are borrowed. File, environment and command-line adapters keep
their copied values and provenance strings until reset or destruction.

Each assignment records its source, optional name and location. Validation
resolves entries into `vitte_config_resolved_t`; freezing prevents later
mutation. Statistics include a semantic fingerprint and the validation pass
count. Provenance, pointer values, insertion order and assignment sequence do
not affect that fingerprint.

## Input adapters

The caller chooses when and which file to load with `vitte_config_load_file`;
this module does not discover configuration files. The format is one flat
assignment per line, separated by `=` or `:`. Blank lines and `#` or `;`
comments are accepted, as are surrounding quotes on string values. Sections
are rejected. Lines are limited to 8191 bytes.

The generic CLI adapter accepts `--key=value`, `--key value`, boolean
`--no-key`, and the `--` terminator. Unknown options and missing values are
errors. The environment adapter reads `<prefix><KEY>` with key names uppercased
and hyphens converted to underscores; a null prefix selects `VITTE_`.
Embedders can provide an isolated lookup callback with
`vitte_config_apply_environment_with`.

## Native compiler

`config.vitte` is the ABI-matched native model consumed by the compiler
pipeline. Resolved defaults and simple query helpers are implemented in Vitte;
mutation, validation and input adapters still use the bootstrap C
implementation while their native counterparts are ported. The standalone
bootstrap continues to build from this directory through the repository root
Makefile.
