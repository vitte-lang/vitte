# Architecture de Vitte — Guide Approfondi

## Vue d'ensemble

Vitte est un langage de programmation moderne avec une toolchain complète "Steel" capable de générer du code C17 natif portable.

```
┌─────────────────────────────────────────────────────────────┐
│                      Source Code (.vitte)                   │
└──────────────────────┬──────────────────────────────────────┘
                       │
        ┌──────────────┴──────────────┐
        │                             │
    ┌───▼────┐               ┌───────▼────┐
    │ Lexer  │               │   Parser   │
    └───┬────┘               └───┬────────┘
        │  (Tokens)              │ (AST)
        └──────────┬─────────────┘
                   │
            ┌──────▼──────┐
            │  Resolver   │ (Symbol Table, Names)
            └──────┬──────┘
                   │
            ┌──────▼──────┐
            │ Type Checker│ (Semantic Analysis)
            └──────┬──────┘
                   │
            ┌──────▼──────┐
            │   HIR       │ (High-level IR)
            └──────┬──────┘
                   │
            ┌──────▼──────┐
            │  Passes     │ (Optimizations)
            └──────┬──────┘
                   │
            ┌──────▼──────┐
            │    IR       │ (Low-level IR)
            └──────┬──────┘
                   │
            ┌──────▼──────────────┐
            │  Backend (C17 emit) │
            └──────┬──────────────┘
                   │
        ┌──────────▼──────────────┐
        │  Generated C Code       │
        └──────────┬──────────────┘
                   │
        ┌──────────▼──────────────┐
        │  Native Binary (clang)  │
        └─────────────────────────┘
```

---

## Structure des répertoires

### 📄 **spec/** — Spécifications du langage

Normalisez les specs par domaine :

```
spec/
├── 00-intro.md                    # Vue d'ensemble (ce document)
├── 01-syntax-lexical.md           # Lexical & tokens
├── 02-grammar.md                  # Context-free grammar (.ebnf)
├── 03-ast.md                      # AST structure & node types
├── 04-semantics.md                # Semantic rules & scoping
├── 05-types.md                    # Type system details
├── 06-modules.md                  # Module system & visibility
├── 07-stdlib.md                   # Standard library
├── 08-abi-ffi.md                  # ABI & FFI C interop
├── 09-error-model.md              # Error handling strategy
├── 10-optimization.md             # IR passes & optimizations
├── 11-backend-c.md                # C code generation
├── 12-runtime-pal.md              # Runtime & PAL interface
│
├── grammar/
│   ├── vitte.ebnf                 # EBNF formelle
│   ├── vitte.pest                 # Parser grammar (Pest)
│   └── keywords.txt               # Reserved words
│
├── type-system/
│   ├── inference.md               # Type inference algorithm
│   ├── constraints.md             # Constraint solving
│   └── builtin-types.md           # Primitifs & composés
│
└── runtime/
    ├── abi.md                     # Application Binary Interface
    ├── memory.md                  # Memory model
    ├── handles.md                 # Handle/pointer semantics
    └── panic.md                   # Panic/unwinding model
```

### 🔨 **src/compiler/** — Compilateur multi-passe

```
src/compiler/
├── ast.c                          # AST nodes + helpers
├── lexer.c                        # Tokenization
├── parser.c                       # Parsing
├── resolver.c                     # Symbol resolution
├── types.c                        # Type definitions
├── typecheck.c                    # Type checking
├── hir_build.c                    # AST → HIR
├── ir_build.c                     # HIR → IR
├── passes.c                       # IR passes
├── symtab.c                       # Symbol tables
├── interner.c                     # String interning
│
├── backend_c/
│   ├── c_emit.c                   # IR → C emission
│   ├── c_name_mangle.c            # Name mangling
│   └── c_runtime_shim.c           # Runtime shims
│
└── driver/
    ├── compile_unit.c             # Per-unit compile
    ├── pipeline.c                 # Driver pipeline
    └── link_step.c                # Link step
```

### 🐍 **src/runtime/** — Runtime & ABI

```
src/runtime/
├── rt_alloc.c                     # Allocation helpers
├── rt_handles.c                   # Handle management
├── rt_panic.c                     # Panic/diagnostic glue
├── rt_slice.c                     # Slice helpers
└── rt_string.c                    # String helpers
```

### 🛠️ **src/pal/** — Platform Abstraction Layer

```
src/pal/
├── posix/
│   ├── pal_posix.c
│   ├── pal_posix_fs.c
│   ├── pal_posix_net.c
│   ├── pal_posix_proc.c
│   ├── pal_posix_thread.c
│   ├── pal_posix_time.c
│   └── pal_posix_dynload.c
│
└── win32/
    ├── pal_win32.c
    ├── pal_win32_fs.c
    ├── pal_win32_net.c
    ├── pal_win32_proc.c
    ├── pal_win32_thread.c
    ├── pal_win32_time.c
    └── pal_win32_dynload.c
```

### 📚 **include/** — Headers C/C++

