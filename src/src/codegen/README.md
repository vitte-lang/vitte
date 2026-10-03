# Vitte Codegen

`src/src/codegen` is the generic code-generation orchestration layer of the Vitte compiler.

It connects lowered compiler representations to concrete output backends while keeping backend-specific implementation details outside the generic compiler pipeline.

---

## 1. Architecture

```text
Vitte source
     |
     v
   Lexer
     |
     v
   Parser
     |
     v
    AST
     |
     v
Semantic analysis
     |
     v
 HIR / IR
     |
     v
+--------------------+
|      Codegen       |
|  src/src/codegen/  |
+--------------------+
     |
     +----------------------+----------------------+
     |                      |                      |
     v                      v                      v
 C17 backend          Native backend         Bytecode backend
     |                   (future)                (future)
     v
 ISO C17
     |
     v
C compiler
     |
     v
Object / executable
```

The codegen layer does not generate C syntax itself.

The current concrete C17 implementation belongs to:

```text
src/src/backend/c17/
```

This separation prevents the generic compiler pipeline from becoming coupled to one output language or machine architecture.

---

## 2. Directory

```text
codegen/
├── codegen.c
├── codegen.h
├── Makefile
└── README.md
```

### `codegen.h`

Defines the public code-generation API:

- lifecycle;
- configuration;
- targets;
- architectures;
- operating systems;
- ABIs;
- endianness;
- optimization levels;
- code-generation units;
- backend interface;
- diagnostic callbacks;
- statistics;
- deterministic fingerprints;
- cancellation;
- output accounting.

### `codegen.c`

Implements the generic orchestration engine.

It owns no AST, HIR, IR or backend-specific representation.

### `Makefile`

Provides a strict standalone ISO C17 build environment.

---

## 3. Responsibilities

Codegen is responsible for:

1. validating code-generation configuration;
2. validating target information;
3. managing the code-generation lifecycle;
4. registering lowered compilation units;
5. preventing duplicate unit identifiers;
6. enforcing unit limits;
7. binding a concrete backend;
8. invoking backend preparation;
9. starting generation;
10. emitting units;
11. finalizing the backend;
12. propagating backend failures;
13. tracking diagnostics;
14. enforcing error limits;
15. enforcing output limits;
16. supporting cancellation;
17. collecting statistics;
18. computing deterministic fingerprints;
19. protecting lifecycle invariants;
20. providing a stable boundary between IR and backend code.

Codegen is deliberately not responsible for:

- lexical analysis;
- parsing;
- AST construction;
- name resolution;
- type checking;
- semantic analysis;
- HIR ownership;
- IR ownership;
- optimization implementation;
- C17 syntax generation;
- assembly generation;
- object-file encoding;
- linking;
- diagnostic rendering;
- source-file loading;
- package management.

---

## 4. Compiler pipeline

The intended compiler pipeline is:

```text
source
  |
  v
lexer
  |
  v
tokens
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
backend
  |
  v
generated representation
```

For the C17 backend:

```text
IR
 |
 v
codegen
 |
 v
backend/c17
 |
 v
translation unit
 |
 v
ISO C17 source
 |
 v
system C compiler
 |
 v
object file
 |
 v
linker
 |
 v
executable
```

---

## 5. Lifecycle

The normal lifecycle is:

```text
vitte_codegen_init()
        |
        v
      READY
        |
        +----------------------+
        |                      |
        v                      v
vitte_codegen_add_unit()  vitte_codegen_set_backend()
        |
        v
vitte_codegen_prepare()
        |
        v
     PREPARED
        |
        v
vitte_codegen_generate()
        |
        v
   GENERATING
        |
        +--> backend.begin()
        |
        +--> backend.emit_unit()
        |
        +--> backend.emit_unit()
        |
        +--> ...
        |
        +--> backend.end()
        |
        v
     FINISHED
        |
        +------------------+
        |                  |
        v                  v
     reset()           destroy()
        |                  |
        v                  v
      READY            DESTROYED
```

Failure path:

```text
READY / PREPARED / GENERATING
             |
             v
           error
             |
             v
           FAILED
```

Cancellation path:

```text
READY / PREPARED / GENERATING
             |
             v
          cancel
             |
             v
         CANCELLED
```

---

## 6. States

The public state machine is represented by:

```c
typedef enum vitte_codegen_state {
    VITTE_CODEGEN_STATE_INVALID = 0,
    VITTE_CODEGEN_STATE_READY,
    VITTE_CODEGEN_STATE_PREPARED,
    VITTE_CODEGEN_STATE_GENERATING,
    VITTE_CODEGEN_STATE_FINISHED,
    VITTE_CODEGEN_STATE_FAILED,
    VITTE_CODEGEN_STATE_CANCELLED,
    VITTE_CODEGEN_STATE_DESTROYED,
    VITTE_CODEGEN_STATE_COUNT
} vitte_codegen_state_t;
```

### `READY`

The generator is initialized and can accept units and backend configuration.

### `PREPARED`

The configuration and backend have been prepared.

No additional units should be registered.

### `GENERATING`

A backend is actively generating output.

### `FINISHED`

Generation completed successfully.

### `FAILED`

A fatal generation error occurred.

### `CANCELLED`

Generation was explicitly cancelled.

### `DESTROYED`

Resources have been released and the object must no longer be used.

---

## 7. Backend abstraction

The generic codegen layer communicates with concrete backends through callbacks.

```c
typedef struct vitte_codegen_backend_interface {
    vitte_codegen_backend_prepare_fn prepare;
    vitte_codegen_backend_begin_fn begin;
    vitte_codegen_backend_emit_unit_fn emit_unit;
    vitte_codegen_backend_end_fn end;
    vitte_codegen_backend_destroy_fn destroy;
} vitte_codegen_backend_interface_t;
```

The generic codegen implementation therefore does not need to know how C17, machine code or bytecode is physically emitted.

---

## 8. Backend lifecycle

A backend receives the following sequence:

```text
prepare()
   |
   v
begin()
   |
   v
emit_unit(unit 0)
   |
   v
emit_unit(unit 1)
   |
   v
...
   |
   v
emit_unit(unit N)
   |
   v
end()
```

`emit_unit` is mandatory.

The other callbacks are optional.

---

## 9. Current backend

The primary backend is:

```text
src/src/backend/c17/
```

Its responsibility is concrete ISO C17 generation.

The intended boundary is:

```text
codegen
   |
   | generic orchestration
   v
backend/c17
   |
   | C-specific lowering/emission
   v
writer
   |
   v
.c output
```

Codegen must not duplicate functionality already belonging to the C17 backend.

---

## 10. Code-generation units

A generation unit is represented by:

```c
typedef struct vitte_codegen_unit {
    uint64_t id;
    const char *name;
    const void *input;
    vitte_codegen_source_location_t location;
} vitte_codegen_unit_t;
```

### `id`

Stable non-zero semantic identifier.

Unit IDs must be unique inside a codegen instance.

### `name`

Canonical human-readable unit name.

The string is borrowed.

The caller must keep it alive for as long as codegen may access the unit.

### `input`

Opaque backend-ready representation.

Codegen deliberately does not interpret this pointer.

Depending on the compiler stage it can represent:

```text
HIR
MIR
IR
lowered module
backend IR
translation-unit description
```

### `location`

Optional source origin used for diagnostics and source mapping.

---

## 11. Source locations

Codegen uses:

```c
typedef struct vitte_codegen_source_location {
    uint32_t file_id;
    size_t begin;
    size_t end;
    bool valid;
} vitte_codegen_source_location_t;
```

Offsets are represented independently from line/column information.

This allows the source manager and diagnostic subsystem to remain responsible for translating offsets into user-facing locations.

---

## 12. Target model

A target contains:

```text
architecture
operating system
ABI
endianness
pointer width
size_t width
character width
hosted/freestanding state
```

Represented by:

```c
typedef struct vitte_codegen_target {
    vitte_codegen_arch_t architecture;
    vitte_codegen_os_t operating_system;
    vitte_codegen_abi_t abi;
    vitte_codegen_endian_t endian;

    unsigned pointer_bits;
    unsigned size_bits;
    unsigned char_bits;

    bool hosted;
} vitte_codegen_target_t;
```

---

## 13. Architectures

The current generic architecture model defines:

```text
x86_64
aarch64
riscv64
wasm32
```

These values describe target capabilities.

Their presence does not mean that every architecture already has a native backend.

---

## 14. Operating systems

The target model currently contains:

```text
Linux
macOS
Windows
FreeBSD
none
```

`none` represents targets without a conventional hosted operating system.

---

## 15. ABI

Current ABI identifiers:

```text
SYSV
WIN64
AAPCS64
WASM
FREESTANDING
```

ABI-specific classification and lowering should remain outside the generic codegen orchestrator.

For the C17 backend, detailed ABI logic belongs under:

```text
src/src/backend/c17/
```

---

## 16. Endianness

The target explicitly records:

```text
little endian
big endian
```

Backends must not infer target endianness from the machine running the compiler.

This is required for correct cross-compilation.

---

## 17. Cross-compilation

A fundamental rule is:

```text
host != target
```

Codegen must therefore avoid deriving semantic target behavior from host properties after the target has been configured.

For example:

```text
Apple Silicon host
      |
      v
Vitte compiler
      |
      +--> Linux x86_64
      |
      +--> Linux AArch64
      |
      +--> Windows x86_64
      |
      +--> WASM32
```

The host compiler is not necessarily the target compiler environment.

---

## 18. Optimization policy

Generic optimization levels are:

```text
NONE
DEBUG
BALANCED
SPEED
SIZE
AGGRESSIVE
```

These describe policy.

They do not require every backend to implement identical optimization passes.

Optimization passes themselves should live in the appropriate IR/optimization subsystem rather than inside generic codegen.

---

## 19. Configuration

The generic configuration contains:

```c
typedef struct vitte_codegen_config {
    vitte_codegen_backend_kind_t backend;
    vitte_codegen_optimization_t optimization;

    size_t max_units;
    size_t max_errors;
    size_t max_output_bytes;

    bool deterministic;

    bool emit_debug_info;
    bool emit_source_map;
    bool emit_comments;

    bool runtime_checks;
    bool bounds_checks;
    bool null_checks;
    bool overflow_checks;

    bool fail_fast;
} vitte_codegen_config_t;
```

The default policy favors correctness.

In particular, runtime safety checks should not silently disappear merely because release optimization is enabled.

---

## 20. Determinism

Deterministic generation is a core design objective.

Given equivalent:

```text
compiler version
configuration
target
input
unit order
backend
```

generation should produce equivalent output.

Codegen must avoid using process-specific information such as pointer addresses as stable identifiers.

---

## 21. Fingerprints

Codegen exposes:

```c
uint64_t
vitte_codegen_fingerprint(
    const vitte_codegen_t *codegen);
```

The fingerprint uses deterministic FNV-1a hashing.

The current fingerprint includes properties such as:

```text
backend
optimization policy
target architecture
target OS
target ABI
endianness
pointer width
size width
unit IDs
unit names
```

Opaque `input` pointer addresses are deliberately excluded.

Pointer addresses are process-specific and would make fingerprints non-deterministic.

---

## 22. Diagnostics

A backend can report diagnostics through the codegen diagnostic callback.

Severity levels are:

```text
NOTE
WARNING
ERROR
```

Codegen tracks diagnostic counts independently from presentation.

Rendering belongs to the diagnostic subsystem.

Possible output formats elsewhere in Vitte include:

```text
terminal
JSON
SARIF
LSP
```

Codegen should not know how those formats are rendered.

---

## 23. Diagnostic flow

Intended flow:

```text
backend
   |
   v
codegen diagnostic callback
   |
   v
diagnostic engine
   |
   +--> terminal
   |
   +--> JSON
   |
   +--> SARIF
   |
   +--> LSP
```

This separation makes backend errors available to IDEs and automated tooling without parsing terminal text.

---

## 24. Error limits

The configuration provides:

```text
max_errors
```

Generation can stop once the configured error threshold is reached.

This prevents cascades containing thousands of secondary diagnostics.

---

## 25. Fail-fast mode

When:

```text
fail_fast = true
```

generation can stop after the first generation error.

This is useful for:

- compiler development;
- debugging;
- fuzzing;
- invariant testing.

Normal user-facing compilation can retain multiple diagnostics.

---

## 26. Output accounting

Concrete backends can report committed output with:

```c
bool
vitte_codegen_account_output(
    vitte_codegen_t *codegen,
    size_t byte_count);
```

This provides global output accounting independent from the backend writer.

It protects against:

- accidental runaway generation;
- malformed compiler input;
- pathological generated output;
- arithmetic overflow.

---

## 27. Output limits

When:

```text
max_output_bytes == 0
```

the generic layer treats output size as unlimited.

When non-zero, committed output must not exceed the configured limit.

