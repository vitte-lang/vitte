# Vitte


**Vitte** is an experimental systems language and toolchain.  
It is designed to make low‑level code **readable**, **explicit**, and **predictable**.

Vitte is not trying to be clever. It prefers clarity over implicit magic, and it favors simple, structured syntax that tools can parse and analyze reliably. The goal is to make it easier to build serious system software without sacrificing the ability to reason about what the code does.

---

## What Vitte Is (and Isn’t)

This section helps you understand the philosophy of the project before you start.

**Vitte aims to be:**
- A language that is easy to read and reason about. That means fewer hidden rules and fewer surprises.
- A structured syntax designed for robust tooling. Editors and linters work better when the grammar is consistent.
- A clean compiler pipeline (frontend → IR → backend). You can inspect or improve each stage without guessing where bugs are.
- A practical base for kernel, embedded, and toolchain projects. This is the world Vitte targets.

**Vitte is not:**
- A stable language. The syntax can still change as the compiler evolves.
- A clone of Rust/C/C++. The goal is not to copy, but to explore a different design.
- A web‑framework‑first language. This is a systems language, not a frontend stack.

---

## Why Vitte

Many systems languages are powerful but difficult to parse mentally. They often include implicit behavior, complex rules, or decades of legacy. Vitte tries to keep the surface syntax small and explicit so you can read code quickly and understand it without context.

That makes Vitte a good fit for:
- hobby OS development,
- bare‑metal runtimes,
- system tooling,
- embedded platforms (Arduino/STM32/ESP32),
- and experimental toolchains.

If your priority is strong safety guarantees, Vitte may not be the right choice. If your priority is clarity and control, Vitte might be a good fit.

---

## Getting Started (Beginner)
Documentation:

https://vitte.netlify.app/
This is the minimal path to compile and run something.
```sh
brew install vitte
```
### 1. Build the project

From the repository root:

```sh
make build
```

This builds the compiler. The binary will be located here:

```
bin/vitte
```

### 2. Run an example

```sh
vitte build examples/syntax_features.vit
```

This takes a `.vit` file and compiles it. If the compiler reports an error, it will show a location in the source file.

### 3. Check syntax (no build)

```sh
vitte check examples/syntax_features.vit
```

This is useful while editing because it validates syntax without producing a build.

---

## Reproducible Objects (macOS, clang)

Vitte can emit deterministic `.o` files for strict byte-for-byte comparison with C/C++ outputs. This mode is designed for macOS and clang, and focuses on object files rather than final binaries.

Build the compiler, then run:

```sh
make repro
```

Or manually:

```sh
vitte build --repro --emit-obj -o build/repro/vitte.o tests/repro/min.vit
```

Notes:
- `--repro` forces flags that reduce non-determinism (debug off, fixed prefix maps, and linker UUID disabled where applicable).
- `--emit-obj` produces an object file and skips linking.
 - `--repro` also enables strict IR lowering order to reduce codegen drift.
 - `--repro-strict` exists for explicit control, but `--repro` already implies it.
- See `tools/repro_compare.sh` for the comparison logic and current fixtures under `tests/repro`.
- To regenerate C++ fixtures from the current lowering, run `make repro-generate`.
- To enable binary comparison, run `COMPARE_BIN=1 make repro`.
- To generate binary mismatch diagnostics (otool/nm), run `COMPARE_BIN=1 DIAG_BIN=1 make repro`.
- To treat code signatures as ignorable for binary compare, run `COMPARE_BIN=1 STRIP_CODESIG=1 make repro`.
- To compare against hand-written C++ fixtures (best-effort), run `COMPARE_MANUAL_CPP=1 make repro`.

---

## Detailed Installation

Use this section if you want a clean, repeatable setup.

### macOS

Requirements: `clang`, `openssl`, `curl`.

```sh
brew install llvm openssl@3 curl
```

Build:

```sh
OPENSSL_DIR=/opt/homebrew/opt/openssl@3 make build
```

If you get OpenSSL errors, make sure `OPENSSL_DIR` points to your Homebrew install.

### Linux (Ubuntu/Debian)

```sh
sudo apt-get update
sudo apt-get install -y clang libssl-dev libcurl4-openssl-dev
make build
```

### Windows

Support is experimental. The recommended path is WSL2 (Ubuntu). Install dependencies inside WSL and follow the Linux steps.

---

## Vitte Syntax (Quick Tour)

The examples below are simple and meant for beginners. Each snippet shows one core idea.

### Procedure

A procedure is a function. It can take parameters and return a value.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

- `proc` defines a function.
- `give` returns a value (like `return`).

### Entry point

An `entry` block defines the program start. It is similar to `main` in C.

```vit
entry main at core/app {
  let x: int = 42
  give x
}
```

### Form (struct)

A `form` is like a struct. It groups fields together.

```vit
form Point {
  x: int
  y: int
}
```

### Pick (enum)

A `pick` is like an enum with variants. It is useful for result types.

```vit
pick Result {
  case Ok(value: int)
  case Err(code: int)
}
```

### Match

`match` is pattern matching. It selects a branch based on a value.

