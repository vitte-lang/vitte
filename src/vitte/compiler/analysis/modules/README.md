# Vitte Compiler — Module Analysis

The `analysis/modules` directory implements the module subsystem of the Vitte
compiler.

It is responsible for representing modules, resolving module names and paths,
tracking imports and exports, building the module dependency graph, detecting
cycles, computing compilation order, validating visibility rules, maintaining
module caches and collecting module statistics.

The public facade of the subsystem is:

```text
module.vit
```

## Overview

The module subsystem sits between parsing and the later semantic analysis
stages.

```text
Source files
    |
    v
Module parser
    |
    v
Module paths / names
    |
    v
Module registry
    |
    v
Module database
    |
    +-------------------+
    |                   |
    v                   v
Imports              Exports
    |                   |
    +---------+---------+
              |
              v
        Module resolver
              |
              v
       Dependency graph
              |
              v
        Cycle detection
              |
              v
      Compilation order
              |
              v
     Visibility checking
              |
              v
        Module validation
              |
              v
        Semantic analysis
```

## Responsibilities

The subsystem provides:

- module identity;
- module names;
- canonical module paths;
- namespaces;
- module metadata;
- module kinds;
- module registration;
- module lookup;
- import representation;
- export representation;
- dependency tracking;
- dependency graph construction;
- cycle detection;
- topological ordering;
- module resolution;
- module searching;
- visibility checking;
- module validation;
- diagnostics;
- caching;
- invalidation;
- statistics.

## Directory structure

```text
analysis/modules/
├── README.md
├── module.vit
├── module_attributes.vit
├── module_cache.vit
├── module_cycle.vit
├── module_database.vit
├── module_diagnostics.vit
├── module_edge.vit
├── module_exports.vit
├── module_graph.vit
├── module_imports.vit
├── module_info.vit
├── module_kind.vit
├── module_manager.vit
├── module_name.vit
├── module_namespace.vit
├── module_node.vit
├── module_order.vit
├── module_parser.vit
├── module_path.vit
├── module_registry.vit
├── module_resolver.vit
├── module_search.vit
├── module_statistics.vit
├── module_validation.vit
└── module_visibility.vit
```

## Components

### `module.vit`

Public facade of the module subsystem.

It combines the major module services behind a coherent API and should remain
relatively small.

It must not duplicate algorithms implemented by specialized files.

Typical responsibilities:

```text
create module system
register module
add dependency
resolve module
search module
check visibility
analyze graph
invalidate module
reset module system
```

---

### `module_name.vit`

Represents and validates module names.

Responsibilities include:

- storing normalized names;
- validating identifiers;
- comparing module names;
- hashing names;
- formatting names for diagnostics.

Example conceptual module name:

```text
parser
```

or:

```text
analysis
```

---

### `module_path.vit`

Represents module paths.

A path identifies a module inside the Vitte package/module hierarchy.

Examples:

```text
vitte/compiler/frontend/parser
vitte/compiler/analysis/modules/module
vitte/stdlib/core
```

Responsibilities include:

- parsing paths;
- normalization;
- joining path components;
- parent lookup;
- basename extraction;
- canonicalization;
- equality;
- hashing.

---

### `module_namespace.vit`

Represents namespaces associated with modules.

Namespaces provide the logical environment in which declarations are resolved.

A namespace can contain:

```text
types
procedures
constants
modules
imports
exports
symbols
```

---

### `module_kind.vit`

Defines the categories of modules understood by the compiler.

Depending on compiler configuration, module kinds may distinguish source,
generated, builtin, external or synthetic modules.

The exact set of kinds is defined by the Vitte implementation rather than by
this document.

---

### `module_info.vit`

Stores descriptive information associated with a module.

Typical metadata includes:

```text
module id
name
path
kind
source
namespace
state
attributes
```

---

### `module_attributes.vit`

Stores module-level attributes and configuration metadata.

Attributes are kept separate from core module identity so they can evolve
without changing the fundamental graph representation.

---

### `module_node.vit`

Represents a module as a node in the module dependency graph.

Conceptually:

