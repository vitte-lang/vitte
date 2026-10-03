cat > /Users/vincent/Documents/Github/vitte/src/src/cli/README.md <<'EOF'
# Vitte CLI

`src/src/cli` contains the canonical command-line interface parser for the Vitte compiler.

The CLI layer converts raw process arguments into a validated, structured representation of user intent.

It does not compile programs itself.

Its primary responsibility is:

```text
argv
 |
 v
CLI parser
 |
 v
validated CLI configuration
 |
 v
compiler driver
```

The compiler driver then dispatches work to the appropriate Vitte subsystem.

---

## 1. Architecture

```text
                         command line
                              |
                              v
                       +-------------+
                       |    argv     |
                       +------+------+
                              |
                              v
                    +-------------------+
                    |      cli.c        |
                    | parsing           |
                    | validation        |
                    | normalization     |
                    +---------+---------+
                              |
                              v
                    +-------------------+
                    | vitte_cli_config  |
                    +---------+---------+
                              |
                              v
                    +-------------------+
                    |      driver       |
                    +---------+---------+
                              |
          +-------------------+-------------------+
          |                   |                   |
          v                   v                   v
      frontend             codegen          package manager
          |                   |                   |
          v                   v                   v
      AST / IR             backend             modules
                              |
                              v
                             C17
```

The central rule is:

```text
CLI decides WHAT the user requested.

Driver decides HOW that request is executed.
```

---

## 2. Directory

```text
cli/
├── cli.c
├── cli.h
├── Makefile
└── README.md
```

### `cli.h`

Canonical public API.

It defines:

- CLI states;
- CLI errors;
- commands;
- backend selection;
- optimization selection;
- diagnostic formats;
- color policy;
- parsed configuration;
- parser messages;
- statistics;
- parser lifecycle;
- input access;
- forwarded arguments;
- deterministic fingerprints;
- command classification helpers.

### `cli.c`

Implements:

- argument parsing;
- command recognition;
- option parsing;
- positional inputs;
- `--` forwarding;
- numeric validation;
- duplicate option detection;
- conflict detection;
- capacity management;
- deterministic hashing;
- lifecycle validation;
- final configuration validation.

### `Makefile`

Provides standalone strict C17 compilation and validation for the CLI subsystem.

---

## 3. Responsibilities

The CLI subsystem is responsible for:

1. accepting `argc` and `argv`;
2. identifying the requested command;
3. parsing global options;
4. parsing command-related options;
5. parsing positional inputs;
6. parsing options with separate values;
7. parsing options with `--option=value`;
8. handling short options;
9. handling `--`;
10. forwarding runtime arguments;
11. validating numeric values;
12. rejecting unknown options;
13. detecting duplicate options;
14. detecting conflicting options;
15. validating required inputs;
16. recording target selection;
17. recording backend selection;
18. recording optimization policy;
19. recording diagnostic policy;
20. recording runtime safety policy;
21. recording package-manager policy;
22. enforcing parser limits;
23. maintaining deterministic configuration;
24. producing a configuration fingerprint;
25. reporting structured parsing failures.

The CLI subsystem is deliberately not responsible for:

- reading source files;
- lexing;
- parsing Vitte source;
- semantic analysis;
- type checking;
- IR construction;
- optimization;
- code generation;
- C17 generation;
- invoking a C compiler;
- linking;
- executing programs;
- downloading packages;
- installing modules;
- formatting source code;
- running tests;
- rendering full compiler diagnostics.

Those operations belong to other subsystems.

---

## 4. Basic usage model

Typical compiler entry point:

```c
#include "cli.h"

int
main(
    int argc,
    char **argv)
{
    vitte_cli_t cli;

    if (!vitte_cli_init(&cli)) {
        return 2;
    }

    if (!vitte_cli_parse(
            &cli,
            argc,
            (const char *const *)argv)) {
        int status;

        status =
            vitte_cli_suggested_exit_status(
                &cli);

        vitte_cli_destroy(&cli);

        return status;
    }

    /*
     * Pass cli configuration to the driver.
     */

    vitte_cli_destroy(&cli);

    return 0;
}
```

The final executable should normally delegate execution to a dedicated driver rather than implementing command behavior directly in `main()`.

---

## 5. Command model

The current CLI recognizes:

```text
build
run
check
test
fmt
install
uninstall
update
clean
version
help
```

The corresponding enum is:

```c
typedef enum vitte_cli_command {
    VITTE_CLI_COMMAND_NONE = 0,

    VITTE_CLI_COMMAND_BUILD,
    VITTE_CLI_COMMAND_RUN,
    VITTE_CLI_COMMAND_CHECK,
    VITTE_CLI_COMMAND_TEST,
    VITTE_CLI_COMMAND_FMT,

    VITTE_CLI_COMMAND_INSTALL,
    VITTE_CLI_COMMAND_UNINSTALL,
    VITTE_CLI_COMMAND_UPDATE,

    VITTE_CLI_COMMAND_CLEAN,

    VITTE_CLI_COMMAND_VERSION,
    VITTE_CLI_COMMAND_HELP,

    VITTE_CLI_COMMAND_COUNT
} vitte_cli_command_t;
```

---

## 6. `build`

Example:

```sh
vitte build main.vit
```

Multiple inputs:

```sh
vitte build main.vit math.vit io.vit
```

Explicit output:

```sh
vitte build main.vit -o app
```

Release policy:

```sh
vitte build main.vit --release
```

Target:

```sh
vitte build main.vit --target aarch64-linux
```

Backend:

```sh
vitte build main.vit --backend c17
```

Optimization:

```sh
vitte build main.vit --opt speed
```

The CLI only records these choices.

The compiler driver performs the actual build.

---

## 7. Implicit build

A source file can be treated as a build request:

```sh
vitte main.vit
```

