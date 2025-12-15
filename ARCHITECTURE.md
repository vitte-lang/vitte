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
├── lexer/
│   ├── mod.rs (ou .c)             # Interface publique
│   ├── scanner.rs                 # State machine
│   ├── token_type.rs              # Token enumeration
│   ├── literal.rs                 # Number/string parsing
│   └── tests/                     # Unit tests
│
├── parser/
│   ├── mod.rs                     # Public interface
│   ├── expr.rs                    # Expression parsing (Pratt)
│   ├── stmt.rs                    # Statement parsing
│   ├── decl.rs                    # Declaration parsing
│   ├── recovery.rs                # Error recovery
│   └── tests/
│
├── ast/
│   ├── mod.rs                     # AST node definitions
│   ├── expr.rs                    # Expression nodes
│   ├── stmt.rs                    # Statement nodes
│   ├── types.rs                   # Type annotations
│   ├── visitor.rs                 # AST visitor pattern
│   └── printer.rs                 # AST pretty-printing
│
├── resolver/
│   ├── mod.rs                     # Symbol resolution phase
│   ├── scopes.rs                  # Scope tables & visibility
│   ├── symbol.rs                  # Symbol metadata
│   └── imports.rs                 # Module/import handling
│
├── typer/
│   ├── mod.rs                     # Type checker entry
│   ├── constraints.rs             # Constraint generation
│   ├── unify.rs                   # Unification algorithm
│   ├── inference.rs               # Type inference
│   ├── builtin.rs                 # Built-in types & ops
│   └── diagnostic.rs              # Type error messages
│
├── hir/
│   ├── mod.rs                     # High-level IR definition
│   ├── builder.rs                 # AST → HIR lowering
│   ├── visitor.rs                 # HIR visitor
│   └── printer.rs                 # HIR debug output
│
├── ir/
│   ├── mod.rs                     # Low-level IR definition
│   ├── builder.rs                 # HIR → IR lowering
│   ├── const_fold.rs              # Constant folding pass
│   ├── dce.rs                     # Dead code elimination
│   ├── inlining.rs                # Function inlining
│   └── visitor.rs                 # IR visitor
│
├── backend_c/
│   ├── mod.rs                     # C17 backend entry
│   ├── emitter.rs                 # IR → C code generation
│   ├── cgen_expr.rs               # Expression code gen
│   ├── cgen_stmt.rs               # Statement code gen
│   ├── names.rs                   # C name mangling/mapping
│   ├── runtime_calls.rs           # Runtime interface calls
│   └── header.rs                  # Generated #include's
│
└── driver.rs                       # Compilation pipeline orchestrator
```

### 🐍 **src/runtime/** — Runtime & ABI

```
src/runtime/
├── mod.rs                         # Runtime initialization
├── alloc.rs                       # Memory allocation interface
├── gc.rs                          # (Optionnel) GC support
├── panic.rs                       # Panic/unwinding
├── handles.rs                     # Handle management (for refs)
├── slice.rs                       # Slice representation
├── string.rs                      # String representation
├── atomics.rs                     # Atomic operations
└── pal_interface.rs               # Interface à PAL
```

### 🛠️ **src/pal/** — Platform Abstraction Layer

```
src/pal/
├── mod.rs                         # PAL public interface
├── fs.rs                          # File system ops
├── os.rs                          # OS-level primitives
├── mem.rs                         # Memory primitives (mmap, etc.)
├── thread.rs                      # Thread primitives
├── time.rs                        # Time/clock operations
│
├── posix/
│   ├── mod.rs
│   ├── fs.rs
│   ├── os.rs
│   └── ...
│
└── win32/
    ├── mod.rs
    ├── fs.rs
    ├── os.rs
    └── ...
```

### 📚 **include/** — Headers C/C++

```
include/
├── steel/
│   ├── config.h                   # Build config (VITTE_DEBUG, etc.)
│   ├── platform.h                 # Platform-specific defines
│   ├── version.h                  # Version info
│   │
│   ├── compiler/
│   │   ├── ast.h                  # AST node types (C)
│   │   ├── ir.h                   # IR node types (C)
│   │   └── ...
│   │
│   ├── runtime/
│   │   ├── alloc.h                # Memory allocation
│   │   ├── abi.h                  # ABI/runtime interface
│   │   └── ...
│   │
│   └── diag/
│       ├── codes.h                # Error codes enum
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

### 🧪 **tests/** — Suite de tests

```
tests/
├── unit/
│   ├── lexer/
│   ├── parser/
│   ├── typer/
│   ├── ir_gen/
│   └── backend_c/
│
├── integration/
│   ├── hello_world/
│   ├── fibonacci/
│   ├── string_ops/
│   ├── module_system/
│   └── error_cases/
│
├── fixtures/
│   ├── valid/
│   │   ├── simple_expr.vitte
│   │   ├── fn_call.vitte
│   │   └── ...
│   └── invalid/
│       ├── type_mismatch.vitte
│       ├── undefined_var.vitte
│       └── ...
│
└── golden/                        # Golden files (expected output)
    ├── lexer_tokens.txt
    ├── ast_dump.txt
    └── ir_dump.txt
```

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