```text
ModuleNode
    |
    +-- module id
    +-- dependencies
    +-- dependents
    +-- state
```

Graph-specific state belongs here rather than in the parser or resolver.

---

### `module_edge.vit`

Represents a dependency relationship between two modules.

Conceptually:

```text
A ----imports----> B
```

The edge can carry information required for diagnostics and graph analysis,
such as its source span or dependency properties.

---

### `module_graph.vit`

Builds and manipulates the complete dependency graph.

Example:

```text
main
 |
 +--> lexer
 |
 +--> parser
       |
       +--> ast
       |
       +--> token
```

The graph is used by:

- cycle detection;
- dependency traversal;
- invalidation;
- ordering;
- statistics.

---

### `module_cycle.vit`

Detects cyclic dependencies.

Example invalid graph:

```text
A
|
v
B
|
v
C
|
+----> A
```

The subsystem must be able to report the complete useful cycle rather than
only reporting that a cycle exists.

Example diagnostic:

```text
error: circular module dependency

A imports B
B imports C
C imports A
```

---

### `module_order.vit`

Computes a valid module processing or compilation order from the dependency
graph.

For:

```text
main -> parser -> ast
```

a dependency-first order is conceptually:

```text
ast
parser
main
```

The ordering stage should be deterministic.

Given the same graph, it should produce the same order.

---

### `module_imports.vit`

Represents imports declared by Vitte source code.

Examples of Vitte import syntax include:

```vitte
use vitte/compiler/frontend/parser
```

Grouped or wildcard imports are represented according to the active Vitte
grammar.

Import analysis records enough information for resolution and diagnostics,
including source locations where appropriate.

---

### `module_exports.vit`

Represents declarations exported by a module.

Exports determine which declarations can be made visible to importing modules.

Conceptually:

```text
module
 |
 +-- private declarations
 |
 +-- exported declarations
```

Export handling must remain distinct from import resolution.

---

### `module_registry.vit`

Provides indexed lookup of known modules.

Instead of scanning the complete database for every request, the registry can
maintain indexes such as:

```text
name -> module
path -> module
id   -> module
```

The registry must remain synchronized with the authoritative module database.

---

### `module_database.vit`

Central storage for module analysis data.

It is the authoritative container for module records and dependency
information.

Conceptually:

```text
ModuleDatabase
 |
 +-- modules
 +-- dependencies
 +-- generations
 +-- graph data
 +-- analysis state
```

Higher-level services should use the database rather than creating independent
copies of module state.

---

### `module_parser.vit`

Extracts module-related information from parsed source representation.

Its job is not to replace the main Vitte parser.

It converts relevant syntax/AST information into structures used by module
analysis.

Conceptually:

```text
AST
 |
 v
module_parser
 |
 +--> module declaration
 +--> imports
 +--> exports
 +--> attributes
```

---

### `module_resolver.vit`

Resolves references to modules.

Resolution may operate from:

```text
module name
module path
import declaration
current namespace
```

Conceptually:

```text
requested path
      |
      v
ModuleResolver
      |
      +--> registry
      |
      +--> database
      |
      v
resolved ModuleId
```

Resolution failures must be represented explicitly and should provide enough
information for diagnostics.

---

### `module_search.vit`

Provides higher-level module searching.

Unlike direct registry lookup, search may support broader queries and
candidate discovery.

Possible search dimensions include:

```text
name
path
namespace
module kind
package
```

Search and resolution are deliberately separate concepts:

```text
search     -> find candidates
resolution -> determine the referenced module
```

---

### `module_visibility.vit`

Checks whether a declaration exported by one module is accessible from
another module.

Conceptually:

```text
source module
     |
     v
visibility rule
     |
     v
target export
     |
     +--> accessible
     |
     +--> inaccessible
```

Visibility checking should operate on semantic module information rather than
raw source strings.

---

### `module_validation.vit`

Runs consistency checks over the module subsystem.

Validation can verify properties such as:

```text
valid module identity
valid paths
valid dependencies
resolved imports
valid exports
valid visibility
complete module order
absence of forbidden cycles
registry consistency
```