Concrete writers may additionally enforce stricter local limits.

---

## 28. Cancellation

Code generation can be cancelled with:

```c
vitte_codegen_cancel(codegen);
```

Cancellation transitions the generator to:

```text
VITTE_CODEGEN_STATE_CANCELLED
```

This is important for:

- IDE builds;
- language servers;
- editor integration;
- parallel builds;
- user-interrupted compilation;
- future build servers.

---

## 29. Statistics

Codegen tracks:

```text
unit count
generated unit count
error count
warning count
note count
output bytes
input fingerprint
prepared state
generated state
failure state
cancellation state
```

Statistics are available through:

```c
vitte_codegen_stats_t
vitte_codegen_stats(
    const vitte_codegen_t *codegen);
```

---

## 30. Memory ownership

Codegen owns:

```text
its internal unit array
its lifecycle state
its statistics
its configuration copy
its target copy
```

Codegen does not own:

```text
unit names
unit input representations
source manager
AST
HIR
IR
diagnostic engine
```

Backend data ownership is controlled by the backend interface.

If a backend provides:

```c
destroy()
```

that callback is invoked when the codegen instance is destroyed.

---

## 31. Reset

A completed, failed or cancelled codegen instance can be reset when allowed by the implementation.

Reset returns the instance to:

```text
READY
```

while preserving reusable allocated storage where appropriate.

This allows repeated compilation without unnecessary allocation churn.

---

## 32. Generation counter

The codegen object contains a generation counter.

It can be used by future infrastructure to detect stale external references.

Conceptually:

```text
generation 1
    |
   reset
    |
generation 2
    |
   reset
    |
generation 3
```

External handles can eventually pair an object ID with a generation number.

---

## 33. Error model

Representative errors include:

```text
INVALID_CODEGEN
INVALID_ARGUMENT
INVALID_STATE
INVALID_CONFIG
INVALID_TARGET
INVALID_BACKEND

DUPLICATE_UNIT
TOO_MANY_UNITS
TOO_MANY_ERRORS

OUTPUT
OUTPUT_LIMIT

OVERFLOW
OUT_OF_MEMORY

UNSUPPORTED
CANCELLED

BACKEND
INTERNAL
CORRUPTION
```

Errors are semantic identifiers, not final user-facing messages.

---

## 34. Integer safety

Capacity and output arithmetic must be checked before operations such as:

```text
a + b
a * b
capacity * sizeof(element)
output_bytes + emitted_bytes
```

Unchecked `size_t` overflow is not acceptable in compiler infrastructure.

---

## 35. Unit capacity

Unit storage grows dynamically.

Growth must:

1. check arithmetic overflow;
2. respect `max_units`;
3. preserve existing units;
4. report allocation failure;
5. never silently wrap capacity.

---

## 36. Duplicate units

Every registered unit must have a unique non-zero ID.

Duplicate unit IDs are rejected.

Future versions may additionally maintain an indexed registry to replace linear lookup for very large programs.

---

## 37. Backend isolation

Generic codegen must never contain C17-specific syntax such as:

```text
#include
typedef
struct declarations
C declarators
C precedence rules
C string escaping
C compiler flags
```

Those belong in:

```text
backend/c17/
```

Likewise, native instruction encoding must not be added to generic codegen.

---

## 38. C17 integration

The intended relationship is:

```text
codegen/codegen.c
        |
        v
backend adapter
        |
        v
backend/c17/backend.c
        |
        v
backend/c17/translation_unit.c
        |
        v
backend/c17/writer.c
        |
        v
generated C17
```

The adapter converts the generic backend interface into C17 backend operations.

---

## 39. C17 backend architecture

The C17 backend is expected to remain split into specialized components such as:

```text
backend/c17/
├── backend.c
├── backend.h
├── target.c
├── target.h
├── writer.c
├── writer.h
├── section.c
├── section.h
├── identifier.c
├── identifier.h
├── literal.c
├── literal.h
├── declarator.c
├── declarator.h
├── type.c
├── type.h
├── expression.c
├── expression.h
├── statement.c
├── statement.h
├── declaration.c
├── declaration.h
├── translation_unit.c
├── translation_unit.h
├── runtime.c
├── runtime.h
├── include.c
├── include.h
├── symbol.c
├── symbol.h
├── temporary.c
├── temporary.h
├── abi.c
├── abi.h
├── source_map.c
├── source_map.h
├── compiler_diagnostic.c
├── compiler_diagnostic.h
├── naming.c
├── naming.h
├── options.c
├── options.h
├── program.c
├── program.h
├── stats.c
└── stats.h
```