Conceptually this becomes:

```sh
vitte build main.vit
```

This provides a convenient path for small programs while retaining explicit subcommands for larger workflows.

---

## 8. `run`

Example:

```sh
vitte run main.vit
```

Program arguments are separated with:

```text
--
```

Example:

```sh
vitte run main.vit -- hello world
```

The CLI stores:

```text
input:
    main.vit

forwarded arguments:
    hello
    world
```

The CLI does not execute the resulting program.

Execution belongs to the driver.

---

## 9. `check`

Example:

```sh
vitte check main.vit
```

The intended semantic meaning is:

```text
load
 |
 v
parse
 |
 v
semantic analysis
 |
 v
type checking
 |
 v
validation
 |
 X
no final executable required
```

The exact compilation depth remains a driver/compiler policy.

---

## 10. `test`

Example:

```sh
vitte test
```

Filter tests:

```sh
vitte test --filter parser
```

Forward test-runner arguments:

```sh
vitte test -- --verbose
```

The CLI records the test request and arguments.

Test discovery and execution belong to the test subsystem and driver.

---

## 11. `fmt`

Format a project:

```sh
vitte fmt
```

Format files:

```sh
vitte fmt main.vit lib.vit
```

Check formatting without modifying files:

```sh
vitte fmt --check
```

Request writing:

```sh
vitte fmt --write
```

The parser rejects conflicting formatting modes when both are enabled simultaneously.

Formatting implementation belongs to the formatter subsystem.

---

## 12. `install`

Example:

```sh
vitte install mymodule
```

The intended package architecture can support commands such as:

```sh
vitte install network
vitte install json
vitte install math
```

The CLI does not perform network operations.

It only records package-manager intent.

A package manager can later resolve a module against infrastructure such as:

```text
vitte-lang.org
```

or another configured registry.

---

## 13. `uninstall`

Example:

```sh
vitte uninstall mymodule
```

The package manager determines:

- installed package location;
- dependency consequences;
- lock-file changes;
- filesystem operations.

---

## 14. `update`

Example:

```sh
vitte update
```

or, depending on future package policy:

```sh
vitte update mymodule
```

The CLI remains independent from registry implementation.

---

## 15. `clean`

Example:

```sh
vitte clean
```

The driver/build subsystem decides which generated artifacts are safe to remove.

The CLI itself performs no filesystem deletion.

---

## 16. `version`

```sh
vitte version
```

or:

```sh
vitte --version
```

The parser exposes:

```text
show_version
```

to the driver.

Version rendering belongs outside the parser.

---

## 17. `help`

```sh
vitte help
```

or:

```sh
vitte --help
```

With no command:

```sh
vitte
```

the current parser defaults to help behavior.

---

## 18. CLI lifecycle

The CLI object has an explicit lifecycle:

```text
                 vitte_cli_init()
                         |
                         v
                       READY
                         |
                         v
                 vitte_cli_parse()
                         |
                         v
                      PARSING
                         |
               +---------+---------+
               |                   |
               v                   v
             PARSED              FAILED
               |                   |
               +---------+---------+
                         |
                         v
                  vitte_cli_reset()
                         |
                         v
                       READY
```

Final destruction:

```text
READY / PARSED / FAILED
          |
          v
vitte_cli_destroy()
          |
          v
      DESTROYED
```

---

## 19. States

States are:

```text
INVALID
READY
PARSING
PARSED
FAILED
DESTROYED
```

### `INVALID`

Sentinel state.

### `READY`

The object is initialized and ready to parse.

### `PARSING`

Arguments are currently being processed.

### `PARSED`

Parsing and final validation completed successfully.

### `FAILED`

A parsing or configuration error occurred.

### `DESTROYED`

Owned resources have been released.

---

## 20. Configuration

The central parsed configuration is:

```c
typedef struct vitte_cli_config {
    vitte_cli_command_t command;

    vitte_cli_backend_t backend;
    vitte_cli_optimization_t optimization;

    vitte_cli_diagnostic_format_t diagnostic_format;
    vitte_cli_color_mode_t color;

    const char *target;
    const char *output;
    const char *manifest_path;

    const char *package;
    const char *test_filter;

    size_t jobs;
    size_t max_errors;

    bool verbose;
    bool quiet;

    bool release;
    bool debug;

    bool emit_debug_info;
    bool emit_source_map;
    bool emit_comments;

    bool runtime_checks;
    bool bounds_checks;
    bool null_checks;
    bool overflow_checks;

    bool warnings_as_errors;

    bool deterministic;
    bool incremental;

    bool offline;
    bool locked;
    bool frozen;

    bool force;

    bool check_format;
    bool write_format;

    bool show_help;
    bool show_version;
} vitte_cli_config_t;
```

This structure is the primary contract between CLI parsing and driver execution.

---

## 21. Backend selection

Current generic backend choices are:

```text
default
c17
native
bytecode
```

Example:

```sh
vitte build main.vit --backend c17
```

The existence of an enum value does not necessarily mean that the corresponding backend is already implemented.

The driver must verify backend availability.

---

## 22. C17 backend

The concrete C17 backend belongs to:

```text
src/src/backend/c17/
```

The intended flow is:

```text
CLI
 |
 v
driver
 |
 v
frontend
 |
 v
IR
 |
 v
codegen
 |
 v
backend/c17
 |
 v
ISO C17
```

The CLI must not contain C17 emission logic.

---

## 23. Optimization

Accepted optimization policies include:

```text
default
none
debug
balanced
speed
size
aggressive
```

Examples:

```sh
vitte build main.vit --opt none
vitte build main.vit --opt debug
vitte build main.vit --opt balanced
vitte build main.vit --opt speed
vitte build main.vit --opt size
vitte build main.vit --opt aggressive
```

Numeric aliases can also be recognized by the implementation:

```text
0
1
2
3
```