Validation should not silently repair invalid semantic state.

Failures should be surfaced through diagnostics.

---

### `module_diagnostics.vit`

Defines diagnostics specific to module analysis.

Typical errors include:

```text
module not found
duplicate module
invalid module path
circular dependency
unresolved import
private module access
duplicate export
invalid dependency
```

Diagnostics should preserve source spans whenever the source of the error is
known.

---

### `module_cache.vit`

Caches reusable module analysis results.

The cache exists to avoid repeating expensive work when neither the module nor
its relevant dependencies changed.

Conceptually:

```text
source changes
      |
      v
invalidate module
      |
      v
invalidate dependents
      |
      v
recompute affected analysis
```

Correctness takes priority over cache hits.

A stale cached result must never be considered authoritative.

---

### `module_statistics.vit`

Collects statistics about module analysis.

Useful metrics can include:

```text
number of modules
number of imports
number of exports
number of dependencies
number of graph edges
number of cycles
maximum dependency depth
resolved modules
unresolved modules
```

Statistics are observational and must not affect semantic results.

---

### `module_manager.vit`

Coordinates the complete module lifecycle.

The manager connects:

```text
database
registry
graph
resolver
cache
validation
diagnostics
```

Conceptually:

```text
                ModuleManager
                     |
       +-------------+-------------+
       |             |             |
       v             v             v
   Database       Registry       Cache
       |
       v
     Graph
       |
       +------------+
       |            |
       v            v
    Resolver     Ordering
       |            |
       +------+-----+
              |
              v
          Validation
```

The manager is responsible for orchestration, not for reimplementing each
algorithm.

## Module lifecycle

A typical module passes through the following pipeline:

```text
source file
    |
    v
parse source
    |
    v
extract module information
    |
    v
create module identity
    |
    v
register module
    |
    v
collect imports
    |
    v
collect exports
    |
    v
resolve dependencies
    |
    v
build dependency graph
    |
    v
detect cycles
    |
    v
compute order
    |
    v
check visibility
    |
    v
validate
    |
    v
semantic analysis
```

## Dependency graph

The dependency graph is directed.

If module `A` imports module `B`:

```text
A -> B
```

then `A` depends on `B`.

Example:

```text
application
    |
    +--> parser
    |      |
    |      +--> ast
    |      |
    |      +--> lexer
    |
    +--> diagnostics
```

A valid compilation order must ensure dependencies are available before their
dependents require them.

## Determinism

Module analysis must be deterministic.

For identical inputs:

```text
same source tree
same configuration
same target
same compiler version
```

the subsystem should produce:

```text
same module identities
same dependency graph
same resolution results
same ordering
same diagnostics
```

Do not rely on unstable hash-map iteration order to define compilation order.

## Error handling

Module analysis should prefer explicit result types over hidden failure.

Conceptually:

```text
success -> ModuleId
failure -> diagnostic / error result
```

Errors should contain enough context to identify:

```text
what failed
where it failed
which module was involved
which path was requested
why resolution failed
```

## Ownership

Module structures should follow the ownership rules of the Vitte compiler.

Large shared structures should generally be referenced rather than copied.

Mutation should be explicit.

Conceptually:

```vitte
ref ModuleDatabase
ref mut ModuleDatabase
```

Module identity values such as `ModuleId` should remain lightweight.

## Invalidation

Changing one module may invalidate modules depending on it.

For:

```text
A -> B -> C
```

if `C` changes, analysis may need to invalidate:

```text
C
B
A
```

The invalidation system should avoid rebuilding unrelated modules.

For example:

```text
A -> B

X -> Y
```

a change to `B` should not invalidate `X` or `Y`.

## Compiler integration

The module subsystem participates in the wider Vitte compiler pipeline:

```text
lexer
  |
  v
parser
  |
  v
AST
  |
  v
module analysis
  |
  v
name resolution
  |
  v
HIR
  |
  v
semantic analysis
  |
  v
type checking
  |
  v
borrow checking
  |
  v
MIR
  |
  v
IR
  |
  v
backend
```