```vit
match res {
  case Ok(v) { give v }
  case Err(e) { give e }
  otherwise { give 0 }
}
```

### Unsafe + asm

Low‑level code is explicit. You must enter `unsafe` before doing raw operations.

```vit
unsafe {
  asm("hlt")
}
```

---

## Hello Vitte (Step‑by‑Step)

This is a minimal tutorial you can follow without prior knowledge.

### 1. Create a file

Create a file named `hello.vit` in the repo root:

```vit
entry main at core/app {
  let msg: string = "Hello, Vitte!"
  give msg
}
```

### 2. Check the file (syntax only)

```sh
./bin/vitte check hello.vit
```

If there is an error, the compiler will show the file, line, and column.

### 3. Build the file

```sh
./bin/vitte build hello.vit
```

This produces output using the default backend. If something fails, re‑run with `check` to get clearer errors.

### 4. Next steps

Try modifying the file:

- Add a `proc` and call it.
- Add a `form` and pass it around.
- Add a `pick` and match on it.

This is the fastest way to learn the syntax by doing.

---

## More Language Basics (Beginner‑Friendly)

This section adds practical examples of common things you will write in Vitte.

### Variables (let / make)

Use `let` for immutable values and `make` for mutable values.

```vit
let x: int = 10
make counter as int = 0
```

### Updating a variable (set)

`set` updates a mutable value.

```vit
make counter as int = 0
set counter = counter + 1
```

### If / else

```vit
if x > 10 {
  give 1
} otherwise {
  give 0
}
```

### Loop

```vit
make i as int = 0
loop {
  if i >= 3 { break }
  set i = i + 1
}
```

### For‑in

```vit
for item in list {
  // use item
}
```

### Match / case

```vit
match value {
  case Ok(v) { give v }
  case Err(e) { give e }
  otherwise { give 0 }
}
```

### Select / when

```vit
select value
  when Ok(v) { give v }
  when Err(e) { give e }
  otherwise { give 0 }
```

### Errors and Diagnostics

When Vitte fails, it reports file, line, and column, plus a small code snippet.
This makes it easier to see the exact place where the error happened.

---

## Types (int / string / bool)

Vitte keeps its core types simple. These are the most common ones you will see:

- `int` for integers
- `string` for text
- `bool` for true/false

Example:

```vit
let age: int = 30
let name: string = "Ada"
let ok: bool = true
```

You can use these types in forms, procedure parameters, and return values.

---

## Procedures and Parameters

Procedures are functions. They can take parameters and return a value.

```vit
proc greet(name: string) -> string {
  give "Hello, " + name
}
```

If a procedure does not return a value, you can omit the return type.

```vit
proc log(msg: string) {
  // do something
}
```

---

## Modules and Imports (use / pull / share)

Vitte has explicit module syntax, designed to keep code organized.

- `use` imports a module or specific names
- `pull` brings a module into scope (typically by path)
- `share` re‑exports names for other modules

Examples:

```vit
use std/core/types.{int, string}
use std/kernel.{console, time}
```

```vit
pull my/project/module as mod
```

```vit
share all
```

This makes dependencies explicit and keeps large projects readable.

---

## form vs pick (Struct vs Enum)

Vitte uses `form` to define structured data (similar to a struct) and `pick` to define variants (similar to an enum).

### form

Use `form` when you always have the same fields:

```vit
form User {
  id: int
  name: string
}
```

### pick

Use `pick` when you have multiple possible shapes:

```vit
pick Result {
  case Ok(value: int)
  case Err(code: int)
}
```

---

## Unsafe + asm (Practical example)

`unsafe` is required for low‑level operations that the compiler cannot guarantee are safe. `asm` allows embedding raw assembly.

Example:

```vit
#[extern("C")]
proc outb(port: u16, val: u8)

entry main at core/app {
  unsafe {
    asm("hlt")
  }
  return
}
```

---

## Style Conventions (Naming and Organization)

These conventions keep code consistent and easy to read:

- Use `snake_case` for variable and function names
- Use `PascalCase` for `form` and `pick` types
- Keep one module per file when possible
- Prefer explicit `use` imports over global assumptions

These are not enforced yet, but following them makes collaboration easier.

---

## Error Handling (Result / Option)

Vitte often uses `Result` for operations that can fail, and `Option` for values that may or may not exist.

Example with `Option`:

```vit
proc maybe_find(id: int) -> Option[int] {
  if id == 0 {
    give Option.None
  }
  give Option.Some(id)
}
```

Example with `Result`:

```vit
proc parse_int(s: string) -> Result[int, string] {
  if s.len == 0 {
    give Result.Err("empty input")
  }
  give Result.Ok(1)
}
```

---

## Test Conventions

Tests live in the `tests/` directory. When you add or modify syntax, you should add a minimal test case that exercises the change.

- Prefer small, focused tests
- Use descriptive filenames
- Keep inputs short so failures are easy to debug

Example:

```
tests/my_feature.vit
```

---

## Complete Mini‑Module Example

This small module shows imports, a form, a pick, and a simple procedure.