Optimization selection describes policy.

Actual optimization passes belong to the compiler optimization pipeline.

---

## 24. Release and debug

Release:

```sh
vitte build main.vit --release
```

Debug:

```sh
vitte build main.vit --debug
```

These modes conflict.

The CLI rejects:

```sh
vitte build main.vit --release --debug
```

unless future semantics explicitly redefine that relationship.

---

## 25. Target selection

Example:

```sh
vitte build main.vit --target x86_64-linux
```

or:

```sh
vitte build main.vit --target aarch64-macos
```

The CLI stores the target string.

Parsing it into a canonical architecture/OS/ABI target belongs to the target subsystem or driver.

This separation avoids duplicating target knowledge in CLI code.

---

## 26. Cross-compilation

CLI target selection must not assume:

```text
host == target
```

For example:

```text
macOS AArch64 host
        |
        v
vitte --target x86_64-linux
        |
        v
Linux x86_64 target
```

The target string describes the requested compilation target, not the machine running the CLI parser.

---

## 27. Output

Short form:

```sh
vitte build main.vit -o app
```

Long form:

```sh
vitte build main.vit --output app
```

Inline form:

```sh
vitte build main.vit --output=app
```

The CLI stores the output path as a borrowed string.

It does not create the output file.

---

## 28. Manifest

Example:

```sh
vitte build --manifest-path ./vitte.toml
```

The CLI records the path.

Manifest parsing belongs to the project/configuration subsystem.

---

## 29. Jobs

Example:

```sh
vitte build main.vit --jobs 8
```

or:

```sh
vitte build main.vit -j 8
```

or:

```sh
vitte build main.vit --jobs=8
```

The value must be a positive integer.

The parser performs checked numeric conversion.

---

## 30. Error limit

Example:

```sh
vitte check main.vit --max-errors 50
```

The parser rejects:

```text
0
negative numbers
non-numeric strings
overflowing values
```

The compiler diagnostic engine ultimately enforces the configured semantic error limit.

---

## 31. Diagnostics

Supported diagnostic format policies include:

```text
default
terminal
short
json
sarif
lsp
```

Examples:

```sh
vitte check main.vit --diagnostics terminal
```

```sh
vitte check main.vit --diagnostics=json
```

```sh
vitte check main.vit --diagnostics sarif
```

The CLI selects the format.

Diagnostic rendering belongs to the diagnostic subsystem.

---

## 32. Diagnostic architecture

```text
compiler phase
      |
      v
structured diagnostic
      |
      v
diagnostic engine
      |
      +--> terminal
      |
      +--> short
      |
      +--> JSON
      |
      +--> SARIF
      |
      +--> LSP
```

The CLI must never force compiler subsystems to generate preformatted terminal strings.

---

## 33. Color

Accepted color modes:

```text
auto
always
never
```

Examples:

```sh
vitte check main.vit --color auto
vitte check main.vit --color always
vitte check main.vit --color never
```

Color policy primarily applies to human-readable terminal output.

Machine-readable output should generally avoid terminal escape sequences.

---

## 34. Verbosity

Verbose:

```sh
vitte build main.vit --verbose
```

or:

```sh
vitte build main.vit -v
```

Quiet:

```sh
vitte build main.vit --quiet
```

or:

```sh
vitte build main.vit -q
```

Verbose and quiet modes conflict.

The parser rejects configurations enabling both.

---

## 35. Warnings as errors

Examples:

```sh
vitte check main.vit -Werror
```

or:

```sh
vitte check main.vit --warnings-as-errors
```

The CLI records policy only.

The diagnostic engine decides how warnings affect compilation success.

---

## 36. Debug information

Example:

```sh
vitte build main.vit --debug-info
```

The exact debug representation is backend-specific.

Possible implementations include:

```text
#line information
DWARF
CodeView
source maps
custom Vitte metadata
```

---

## 37. Source maps

Enable:

```sh
vitte build main.vit --source-map
```

Disable:

```sh
vitte build main.vit --no-source-map
```

The C17 backend can use this policy to map generated C ranges back to original Vitte source.

---

## 38. Generated comments

Example:

```sh
vitte build main.vit --emit-comments
```

This can be useful when inspecting generated C17 output.

Generated comments are a backend concern.

---

## 39. Runtime checks

The default configuration enables safety checks.

Current policy fields include:

```text
runtime_checks
bounds_checks
null_checks
overflow_checks
```

Disable general runtime checks:

```sh
vitte build main.vit --no-runtime-checks
```

Disable bounds checks:

```sh
vitte build main.vit --no-bounds-checks
```

Disable null checks:

```sh
vitte build main.vit --no-null-checks
```

Disable overflow checks:

```sh
vitte build main.vit --no-overflow-checks
```

The CLI only records policy.

Compiler lowering and backend implementation determine the generated checks.

---

## 40. Safety principle

Release mode must not implicitly mean:

```text
unsafe
```

Optimization and safety are separate policies.

A release build can remain optimized while retaining language-required runtime protections.

---

## 41. Determinism

Deterministic mode is enabled by default.

Explicit enable:

```sh
vitte build main.vit --deterministic
```

Disable:

```sh
vitte build main.vit --no-deterministic
```

Deterministic mode should eventually influence:

```text
module ordering
symbol generation
temporary naming
backend output ordering
metadata ordering
build fingerprints
archive construction
```

---

## 42. Incremental compilation

Incremental compilation is enabled by default in the initial CLI policy.

Enable:

```sh
vitte build main.vit --incremental
```

Disable:

```sh
vitte build main.vit --no-incremental
```

The CLI does not implement the incremental cache.

It only communicates the requested policy.

---

## 43. Offline package operations

Example:

```sh
vitte install module --offline
```

Offline mode means the package manager should avoid network access.

Exact behavior belongs to package management.