Generic codegen must orchestrate this infrastructure rather than duplicate it.

---

## 40. Source maps

When:

```text
emit_source_map = true
```

backends should preserve relationships between generated output and original Vitte source locations.

Conceptually:

```text
Vitte source span
       |
       v
      IR
       |
       v
    codegen
       |
       v
generated C range
```

This mapping is required for useful diagnostics when a downstream C compiler reports an error.

---

## 41. Reverse diagnostics

For the C17 pipeline, the long-term objective is:

```text
generated C compiler diagnostic
            |
            v
generated C source location
            |
            v
C17 source map
            |
            v
original Vitte source span
            |
            v
Vitte diagnostic
```

The user should normally see errors in terms of Vitte source rather than generated C internals.

---

## 42. Debug information

`emit_debug_info` represents generic policy.

The exact implementation is backend-specific.

Possible future implementations include:

```text
#line mappings
DWARF metadata
CodeView metadata
custom Vitte debug metadata
source-map sidecar files
```

---

## 43. Runtime safety

Generic codegen configuration exposes:

```text
runtime_checks
bounds_checks
null_checks
overflow_checks
```

These are policy switches.

The backend or lowering pipeline decides how the checks are represented.

Release optimization must not automatically imply memory-unsafety.

---

## 44. Unsupported features

A backend should explicitly report unsupported operations rather than silently emitting incorrect output.

Use:

```text
VITTE_CODEGEN_ERROR_UNSUPPORTED
```

for features that are valid in the compiler but unavailable for a particular target/backend combination.

---

## 45. Internal compiler failures

Compiler invariant failures should remain distinguishable from user program errors.

The generic codegen layer provides:

```text
INTERNAL
CORRUPTION
```

These should eventually integrate with Vitte's compiler diagnostic namespace.

They must not be presented as ordinary source errors when the compiler itself is at fault.

---

## 46. Threading

The current codegen object should be treated as externally synchronized.

Do not concurrently mutate the same:

```c
vitte_codegen_t
```

from multiple threads unless explicit synchronization is added.

Independent codegen instances can be used for future parallel compilation.

---

## 47. Future parallel generation

The architecture should eventually permit:

```text
module A ----\
module B -----+--> parallel lowering --> ordered final emission
module C -----+
module D ----/
```

Deterministic output must remain preserved even when internal work is parallelized.

Parallel execution must therefore not make output ordering dependent on thread scheduling.

---

## 48. Future incremental compilation

The deterministic fingerprint infrastructure can support future incremental compilation.

Conceptually:

```text
unit
 |
 +--> source hash
 +--> semantic hash
 +--> target hash
 +--> options hash
 +--> compiler version
 |
 v
cache key
```

A cache hit could bypass expensive lowering or generation work.

---

## 49. Future backend capabilities

A future backend capability model may expose properties such as:

```text
supports_debug_info
supports_source_maps
supports_native_objects
supports_threads
supports_atomics
supports_exceptions
supports_async
supports_vector_types
supports_inline_assembly
supports_freestanding
```

Generic codegen should validate requested features against backend capabilities before generation starts.

---

## 50. Future unit dependency graph

The current unit list can evolve into a dependency-aware graph:

```text
module A
  |
  +--> module B
  |
  +--> module C
          |
          +--> module D
```

Generation order should then be computed deterministically.

Cycles must be diagnosed explicitly when the relevant dependency relation forbids them.

---

## 51. Future symbol integration

Codegen should eventually integrate with a global symbol registry.

That registry can track:

```text
Vitte symbol ID
source name
generated name
module
visibility
linkage
ABI
type
definition/declaration state
```

Generated backend identifiers should remain under the authority of the backend naming subsystem.

---

## 52. Future ABI integration

Generic codegen records the selected ABI.

Detailed ABI lowering should remain specialized.

Examples:

```text
SysV AMD64
Win64
AAPCS64
WASM
freestanding
```

ABI logic can determine:

```text
argument classification
return classification
register usage
stack layout
aggregate passing
alignment
calling convention
variadic behavior
```

---

## 53. Future IR contract

The opaque `input` pointer is intentionally generic during early architecture development.

A mature implementation should define a precise backend-ready IR contract.