```vit
space my/demo

use std/core/types.{int, string}

form User {
  id: int
  name: string
}

pick FindResult {
  case Found(user: User)
  case NotFound
}

proc find_user(id: int) -> FindResult {
  if id == 1 {
    let u: User = User(id = 1, name = "Ada")
    give FindResult.Found(u)
  }
  give FindResult.NotFound
}
```

This is the general style you can follow for small modules.

---

## Full Multi‑File Project Example

This example shows how a small project can be split into modules. You can copy this into a folder and build it.

**File structure:**

```
my_project/
  main.vit
  math.vit
```

**math.vit**

```vit
space my_project

proc add(a: int, b: int) -> int {
  give a + b
}
```

**main.vit**

```vit
space my_project

use my_project.math

entry main at core/app {
  let v: int = math.add(2, 3)
  give v
}
```

This example keeps the module path consistent (`space my_project`) and imports it from the main file.

---

## Debugging Errors (Beginner Guide)

When Vitte fails, it shows the exact file, line, and column. You should always read the line above and below the highlighted snippet.

Steps you can follow:

1. **Read the error message** and the shown snippet.
2. **Check syntax** for missing braces, commas, or parentheses.
3. **Re‑run with `check`** to get faster feedback:
   `./bin/vitte check path/to/file.vit`
4. **Reduce the file** by commenting out sections to isolate the error.

Most beginner errors are missing braces, wrong keywords, or incorrect module paths.

---

## Quick Contribution Checklist

Use this list before you open a PR:

- [ ] Code builds locally (`make build`)
- [ ] Tests run if relevant (`make test`, `make parse`, `make hir-validate`)
- [ ] New syntax or behavior has a minimal test
- [ ] Documentation updated if behavior changed

This keeps reviews fast and helps others understand your changes.

---

## Project Structure

The repository is split into clear areas. This helps beginners navigate:

- `src/` : the compiler implementation.
- `src/vitte/std/` : the standard library.
- `examples/` : example `.vit` programs.
- `tests/` : language and regression tests.
- `tools/` : scripts and utilities.
- `target/` : runtimes and target‑specific toolchains.

---

## Useful Commands

These are the most common commands you will use while experimenting:

```sh
vitte parse  path/to/file.vit
vitte check  path/to/file.vit
vitte build  path/to/file.vit
```

- `parse` only checks parsing.
- `check` validates more rules without building.
- `build` produces compiled output.

---

## Vitte and Low‑Level Development

Vitte is designed for code that is close to the metal, but still readable:

```vit
#[extern("C")]
proc outb(port: u16, val: u8)

entry main at core/app {
  unsafe {
    asm("hlt")
  }
  return
}
```

This style is intentional: explicit, minimal, and clear.

---

## Design Principles

Vitte is guided by a small set of principles. These ideas shape both syntax and tooling.

- **Explicitness over inference**: if something matters, it should be spelled out.
- **Readable structure**: blocks, forms, and control flow are visually clear.
- **Tooling‑friendly syntax**: predictable grammar makes IDEs and tooling reliable.
- **Deterministic builds**: the toolchain aims for stable, reproducible outputs.
- **Low‑level control**: you should be able to reach the metal when needed.

---

## Compiler Pipeline Explained

The compiler is intentionally simple and layered. This makes it easier to debug and extend.

1. **Frontend**: parses the source into an AST and checks syntax and structure.
2. **HIR (High‑level IR)**: lowers surface syntax into a more uniform representation.
3. **MIR (Mid‑level IR)**: normalizes control flow, patterns, and low‑level operations.
4. **Backend**: emits C++ for now, then relies on the native toolchain.

If you understand this pipeline, you can reason about where changes belong.

---

## Language Comparison (High‑Level)

This is a simple comparison to help newcomers:

- **C**: maximum control, minimal safety. Vitte keeps control but adds structure.
- **C++**: very powerful, but complex. Vitte aims for clarity and smaller rules.
- **Rust**: strong safety guarantees. Vitte focuses on readability and explicitness.

If you want absolute safety, Vitte is not your choice. If you want explicit, readable systems code, it might be.

---

## FAQ

**Is Vitte stable?**  
No. Vitte is experimental and syntax may change.

**Why a new syntax?**  
The goal is to make system‑level code more readable and tooling‑friendly.

**Can I build an OS with it?**  
That’s a primary goal; VitteOS development is ongoing.

**Is C compatibility guaranteed?**  
Not directly. The approach is explicit runtime/bindings rather than drop‑in C.

**Where do I get help?**  
See `SUPPORT.md` or open a GitHub issue with a minimal example.

---

## CLI and Docs

These files provide more detail:

- `docs/cli.md` : commands and options
- `docs/errors.md` : diagnostic codes
- `docs/stdlib.md` : standard library modules
- `CONTRIBUTING.md` : contribution guidelines
- `SECURITY.md` : security policy
- `SUPPORT.md` : support and help
- `ROADMAP.md` : project roadmap
- `CHANGELOG.md` : notable changes

---

## Status

Vitte is **experimental**.  
Stability is not guaranteed and breaking changes are expected.

---

## License

See `LICENSE`.