---

## 44. Locked mode

Example:

```sh
vitte build --locked
```

The intended semantics are that dependency resolution must respect the existing lock state rather than silently modifying it.

The exact lock-file contract belongs to the package/project subsystem.

---

## 45. Frozen mode

Example:

```sh
vitte build --frozen
```

The current parser treats frozen mode as implying:

```text
locked = true
offline = true
```

Conceptually:

```text
frozen
  |
  +--> locked
  |
  +--> offline
```

This supports reproducible build environments.

---

## 46. Force

Example:

```sh
vitte install module --force
```

The CLI records force intent.

The receiving subsystem must determine whether forcing a particular operation is valid and safe.

---

## 47. Package selection

Example:

```sh
vitte test --package core
```

or:

```sh
vitte build --package myapp
```

The CLI stores:

```text
package = "..."
```

Project/package resolution belongs elsewhere.

---

## 48. Test filter

Example:

```sh
vitte test --filter lexer
```

The filter string remains opaque to the CLI.

The test subsystem defines matching semantics.

---

## 49. Option values

The parser supports separated values:

```sh
--target x86_64-linux
--backend c17
--jobs 8
```

and selected inline values:

```sh
--target=x86_64-linux
--backend=c17
--jobs=8
```

Short options can use separate values:

```sh
-o app
-j 8
-p core
```

---

## 50. Duplicate options

Options with a single canonical value are tracked.

Examples that should be rejected:

```sh
vitte build main.vit \
    --backend c17 \
    --backend native
```

or:

```sh
vitte build main.vit \
    --target x86_64-linux \
    --target aarch64-linux
```

Silently accepting contradictory duplicate configuration makes builds harder to reason about.

---

## 51. Conflicting options

Examples of explicit conflicts:

```text
--release + --debug
--verbose + --quiet
fmt --check + --write
```

The parser reports a structured conflict rather than relying on arbitrary last-option-wins behavior.

---

## 52. Unknown options

Unknown options are errors.

Example:

```sh
vitte build main.vit --does-not-exist
```

The parser should produce:

```text
VITTE_CLI_ERROR_UNKNOWN_OPTION
```

The driver can later render a contextual diagnostic and potentially suggest a similar option.

---

## 53. Unknown commands

The current convenience model interprets an unknown first non-option token as an input and selects implicit `build`.

Therefore:

```sh
vitte main.vit
```

works naturally.

A future command registry may distinguish probable source paths from mistyped command names.

For example:

```text
vitte biuld
```

could eventually suggest:

```text
did you mean `build`?
```

rather than treating `biuld` as a source path.

---

## 54. `--`

The double-dash token terminates normal option parsing.

For `run` and `test`, remaining arguments are forwarded.

Example:

```sh
vitte run main.vit -- --port 8080
```

CLI interpretation:

```text
Vitte option parsing:
    main.vit

program arguments:
    --port
    8080
```

This distinction is essential because the executed Vitte program may use options that have the same names as compiler options.

---

## 55. Inputs

Inputs are stored as borrowed pointers.

Example:

```sh
vitte build a.vit b.vit c.vit
```

results in:

```text
input_count = 3

inputs[0] = "a.vit"
inputs[1] = "b.vit"
inputs[2] = "c.vit"
```

Access through:

```c
vitte_cli_input_count()
vitte_cli_input_at()
```

---

## 56. Forwarded arguments

Forwarded arguments are stored separately.

Access through:

```c
vitte_cli_forward_arg_count()
vitte_cli_forward_arg_at()
```

This prevents compiler inputs from being confused with program runtime arguments.

---

## 57. Memory ownership

The CLI owns:

```text
input pointer array
forwarded-argument pointer array
configuration object
statistics
message metadata
parser state
```

The CLI does not own the strings inside `argv`.

For example:

```c
cli->inputs[0]
```

points to caller-owned command-line memory.

The caller must keep referenced strings alive while the parsed CLI configuration is in use.

---

## 58. Dynamic storage

Input arrays grow dynamically.

Growth must:

1. use checked arithmetic;
2. respect configured maximums;
3. preserve existing entries;
4. reject overflow;
5. report allocation failure;
6. never silently wrap capacity.

The initial capacity is:

```text
8
```

and capacity grows geometrically.

---

## 59. Parser limits

Default limits include:

```text
max inputs:
    65536

max forwarded arguments:
    65536

max compiler errors:
    100

default jobs:
    1
```

Parser limits can be changed before parsing with:

```c
vitte_cli_set_limits()
```

---

## 60. Checked arithmetic

CLI infrastructure uses checked arithmetic for operations such as:

```text
count + 1
capacity * 2
capacity * sizeof(pointer)
```

This prevents `size_t` wraparound from becoming an allocation vulnerability or memory corruption bug.

---

## 61. Numeric parsing

Numeric option parsing does not depend on permissive conversion behavior.

The parser validates every character and checks overflow.

For example:

```text
--jobs 8
```

is valid.

These are invalid:

```text
--jobs 0
--jobs -1
--jobs abc
--jobs 12abc
--jobs <overflowing integer>
```

---

## 62. Error model

The public CLI error model includes:

```text
NONE

INVALID_CLI
INVALID_ARGUMENT
INVALID_STATE

UNKNOWN_COMMAND
UNKNOWN_OPTION
MISSING_OPTION_VALUE
INVALID_OPTION_VALUE
DUPLICATE_OPTION
CONFLICTING_OPTIONS

MISSING_INPUT
TOO_MANY_INPUTS
TOO_MANY_FORWARD_ARGS

INVALID_TARGET
INVALID_BACKEND
INVALID_OPTIMIZATION
INVALID_DIAGNOSTIC_FORMAT
INVALID_COLOR_MODE

OVERFLOW
OUT_OF_MEMORY
```

The error enum identifies the class of failure.