For example:

```text
HIR
 |
 v
MIR
 |
 v
backend-neutral IR
 |
 v
codegen
 |
 v
backend lowering
```

Once stabilized, codegen should consume a typed representation rather than relying indefinitely on an untyped `const void *`.

---

## 54. API invariants

Core invariants include:

```text
magic == VITTE_CODEGEN_MAGIC
unit_count <= unit_capacity
unit_count > 0 => units != NULL
unit IDs are non-zero
unit IDs are unique
backend emit_unit callback exists before generation
configuration is valid
target is valid
```

Violations indicate invalid API use or compiler corruption.

---

## 55. Strict C17 policy

The subsystem targets ISO C17.

The standalone Makefile enables strict warnings including:

```text
-Wall
-Wextra
-Wpedantic
-Wconversion
-Wsign-conversion
-Wshadow
-Wcast-align
-Wcast-qual
-Wwrite-strings
-Wundef
-Wformat=2
-Wstrict-prototypes
-Wmissing-prototypes
-Wmissing-declarations
-Wold-style-definition
-Wswitch-enum
-Wimplicit-fallthrough
-Wvla
-Wpointer-arith
-Wbad-function-cast
-Wstrict-overflow=5
```

Clang additionally enables:

```text
-Wdocumentation
-Wnewline-eof
-Wcomma
-Wconditional-uninitialized
-Wcovered-switch-default
```

`-Wswitch-default` is deliberately not enabled.

The project policy is to explicitly handle enum values, including sentinel values where appropriate, while allowing exhaustive switches without redundant `default` branches.

---

## 56. Header policy

`codegen.h` must be self-contained.

This must compile:

```c
#include "codegen.h"

int
main(void)
{
    return 0;
}
```

Run:

```sh
make check-header
```

to verify this invariant.

---

## 57. Include-path policy

Do not add:

```text
-I.../include/vitte
```

directly to compiler flags.

The repository contains Vitte headers whose names can collide with standard C headers.

In particular, a Vitte `string.h` must never shadow the platform:

```c
#include <string.h>
```

Use the repository include root instead.

The standalone Makefile provides:

```sh
make check-string-header
```

to inspect header resolution.

---

## 58. Build

Enter the directory:

```sh
cd /Users/vincent/Documents/Github/vitte/src/src/codegen
```

Build the debug archive:

```sh
make
```

or:

```sh
make debug
```

The archive is generated under the repository build directory.

---

## 59. Release build

```sh
make release
```

The release profile enables optimization while retaining the generic compiler safety architecture.

---

## 60. Strict validation

Run:

```sh
make CC=clang WERROR=1 check
```

This performs strict syntax and header validation.

Warnings are treated as errors.

This is the recommended command while developing the subsystem.

---

## 61. Sanitizers