```
include/
└── vitte/
    ├── vitte.h                    # Public umbrella header
    ├── runtime.h                  # Runtime interface
    ├── lexer.h                    # Lexer API
    ├── parser_phrase.h            # Parser API
    ├── desugar_phrase.h           # Desugaring API
    ├── codegen.h                  # Codegen API
    ├── diag.h                     # Diagnostics
    ├── cpu.h                      # CPU feature detection
    └── asm_verify.h               # ASM verification
│       ├── span.h                 # Source location
│       └── messages.h             # Error message templates
```

### 🎓 **std/** — Stdlib Vitte

```
std/
├── muffin.muf                     # Package definition
├── src/
│   ├── builtin.vitte              # Magic built-ins (types, operators)
│   ├── prelude.vitte              # Automatically imported
│   ├── core/
│   │   ├── option.vitte           # Option<T>
│   │   ├── result.vitte           # Result<T, E>
│   │   ├── slice.vitte            # Slice operations
│   │   └── string.vitte           # String type
│   ├── memory/
│   │   ├── alloc.vitte            # Allocators
│   │   ├── box.vitte              # Box<T>
│   │   └── unique.vitte           # Unique handles
│   ├── io/
│   │   ├── file.vitte             # File I/O
│   │   ├── stdio.vitte            # stdin/stdout/stderr
│   │   └── format.vitte           # Formatting
│   └── sys/
│       ├── fs.vitte               # Filesystem
│       ├── process.vitte          # Process spawning
│       └── env.vitte              # Environment
```

### 🧪 Tests

- Compiler/tests: `compiler/` via CMake/ctest (`make test`).
- Stdlib tests: `std/scripts/test_std.sh` (wrapper `muffin test`).
- `tests/` au niveau racine est un placeholder (peu ou pas de cas aujourd'hui).

### 📖 **docs/** — Documentation utilisateur

```
docs/
├── index.md                       # Landing page
├── getting-started.md             # Installation & first program
├── tutorial/
│   ├── 01-basics.md
│   ├── 02-functions.md
│   ├── 03-types.md
│   ├── 04-modules.md
│   └── 05-advanced.md
├── reference/
│   ├── syntax.md
│   ├── builtins.md
│   ├── stdlib.md
│   └── abi.md
└── examples/
    ├── fib.vitte
    ├── http_server.vitte
    └── ...
```

---

## Phases de compilation

### 1. **Lexical Analysis** (`src/compiler/lexer/`)
- Entrée : source `.vitte`
- Sortie : stream de tokens
- Outils : DFA, longest match

### 2. **Parsing** (`src/compiler/parser/`)
- Entrée : tokens
- Sortie : AST
- Stratégie : Pratt parsing (expressions), recursive descent (statements)

### 3. **Resolution** (`src/compiler/resolver/`)
- Entrée : AST
- Tâche : identifier → symbol, resolve imports, check visibility
- Sortie : Annotated AST

### 4. **Type Checking** (`src/compiler/typer/`)
- Entrée : Resolved AST
- Algo : Hindley-Milner + constraints
- Sortie : Type-annotated AST + type environment

### 5. **Lowering to HIR** (`src/compiler/hir/`)
- Entrée : Typed AST
- Tâche : desugaring, normalization
- Sortie : HIR (plus régulier, moins d'expressions)

### 6. **Optimization Passes** (`src/compiler/ir/`)
- Entrée : HIR
- Passes :
  - Constant folding
  - Dead code elimination
  - Function inlining
  - Tail call optimization (si applicable)
- Sortie : Optimized IR

### 7. **Code Generation** (`src/compiler/backend_c/`)
- Entrée : IR
- Processus :
  - Map IR nodes → C statements/expressions
  - Allocate C variables for values
  - Call runtime functions
- Sortie : C17 code

### 8. **Compilation C** (external, clang)
- Entrée : `.c` files
- Sortie : native binary

---

## Système de type proposé

```
Types primitifs:
  i8, i16, i32, i64, u8, u16, u32, u64
  f32, f64
  bool, char
  void (unit type)

Types composés:
  T!            (non-nullable pointer, owns T)
  &T            (borrowed reference)
  &mut T        (mutable borrow)
  [T]           (slice of T)
  [T; N]        (array T[N])
  (T1, T2, ...) (tuple)
  {x: T1, y: T2} (struct)

Type variables:
  Generic<T>    (parametric polymorphism)

Fonctions:
  fn(arg: T1) -> T2 (function type)
```

---

## Package Management (Muffin)

```toml
# muffin.muf (Vitte package manifest)

[package]
name = "mylib"
version = "0.1.0"
edition = "2025"

[dependencies]
std = "0.1"
myutil = { path = "../myutil" }

[profile.debug]
opt-level = 0
debug = true

[profile.release]
opt-level = 3
lto = true
```

---

## Build Pipeline

1. **parse muffin.muf** → workspace config
2. **resolve dependencies** → lock file (muffin.lock)
3. **compile each unit** with multi-pass compiler
4. **link** (if needed) with generated C code
5. **invoke C compiler** (clang/gcc/msvc)
6. **produce binary**

---

## Bonnes pratiques

✅ **Chaque phase est indépendante** → testable séparément  
✅ **AST/IR visitors** pour parcourir facilement  
✅ **Comprehensive error reporting** (span + message)  
✅ **Golden test files** pour output cmpare  
✅ **Well-documented passes** avec algorithmes  
✅ **Runtime unifies all platforms** via PAL  