Human-facing rendering belongs to the driver/diagnostic layer.

---

## 63. Error message metadata

A parser failure records:

```c
typedef struct vitte_cli_message {
    vitte_cli_error_t error;
    int argument_index;
    const char *argument;
    const char *detail;
} vitte_cli_message_t;
```

Example conceptual result:

```text
error:
    INVALID_BACKEND

argument_index:
    3

argument:
    "llvm"

detail:
    "unknown code-generation backend"
```

The driver can use this information to produce a richer diagnostic.

---

## 64. Exit status

The parser provides:

```c
vitte_cli_suggested_exit_status()
```

Current CLI parsing convention:

```text
0 = CLI configuration usable
2 = command-line usage/parsing failure
```

Compilation, linking, testing and executed-program exit statuses belong to the driver.

---

## 65. Statistics

The parser records:

```text
argument count
option count
input count
forwarded argument count
configuration fingerprint
```

Access through:

```c
vitte_cli_stats()
```

These statistics are useful for:

- tests;
- debugging;
- instrumentation;
- reproducibility checks.

---

## 66. Fingerprint

The CLI can calculate:

```c
uint64_t
vitte_cli_fingerprint(
    const vitte_cli_t *cli);
```

The fingerprint is deterministic.

It includes semantic command-line configuration such as:

```text
command
backend
optimization
diagnostic format
color policy
target
output
manifest
package
test filter
jobs
max errors
boolean policies
inputs
forwarded arguments
```

---

## 67. Pointer addresses are not hashed

The fingerprint hashes string contents.

It does not hash:

```text
argv pointer addresses
heap addresses
input array addresses
CLI object address
```

This is essential because pointer values change between processes and would destroy deterministic behavior.

---

## 68. Hash algorithm

The current implementation uses FNV-1a 64-bit.

Constants:

```c
#define VITTE_CLI_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_CLI_FNV_PRIME \
    UINT64_C(1099511628211)
```

The fingerprint is suitable as deterministic infrastructure metadata.

It must not be treated as a cryptographic hash.

---

## 69. Reset

The CLI can be reused:

```c
vitte_cli_reset(&cli);
```

Reset:

- clears parsed configuration;
- resets counters;
- clears the last message;
- clears input counts;
- clears forwarded-argument counts;
- returns the parser to `READY`.

Allocated arrays may be retained for reuse.

This avoids unnecessary allocation churn.

---

## 70. Destroy

Always destroy initialized CLI objects:

```c
vitte_cli_destroy(&cli);
```

Destroy releases owned dynamic arrays and invalidates the object.

After destruction, the object must not be treated as a live parser.

---

## 71. Magic values

The CLI uses a live magic value and dead magic value.

They help detect obvious misuse such as:

```text
uninitialized object
wrong object type
use after destroy
double lifecycle mistakes
```

Magic values are debugging/invariant aids.

They are not a replacement for memory safety.

---

## 72. Validation

`vitte_cli_is_valid()` checks structural invariants such as:

```text
correct magic
valid live state
input_count <= input_capacity
forward_arg_count <= forward_arg_capacity
non-empty arrays have storage
```

`vitte_cli_config_validate()` checks semantic configuration invariants.

---

## 73. Command classification

Helpers include:

```c
vitte_cli_command_is_compilation()
```

for:

```text
build
run
check
test
```

Package-manager classification:

```c
vitte_cli_command_uses_package_manager()
```

for:

```text
install
uninstall
update
```

Execution classification:

```c
vitte_cli_command_executes_program()
```

for commands whose workflow includes execution.

---

## 74. Driver integration

The intended driver dispatch resembles:

```c
switch (config->command) {
    case VITTE_CLI_COMMAND_BUILD:
        /* build */
        break;

    case VITTE_CLI_COMMAND_RUN:
        /* build + execute */
        break;

    case VITTE_CLI_COMMAND_CHECK:
        /* frontend checks */
        break;

    case VITTE_CLI_COMMAND_TEST:
        /* test pipeline */
        break;

    case VITTE_CLI_COMMAND_FMT:
        /* formatter */
        break;

    case VITTE_CLI_COMMAND_INSTALL:
        /* package manager */
        break;

    default:
        break;
}
```

The real driver should use exhaustive enum handling consistent with Vitte's strict warning policy.

---

## 75. Codegen integration

Relevant CLI policy maps naturally into generic codegen configuration.

Conceptually:

```text
CLI backend
        |
        v
codegen backend

CLI optimization
        |
        v
codegen optimization

CLI target
        |
        v
target parser
        |
        v
codegen target

CLI source-map flag
        |
        v
codegen emit_source_map

CLI runtime checks
        |
        v
codegen safety policy
```

This mapping should live in a driver/config adapter rather than in the CLI parser itself.

---

## 76. Diagnostic integration

CLI errors should eventually use the same structured diagnostic philosophy as compiler diagnostics.

Example:

```text
error[ECLI0004]: invalid value for `--backend`
  |
  = value: `llvm`
  = expected one of: default, c17, native, bytecode
  = help: use `vitte build --backend c17`
```

The CLI parser should provide data.

A renderer should decide presentation.

---

## 77. Future suggestions

The parser can later support typo suggestions.

Example:

```text
--traget
```

could produce:

```text
unknown option `--traget`
help: did you mean `--target`?
```

Command typo:

```text
vitte biuld
```

could produce:

```text
unknown command `biuld`
help: did you mean `build`?
```

This should use deterministic edit-distance or command-registry logic.

---

## 78. Future command registry

As the CLI grows, hard-coded command parsing can evolve into descriptors:

```c
typedef struct vitte_cli_command_descriptor {
    vitte_cli_command_t id;
    const char *name;
    const char *summary;
    uint32_t flags;
} vitte_cli_command_descriptor_t;
```