Combined AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
make sanitize
```

AddressSanitizer:

```sh
make asan
```

UndefinedBehaviorSanitizer:

```sh
make ubsan
```

Build all profiles:

```sh
make profiles
```

---

## 62. Build information

Display configuration:

```sh
make info
```

Display compiler information:

```sh
make compiler-info
```

List source and header files:

```sh
make list
```

List archive members:

```sh
make archive-list
```

---

## 63. Cleaning

Clean generated codegen profiles:

```sh
make clean
```

Remove the complete codegen build directory:

```sh
make distclean
```

Rebuild debug:

```sh
make rebuild
```

Rebuild release:

```sh
make rebuild-release
```

---

## 64. Testing strategy

The subsystem should eventually include several testing layers.

### Unit tests

Test:

```text
configuration validation
target validation
state transitions
unit registration
duplicate detection
capacity growth
output accounting
error limits
cancellation
reset
fingerprints
backend callback ordering
```

### Golden tests

Verify stable generated results for known IR inputs.

### Failure injection

Simulate:

```text
allocation failure
backend prepare failure
backend begin failure
backend unit failure
backend end failure
output limit exhaustion
cancellation
```

### Sanitizers

Run:

```text
ASan
UBSan
```

regularly.

### Fuzzing

Fuzz:

```text
state transitions
unit registration
target configuration
backend callback behavior
malformed lowered IR adapters
```

---

## 65. Deterministic tests

Determinism should be tested by generating the same input repeatedly and comparing:

```text
fingerprint
output size
output bytes
symbol names
unit ordering
source maps
```

The result must not depend on:

```text
heap addresses
ASLR
thread scheduling
filesystem traversal order
hash-table randomization
host endianness
```

unless explicitly required by target semantics.

---

## 66. Performance

Codegen should remain lightweight compared with semantic analysis and backend lowering.

Important future metrics include:

```text
units generated
bytes generated
time per unit
peak memory
backend preparation time
backend finalization time
diagnostic count
source-map entry count
```

The generic layer should avoid unnecessary copies of large compiler representations.

---

## 67. Scalability

The current simple registry is suitable for initial compiler architecture.

For very large projects, future improvements may include:

```text
hash-indexed unit lookup
dependency graph indexing
arena-backed metadata
parallel preparation
parallel lowering
incremental fingerprints
persistent build cache
streamed output
```

These optimizations should preserve the public lifecycle contract.

---

## 68. Security and robustness

Code generation processes compiler-controlled and potentially adversarial source-derived structures.

The subsystem must therefore defend against:

```text
integer overflow
allocation overflow
invalid enum values
invalid state transitions
duplicate IDs
unbounded output
diagnostic explosions
backend failures
corrupt internal state
```

Compiler crashes should be treated as bugs.

Malformed user source must not be able to trigger undefined behavior in codegen.

---

## 69. Design principles

The codegen subsystem follows these principles:

### One responsibility per layer

Generic orchestration stays separate from concrete output generation.

### Determinism

Equivalent compiler input should produce equivalent compiler output.

### Explicit state

Lifecycle transitions are represented directly.

### Checked arithmetic

Size calculations must not silently overflow.

### Backend isolation

Backend-specific syntax stays inside backend directories.

### Cross-compilation correctness

Target properties must not be confused with host properties.

### Diagnostics as data

Diagnostics are structured events rather than formatted terminal strings.

### Safety before optimization

Optimization must not silently remove required language guarantees.

### Strict compilation

Warnings are treated as defects during subsystem development.

---

## 70. Intended mature architecture

The long-term architecture is:

```text
                           +------------------+
                           |   Vitte source   |
                           +--------+---------+
                                    |
                                    v
                           +------------------+
                           | Lexer / Parser   |
                           +--------+---------+
                                    |
                                    v
                           +------------------+
                           |       AST        |
                           +--------+---------+
                                    |
                                    v
                           +------------------+
                           | Semantic analysis|
                           +--------+---------+
                                    |
                                    v
                           +------------------+
                           |       HIR        |
                           +--------+---------+
                                    |
                                    v
                           +------------------+
                           |        IR        |
                           +--------+---------+
                                    |
                                    v
                           +------------------+
                           | Optimization     |
                           +--------+---------+
                                    |
                                    v
                         +----------------------+
                         |       Codegen        |
                         | generic orchestration|
                         +----------+-----------+
                                    |
              +---------------------+---------------------+
              |                     |                     |
              v                     v                     v
      +---------------+     +---------------+     +---------------+
      |  C17 backend  |     | Native backend|     |Bytecode backend|
      +-------+-------+     +-------+-------+     +-------+-------+
              |                     |                     |
              v                     v                     v
          ISO C17               object code             VM code
              |
              v
        system compiler
              |
              v
          executable
```

The generic `codegen` directory is the orchestration boundary in the middle of this architecture.

It should remain small, deterministic, backend-neutral and strict.

---

## 71. Current status

The initial codegen implementation provides the foundation for:

- lifecycle management;
- generic backend callbacks;
- target description;
- unit registration;
- deterministic fingerprints;
- output accounting;
- diagnostics;
- statistics;
- cancellation;
- error limits;
- strict standalone compilation.

The next integration work should focus on:

1. making `codegen.c` include `codegen.h` as the single public API authority;
2. removing duplicated public type declarations from `codegen.c`;
3. adding a C17 backend adapter;
4. connecting codegen units to the real Vitte IR;
5. connecting diagnostics to the central diagnostic engine;
6. connecting source locations to the source manager;
7. adding unit and state-machine tests;
8. adding failure-injection tests;
9. adding sanitizer CI;
10. validating deterministic generation across repeated builds.

---

## 72. Core rule

The central architectural rule is:

```text
Codegen decides WHEN and WHAT to generate.

A backend decides HOW to represent it.
```

Keeping this boundary strict allows Vitte to evolve beyond C17 without redesigning the entire compiler pipeline.