The exact ordering can evolve with the compiler architecture, but module
resolution must provide stable semantic identities to downstream stages.

## Design rules

The following rules should be preserved.

### 1. One authoritative database

Module state should have a clear owner.

Avoid independent copies of module records across resolver, registry and graph
code.

### 2. IDs over pointers for identity

Semantic relationships should prefer stable IDs:

```text
ModuleId
DependencyId
```

rather than using memory addresses as identity.

### 3. Paths must be normalized

Equivalent paths must not accidentally produce distinct modules.

### 4. Resolution must be explicit

Do not silently choose arbitrary candidates when module resolution is
ambiguous.

### 5. Diagnostics retain spans

Whenever possible, import, export and dependency errors should point to their
originating source location.

### 6. Graph algorithms stay isolated

Cycle detection and ordering belong in their specialized modules rather than
being embedded in parser or resolver code.

### 7. Deterministic ordering

Graph traversal must not introduce nondeterministic compiler output.

### 8. Cache is never semantic authority

Cached information can accelerate analysis but the semantic model remains
authoritative.

### 9. Keep `module.vit` small

`module.vit` is the facade.

Algorithms belong in specialized implementation files.

### 10. Separate syntax and semantics

Parsing answers:

```text
What did the source say?
```

Module analysis answers:

```text
What module does it refer to?
Is it valid?
Is it visible?
What does it depend on?
```

These responsibilities should not be mixed.

## Example

Given:

```text
app/main.vit
compiler/parser.vit
compiler/lexer.vit
compiler/ast.vit
```

and dependencies:

```text
main   -> parser
parser -> lexer
parser -> ast
```

the graph becomes:

```text
        main
          |
          v
        parser
        /    \
       v      v
    lexer     ast
```

A valid dependency-first order is:

```text
lexer
ast
parser
main
```

The exact order between independent nodes such as `lexer` and `ast` should be
resolved using a deterministic policy.

## Testing

The subsystem should have tests covering at least:

- empty database;
- one module;
- multiple independent modules;
- linear dependency chains;
- diamond dependencies;
- duplicate module names;
- duplicate canonical paths;
- missing modules;
- unresolved imports;
- public exports;
- private exports;
- visibility failures;
- direct cycles;
- indirect cycles;
- self-dependencies;
- deterministic ordering;
- cache hits;
- cache invalidation;
- transitive invalidation;
- registry rebuilding;
- malformed module paths;
- search with no results;
- search with one result;
- ambiguous resolution.

Important graph shapes include:

### Linear

```text
A -> B -> C
```

### Diamond

```text
    A
   / \
  v   v
  B   C
   \ /
    v
    D
```

### Cycle

```text
A -> B
^    |
|    v
+----C
```

### Independent components

```text
A -> B

C -> D
```

## Performance

Module analysis is potentially executed for every compilation.

Important operations should therefore avoid unnecessary full-database scans.

Prefer indexed lookup where appropriate:

```text
O(1) / O(log n) lookup
```

over repeated:

```text
O(n) database scan
```

Graph algorithms should generally target:

```text
O(V + E)
```

where:

```text
V = modules
E = dependencies
```

Incremental compilation should limit recomputation to affected portions of the
graph whenever possible.

## Future extensions

The architecture should leave room for:

- incremental module loading;
- parallel module analysis;
- package-level module graphs;
- precompiled modules;
- generated modules;
- virtual modules;
- compiler builtin modules;
- module fingerprints;
- persistent caches;
- dependency graph serialization;
- module-level ABI metadata;
- cross-package visibility;
- IDE/LSP module queries.

These features should extend the existing model rather than bypassing the
module database and resolver.

## Source of truth

Implementation is authoritative.

When this document and the compiler disagree, update this document to match the
validated compiler implementation.

The Vitte grammar remains the authority for Vitte source syntax.

Relevant grammar:

```text
src/vitte/grammar/vitte.ebnf
```

Module implementation:

```text
src/vitte/compiler/analysis/modules/
```

Public facade:

```text
src/vitte/compiler/analysis/modules/module.vit
```