This registry could power:

```text
parsing
help generation
shell completion
documentation
command discovery
tests
```

---

## 79. Future option registry

Likewise, options can eventually become declarative descriptors.

Conceptually:

```text
name
short name
value type
scope
repeatability
conflicts
default
help text
environment mapping
```

A registry would reduce duplicated parsing/help/completion metadata.

---

## 80. Shell completion

The CLI architecture should eventually support generated completions for:

```text
bash
zsh
fish
PowerShell
```

Completion data can be generated from the same canonical command/option registry used by the parser.

This avoids documentation drift.

---

## 81. Help generation

Future help should be generated from canonical command metadata rather than manually duplicated.

Example:

```text
Vitte compiler

Usage:
    vitte <command> [options] [inputs]

Commands:
    build       Build a Vitte program
    run         Build and run a Vitte program
    check       Check a Vitte program
    test        Run tests
    fmt         Format Vitte source
    install     Install a module
    uninstall   Remove a module
    update      Update modules
    clean       Remove build artifacts
    version     Show version
    help        Show help
```

---

## 82. Environment variables

Future CLI configuration can support environment variables.

Possible examples:

```text
VITTE_COLOR
VITTE_TARGET
VITTE_JOBS
VITTE_HOME
VITTE_CACHE
VITTE_REGISTRY
```

Precedence should be explicit.

A possible policy:

```text
built-in defaults
        <
configuration file
        <
environment
        <
command line
```

Command-line arguments should generally have the highest explicit precedence.

---

## 83. Configuration files

Project configuration should not be manually reimplemented in CLI parsing.

The CLI should identify:

```text
manifest path
profile
target
package
```

and allow a dedicated configuration subsystem to merge project settings.

---

## 84. Reproducibility

CLI configuration participates in reproducible builds.

A complete build identity may eventually combine:

```text
compiler version
CLI semantic fingerprint
manifest fingerprint
lock-file fingerprint
source fingerprints
target
backend version
runtime version
dependency graph
```

This can become the basis for incremental build caching.

---

## 85. Package manager architecture

The intended separation is:

```text
vitte install foo
       |
       v
      CLI
       |
       v
     driver
       |
       v
package manager
       |
       +--> registry
       +--> resolver
       +--> cache
       +--> lock file
       +--> installer
```

The CLI must not contain registry protocol logic.

---

## 86. Security

Command-line input is untrusted input.

The parser must therefore protect against:

```text
integer overflow
allocation overflow
NULL arguments
unexpected state transitions
unbounded argument counts
invalid enum values
ambiguous option parsing
silent conflicting configuration
```

Paths and package names must not be interpreted as shell commands by the CLI parser.

---

## 87. No shell execution

The CLI parser must never construct shell commands from raw arguments.

For example, a future driver invoking a compiler should prefer structured process APIs rather than:

```text
system("cc ... user input ...")
```

This avoids shell-injection vulnerabilities.

The parser itself only stores argument data.

---

## 88. Path policy

The CLI should avoid prematurely canonicalizing paths.

Why:

- files may not exist yet;
- paths may be relative to a project root;
- virtual filesystems may be introduced;
- symlink semantics belong to filesystem/project policy;
- cross-platform path handling differs.

The CLI records user input.

A filesystem/project layer resolves it.

---

## 89. Unicode

Arguments should be treated as byte strings unless the platform abstraction guarantees a specific encoding.

The CLI should not silently corrupt non-ASCII paths.

Future Windows support may require a dedicated UTF-16 entry-point adapter before arguments reach the canonical UTF-8 compiler representation.

---

## 90. Windows

A future Windows frontend may convert:

```text
wchar_t argv
```

into canonical UTF-8 arguments before calling the generic CLI parser.

The core parser should remain independent from Windows-specific process APIs.

---

## 91. Thread safety

A single `vitte_cli_t` instance is not designed for concurrent mutation.

Treat it as externally synchronized.

Independent CLI instances can safely be used independently once surrounding allocator/runtime assumptions permit it.

---

## 92. Testing

The CLI requires extensive unit testing.

Minimum command tests:

```text
build
run
check
test
fmt
install
uninstall
update
clean
version
help
implicit build
```

Minimum option tests:

```text
--backend
--opt
--target
--output
-o
--manifest-path
--jobs
-j
--max-errors
--diagnostics
--color
--package
-p
--filter
--release
--debug
--verbose
--quiet
--source-map
--no-source-map
--emit-comments
--no-runtime-checks
--no-bounds-checks
--no-null-checks
--no-overflow-checks
-Werror
--deterministic
--no-deterministic
--incremental
--no-incremental
--offline
--locked
--frozen
--force
--check
--write
```

---

## 93. Error tests

Test every error class.

Examples:

```text
missing option value
invalid backend
invalid optimization
invalid color
invalid diagnostic format
jobs = 0
jobs overflow
max-errors = 0
duplicate target
duplicate backend
release + debug
verbose + quiet
too many inputs
too many forwarded arguments
unknown option
missing required input
```

---

## 94. `--` tests

Test:

```sh
vitte run main.vit -- a b c
```

and:

```sh
vitte test -- --verbose
```

Also test non-executing commands where `--` terminates option parsing.

---

## 95. Fingerprint tests

Equivalent semantic input must produce the same fingerprint.

Repeated parsing of:

```sh
vitte build main.vit --backend c17 --target aarch64-linux
```

should produce the same semantic fingerprint across runs.

The fingerprint must not depend on ASLR or heap layout.

---

## 96. Fuzzing

The parser is an excellent fuzzing target.

Fuzz:

```text
argument counts
empty arguments
very long arguments
random option sequences
random `=` placement
numeric overflow
repeated options
multiple `--`
Unicode byte sequences
command/option mixtures
```

Assertions should verify that malformed input never causes:

```text
out-of-bounds access
use-after-free
double free
integer overflow
invalid state corruption
undefined behavior
```

---

## 97. Sanitizers

Regularly test with:

```text
AddressSanitizer
UndefinedBehaviorSanitizer
```

A standalone Makefile should provide:

```sh
make asan
make ubsan
make sanitize
```

---

## 98. Strict C17

The CLI is designed for ISO C17.

Recommended strict development command:

```sh
make CC=clang WERROR=1 check
```

The subsystem should compile without warnings.

---

## 99. Enum policy

Vitte compiler infrastructure uses explicit exhaustive enum handling.

Recommended warning policy:

```text
-Wswitch-enum
```

and with Clang:

```text
-Wcovered-switch-default
```

Do not combine this design with mandatory redundant defaults through:

```text
-Wswitch-default
```

Exhaustive enum switches should explicitly handle sentinel values such as:

```text
INVALID
COUNT
```

where those values are part of the enum.

---

## 100. Header authority

`cli.h` is the single authority for the public CLI API.

Therefore `cli.c` should begin with:

```c
#include "cli.h"
```

and must not duplicate public definitions already declared by the header.

Public definitions belonging only in `cli.h` include:

```text
constants
public enums
public structures
public callback types
public function prototypes
public inline helpers
```

This avoids API drift between implementation and header.

---

## 101. Header self-containment

This must compile independently:

```c
#include "cli.h"

int
main(void)
{
    return 0;
}
```

A CLI Makefile should provide:

```sh
make check-header
```

to enforce this property.

---

## 102. Include-path safety

Never add the repository directory:

```text
include/vitte
```

directly as a system-style include root.

Doing so can cause project headers such as:

```text
string.h
```

to shadow the platform C library:

```c
#include <string.h>
```

Use public repository include roots instead.

---

## 103. Recommended standalone structure

```text
src/src/cli/
├── cli.c
├── cli.h
├── Makefile
├── README.md
└── tests/
    ├── test_cli.c
    ├── test_commands.c
    ├── test_options.c
    ├── test_errors.c
    ├── test_forward.c
    ├── test_fingerprint.c
    └── test_limits.c
```

As the subsystem grows, implementation can be split further.

---

## 104. Future split

A mature CLI may evolve toward:

```text
cli/
├── cli.c
├── cli.h
├── command.c
├── command.h
├── option.c
├── option.h
├── parser.c
├── parser.h
├── value.c
├── value.h
├── help.c
├── help.h
├── completion.c
├── completion.h
├── suggestion.c
├── suggestion.h
├── fingerprint.c
├── fingerprint.h
├── stats.c
├── stats.h
├── Makefile
└── README.md
```

The split should happen when responsibilities become large enough to justify separate modules.

---

## 105. Command registry

A future command registry can become the single source of truth for commands.

Example concept:

```text
build
    category: compiler
    accepts inputs: yes
    accepts forwarded args: no

run
    category: compiler
    accepts inputs: yes
    accepts forwarded args: yes

test
    category: test
    accepts inputs: optional
    accepts forwarded args: yes

install
    category: package
    accepts package: yes
```

This metadata can drive parsing, help and completion.

---

## 106. Option scope

Not every option should eventually be accepted by every command.

For example:

```text
--backend
    build/run/check/test

--filter
    test

--write
    fmt

--offline
    package/project operations

--output
    build
```

The initial parser may be permissive while architecture is being established.

A mature parser should validate option scope explicitly.

---

## 107. Option provenance

Future configuration merging should track where a value came from.

Example:

```text
optimization = speed
source       = command line
argument     = --opt=speed
```

or:

```text
target       = aarch64-linux
source       = project manifest
```

This makes configuration diagnostics much easier to understand.

---

## 108. Canonical normalized configuration

The long-term CLI pipeline should be:

```text
raw argv
   |
   v
syntactic parsing
   |
   v
raw CLI options
   |
   v
normalization
   |
   v
configuration merge
   |
   v
semantic validation
   |
   v
canonical driver request
```

This separates syntax from semantic project configuration.

---

## 109. Driver request

Eventually the CLI configuration can be converted into a dedicated driver request.

Conceptually:

```c
typedef struct vitte_driver_request {
    command;
    inputs;
    target;
    backend;
    optimization;
    diagnostics;
    safety;
    package_policy;
    output;
} vitte_driver_request_t;
```

The driver then does not need to understand raw `argv`.

---

## 110. Main executable architecture

The final executable entry point should remain small.

Ideal architecture:

```text
main.c
  |
  +--> cli_init
  |
  +--> cli_parse
  |
  +--> driver_init
  |
  +--> driver_execute
  |
  +--> driver_destroy
  |
  +--> cli_destroy
  |
  +--> exit
```

`main.c` should not become a second CLI parser.

---

## 111. Compiler pipeline integration

For:

```sh
vitte build main.vit
```

the full flow should become:

```text
argv
 |
 v
CLI
 |
 v
driver request
 |
 v
source manager
 |
 v
lexer
 |
 v
parser
 |
 v
AST
 |
 v
semantic analysis
 |
 v
HIR
 |
 v
IR
 |
 v
optimization
 |
 v
codegen
 |
 v
C17 backend
 |
 v
generated C
 |
 v
C compiler
 |
 v
linker
 |
 v
program
```

---

## 112. Check pipeline

For:

```sh
vitte check main.vit
```

the driver can stop before final artifact generation:

```text
source
 |
 v
frontend
 |
 v
semantic validation
 |
 v
diagnostics
 |
 X
```

This keeps `check` fast while sharing the same compiler frontend.

---

## 113. Run pipeline

For:

```sh
vitte run main.vit -- hello
```

the driver flow becomes:

```text
CLI
 |
 v
build
 |
 v
temporary/final executable
 |
 v
execute
 |
 v
forward:
    "hello"
```

Program exit status should be propagated according to driver policy.

---

## 114. Error ownership

CLI errors describe malformed command invocation.

Compiler errors describe malformed Vitte programs.

Backend errors describe generation failures.

Toolchain errors describe downstream compiler/linker failures.

These categories should remain distinguishable.

Conceptually:

```text
CLI error
compiler diagnostic
backend diagnostic
toolchain diagnostic
runtime exit
```

They should not all collapse into one generic error string.

---

## 115. Future error namespaces

CLI diagnostics may eventually receive a dedicated namespace such as:

```text
ECLI0001
ECLI0002
...
```

This would allow:

```sh
vitte explain ECLI0002
```

if Vitte later implements diagnostic explanation infrastructure across compiler subsystems.

---

## 116. Machine-readable CLI errors

Structured CLI errors can eventually be rendered as JSON.

Example concept:

```json
{
  "kind": "cli-error",
  "code": "ECLI0004",
  "argument_index": 3,
  "argument": "llvm",
  "message": "unknown backend",
  "expected": [
    "default",
    "c17",
    "native",
    "bytecode"
  ]
}
```

The parser itself should not need JSON-specific logic.

---

## 117. Performance

CLI parsing should remain negligible compared with compilation.

Desired complexity for ordinary parsing is approximately:

```text
O(number of arguments)
```

Some current duplicate checks are constant-time through bit flags.

Future command/option registries should preserve efficient lookup.

---

## 118. Allocation strategy

The current implementation dynamically allocates arrays for:

```text
inputs
forwarded arguments
```

Future optimization may use:

```text
small-buffer storage
arena allocation
caller-provided storage
single allocation
```

but only if complexity is justified.

Correctness and API clarity come first.

---

## 119. No unnecessary string copies

The parser currently borrows `argv` strings.

This keeps parsing lightweight.

If future requirements need configuration to outlive `argv`, an owned CLI snapshot can be introduced explicitly rather than silently changing ownership semantics.

---

## 120. Stable API principle

Public API changes should preserve a clear distinction between:

```text
parsing
normalization
driver execution
diagnostic rendering
```

Avoid exposing backend internals through `cli.h`.

The CLI API should describe user intent, not concrete compiler implementation details.

---

## 121. Design principles

The CLI subsystem follows these principles.

### Strict parsing

Invalid options are rejected rather than silently ignored.

### Explicit conflicts

Contradictory options produce errors.

### Checked arithmetic

Argument storage cannot silently overflow.

### Determinism

Semantic configuration can be fingerprinted reproducibly.

### Borrowed input

Raw argument strings are not copied unnecessarily.

### Structured errors

Failures are represented as data.

### Separation of concerns

Parsing does not perform compilation or package management.

### Backend neutrality

CLI policy does not generate backend syntax.

### Cross-platform architecture

Target selection remains independent from the host.

### Small entry point

`main()` should delegate rather than accumulate compiler logic.

---

## 122. Current implementation status

The initial CLI implementation provides:

- explicit lifecycle;
- command parsing;
- implicit build;
- positional inputs;
- forwarded arguments;
- backend selection;
- optimization selection;
- target selection;
- output selection;
- manifest selection;
- package selection;
- test filtering;
- jobs;
- error limits;
- diagnostic formats;
- color policy;
- release/debug modes;
- verbosity;
- debug-info policy;
- source-map policy;
- runtime safety switches;
- warnings-as-errors;
- deterministic mode;
- incremental mode;
- offline mode;
- locked mode;
- frozen mode;
- force mode;
- formatter modes;
- duplicate-option detection;
- conflict detection;
- checked allocation;
- checked numeric parsing;
- deterministic FNV-1a fingerprinting;
- parser statistics;
- structured errors;
- reset/reuse;
- explicit destruction.

---

## 123. Immediate integration work

The implementation should next be synchronized so that:

```c
#include "cli.h"
```

is the first project header used by `cli.c`.

Public definitions must exist only in `cli.h`.

Remove duplicated public declarations from `cli.c`, including:

```text
public constants
public enums
public structures
public prototypes
```

Private implementation details remain in `cli.c`.

Then validate with strict compilation.

---

## 124. Recommended validation

Once `cli.c` and `cli.h` are synchronized:

```sh
cd /Users/vincent/Documents/Github/vitte/src/src/cli
```

Run:

```sh
make CC=clang WERROR=1 check
```

The objective is:

```text
0 errors
0 warnings
```

under the project's strict C17 warning policy.

---

## 125. Mature CLI architecture

Long-term architecture:

```text
                           +----------------+
                           | process / argv |
                           +-------+--------+
                                   |
                                   v
                           +----------------+
                           | CLI tokenizer  |
                           +-------+--------+
                                   |
                                   v
                           +----------------+
                           | option parser  |
                           +-------+--------+
                                   |
                                   v
                           +----------------+
                           | normalization  |
                           +-------+--------+
                                   |
                                   v
                           +----------------+
                           | configuration  |
                           | merge          |
                           +-------+--------+
                                   |
                                   v
                           +----------------+
                           | validation     |
                           +-------+--------+
                                   |
                                   v
                           +----------------+
                           | driver request |
                           +-------+--------+
                                   |
             +---------------------+----------------------+
             |                     |                      |
             v                     v                      v
       compiler driver       package manager          formatter
             |
             v
          frontend
             |
             v
             IR
             |
             v
          codegen
             |
             v
           C17
```

---

## 126. Core rule

The most important boundary is:

```text
argv
  |
  v
CLI
  |
  v
structured user intent
  |
  v
driver
```

The CLI parses intent.

It does not execute compiler architecture.

Keeping that boundary strict makes the Vitte command-line interface easier to test, safer to evolve, and reusable by future frontends such as IDEs, build servers and graphical tools.
EOF