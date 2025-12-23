# Vitte — Toolchain, Bench & Fuzz Toolkit (C17)

<p align="center">
  <img src="assets/logo-vitte.svg" alt="Vitte Logo" width="220">
</p>

<p align="center">
  <i>Tooling systèmes portable en C17 : build, toolchain, benchmarks, fuzzing — orienté CI et reproductibilité.</i>
</p>

<p align="center">
  <a href="https://github.com/vitte-lang/vitte/releases">
    <img alt="Release" src="https://img.shields.io/github/v/release/vitte-lang/vitte?style=flat-square">
  </a>
  <img alt="License" src="https://img.shields.io/github/license/vitte-lang/vitte?style=flat-square">
  <img alt="Top language" src="https://img.shields.io/github/languages/top/vitte-lang/vitte?style=flat-square">
</p>

## TL;DR (10s)

Vitte is an experimental language + C17 codebase that includes a working compiler CLI (`vittec`) and supporting tooling (bench/toolchain/fuzz) aimed at reproducible, CI-friendly systems development.

- For: contributors hacking on a small C compiler/codegen pipeline, and folks who want a portable C17 toolkit around it.
- Build (compiler): `cd compiler && make debug`
- Run tests (compiler): `cd compiler && make test`
- Compile an example (emit C): `cd compiler && ./build/debug/vittec --emit-c examples/hello.vitte -o build/hello.c`

## Quick start (compiler)

These commands mirror `compiler/START_HERE.txt`:

```sh
cd compiler
make debug
make test
make examples
make help
```

## Prereqs

- Compiler toolchain: a C compiler with C17 support (Clang, GCC, or MSVC).
- Build tools: CMake (3.16+) and a build backend (Make or Ninja).
- OS notes:
  - Linux/macOS: `make debug`/`make test` work out of the box if CMake + a compiler are installed.
  - Windows: easiest is CMake + Ninja + Clang; `make examples` runs `./test_examples.sh` (needs a POSIX shell such as Git Bash/MSYS2/WSL).
- Optional deps:
  - `clang-format`/`clang-tidy` (repo has configs) if you want formatting/linting locally.

## Repo layout

- [`compiler/`](compiler/) — the Vitte compiler (`vittec`) + its own docs and Make/CMake glue.
- [`include/`](include/) — public C headers for the core libraries/tooling.
- [`src/`](src/) — core language libraries (parser/desugar/codegen building blocks used by the compiler).
- [`tests/`](tests/) — unit tests for the core libraries.
- [`examples/`](examples/) — example programs and small samples.
- [`docs/`](docs/) — additional documentation.
- [`spec/`](spec/) — language/runtime specs and contracts.
- [`toolchain/`](toolchain/) — reproducible toolchain/config infrastructure (targets/flags/RSP/env).

## Common workflows

```sh
# Debug build
cd compiler && make debug

# Release build
cd compiler && make release

# Run a single test binary
cd compiler/build/debug && ./test_lexer

# Run ctest (all tests, or filtered)
cd compiler/build/debug && ctest --output-on-failure
cd compiler/build/debug && ctest -R test_lexer --output-on-failure

# Clean build (wipe + rebuild)
cd compiler && make clean && make debug
```

## Using the CLI

Assuming you already ran `cd compiler && make debug`:

```sh
# 1) Tokens: prints one token per line (kind, span, lexeme)
./build/debug/vittec --tokens examples/hello.vitte
# Example output (shape):
# TK_KW_FN 0..2 `fn`
# TK_IDENT 3..8 `hello`

# 2) Emit C: writes a C file (no stdout on success)
./build/debug/vittec --emit-c examples/hello.vitte -o build/hello.c
# Expected: build/hello.c created

# 3) Compile emitted C with your system compiler
clang -std=c17 build/hello.c -o build/hello
# Expected: build/hello produced (or build/hello.exe on Windows)
```

## Project status

> **Implemented:** lexer + token emission, C emission, unit tests, example programs, CMake/Make build glue.
>
> **Next:** parser expansion, semantic analysis/type checking, richer lowering/IR, backends beyond “emit C”.
>
> **Stability:** early-stage; CLI/output formats and internal APIs may change, but tests should remain the source of truth.

## Troubleshooting

- Missing compiler (`clang`/`gcc`/`cl`): install a C toolchain and ensure it’s on `PATH`.
- CMake too old: upgrade to CMake 3.16+ (and make sure `cmake --version` matches what you think you installed).
- Windows generator mismatch (MSVC vs Clang/Ninja): prefer `cmake -G Ninja -DCMAKE_C_COMPILER=clang ...` for predictable builds.
- `ctest` not found: it ships with CMake; ensure the CMake bin dir is on `PATH` and try `cmake --version` to confirm.
- PATH/shell issues running scripts: `make examples` uses `./test_examples.sh`; run in Git Bash/MSYS2/WSL, or execute the equivalent steps manually.

---

## Sommaire

1. [Présentation](#présentation)
2. [Ce que contient le dépôt](#ce-que-contient-le-dépôt)
3. [Architecture](#architecture)
4. [Compatibilité & cibles](#compatibilité--cibles)
5. [Build](#build)
6. [Bench](#bench)
7. [Toolchain](#toolchain)
8. [Fuzz](#fuzz)
9. [Contribuer](#contribuer)
10. [Licence](#licence)

---

## Présentation

**Vitte** (ce dépôt) est une base **C17** pour outiller un écosystème systèmes :

- orchestration **toolchain** (clang/lld, SDK, flags, fichiers réponse `.rsp`, env),
- harness **benchmark** (registry + runner, warmup/repeat, exports JSON/CSV),
- pipelines **fuzz** (configs CI + corpora),
- modules utilitaires (string/timing/log/options) pour garder le cœur sans dépendances.

Objectifs d’ingénierie :

- **Portable** : macOS / Linux / Windows.
- **Prévisible** : sorties stables, erreurs explicites, configs versionnées.
- **CI-friendly** : execution non-interactive, artefacts machine-friendly (JSON/CSV), modes fast/fail-fast.
- **Intégrable** : bibliothèques C17 simples à embarquer dans un mono-repo.



Ici, “plus puissant” veut dire “plus expressif” : le **langage Vitte** vise à offrir des primitives haut niveau,
sans abandonner les contraintes systèmes (perf, portabilité, ABI/FFI).

- **Types & abstractions** : `struct`, `enum`, `trait`/`interface`, `impl`, `type`, annotations de types, visibilité (`pub`/`private`…).
- **Contrôle de flux riche** : `match` (pattern matching), exceptions (`try`/`catch`/`throw`), `async`/`await`, `yield`.
- **Modularité** : `module`, `import`/`export` pour structurer des projets au-delà d’un “header+source”.
- **Frontière unsafe explicite** : blocs `unsafe { ... }` pour isoler les opérations à risque.
- **Interop** : appel de C depuis Vitte et export Vitte → C (le compilateur peut aussi **émettre du C**).

Références : `docs/language-spec/reference.md`, `sdk/docs/FFI.md`.

---

## Ce que contient le dépôt

Le contenu exact peut évoluer, mais les briques visées sont :

- `bench/` : harness benchmark C17.
- `toolchain/` : intégration clang/lld, configuration par OS/target, templates `.rsp`.
- `fuzz/` : configs CI (sanitizers/coverage) + corpora (seeds) par composant.
- `spec/` : specs (targets, flags, conventions, éventuellement grammaire/ABI).
- `tools/` : scripts d’assistance (CI, packaging, diagnostics).
- `std/` / `modules/` : modules utilitaires partagés (si présents dans ton état de dépôt).

Arborescence indicative :

```text
vitte/
  bench/
    src/bench/
      registry.[ch]
      runner.[ch]
      output.[ch]
      options.[ch]
      platform.h
      timing.[ch]
      strutil.[ch]
      types.h
      sample.[ch]
  toolchain/
  fuzz/
  spec/
  tools/
  README.md
```

---

## Architecture

### Build / Toolchain

```text
Sources / Inputs
   │
   ▼
toolchain/ (détection SDK, flags, .rsp, env)
   │
   ├─ compile → .o
   ├─ archive → .a
   └─ link    → exe/dylib/so
   ▼
Artefacts + logs reproductibles
```

### Bench

```text
bench_registry_add(...)  → registry
          │
          ▼
bench_runner_run(...)    → warmup / repeat / min-time
          │
          ├─ console
          ├─ JSON
          └─ CSV
```

### Fuzz

```text
corpora/ + harness → fuzz runners → crashes/repros → triage
```

---

## Compatibilité & cibles

Cibles de dev quotidiennes :

- Linux : x86_64 / arm64
- macOS : Intel / Apple Silicon
- Windows : x64 / arm64

Les détails (triples, flags, variantes clang/msvc, etc.) sont pilotés via `toolchain/` et la CI (`fuzz/` / workflows).

---

## Build

Le dépôt est conçu pour être compilé via tes scripts/outils de build (Make/CMake/Ninja/scripts CI).
Sans présumer du build-system exact, la baseline recommandée pour compiler les modules C17 :

- Clang/GCC : `-std=c17 -O2 -Wall -Wextra -Wpedantic`
- MSVC : `/std:c17 /O2 /W4`

Point d’attention :

- `bench/src/bench/platform.h` inclut une implémentation conditionnelle via `BENCH_PLATFORM_IMPLEMENTATION`.
  Définir cette macro dans **exactement une** translation unit, ou définir `BENCH_PLATFORM_NO_IMPL` partout et fournir l’impl ailleurs.

### Toolchain locale (Windows)

Option recommandée (preset) : voir `docs/windows.md` et lancer, depuis un **Visual Studio Developer Prompt** :

```powershell
cmake --preset win-clang-cl-ninja
cmake --build --preset win-clang-cl-ninja
ctest --preset win-clang-cl-ninja
```

Pour construire `src/vitte/*.c` et exécuter les tests unitaires depuis Windows, installe les outils et configure CMake ainsi :

1. Installer Clang et Ninja via `winget` :

   ```powershell
   winget install -e --id LLVM.LLVM --accept-package-agreements --accept-source-agreements
   winget install -e --id Ninja-build.Ninja --accept-package-agreements --accept-source-agreements
   ```

2. Configurer CMake avec les binaire installés (adapter les chemins si besoin) :

   ```powershell
   $clang = "C:/Program Files/LLVM/bin/clang.exe"
   $ninja = "$env:LOCALAPPDATA/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe/ninja.exe"
   cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER="$clang" -DCMAKE_MAKE_PROGRAM="$ninja"
   ```

3. Construire et lancer les tests `vitte_t_lexer` et `vitte_t_desugar_phrase` :

   ```powershell
   cmake --build build --target vitte_tests
   ctest --test-dir build --output-on-failure
   ```

   Une cible Make équivalente est disponible : `make vitte-tests`.

> ℹ️ Alternative GCC/MinGW : si tu préfères utiliser `gcc`, décompresse un bundle WinLibs (ex. `C:\Tools\mingw64`) puis configure CMake avec `-DCMAKE_C_COMPILER="C:/Tools/mingw64/bin/gcc.exe"` (et éventuellement `-DCMAKE_RC_COMPILER="C:/Tools/mingw64/bin/windres.exe"`). Les mêmes commandes `cmake --build ... --target vitte_tests` fonctionnent ensuite.

### Windows Rust linking notes

- Both the compiler (`vittec_compiler`) and benchmark (`benchc`) targets link `ntdll` when `WIN32` is set so that the MinGW-style Rust static library can resolve the CRT exports it needs.
- When building with MinGW we rely on Cargo running on the `stable-x86_64-pc-windows-gnu` toolchain so `libvitte_rust_api.a` matches the GNU ABI that the C build links against (`toolchain/config/targets/windows_x86_64_gnu.toml` describes the target preset).


---

## Bench

Le module `bench/` fournit :

- un **registry** : `bench_registry_add(id, kind, fn, ctx)`
- un **runner** : `bench_runner_run(argc, argv)`
- des **sorties** : console + JSON/CSV (`bench_output_*`)

Signature par défaut (voir `bench/src/bench/types.h`) :

```c
int my_bench(void* ctx, int64_t iters);
```

### Quickstart (sample)

`bench/src/bench/sample.c` est un binaire de démonstration.

Exemples :

```sh
./bench --list
./bench
./bench --bench micro/add
./bench --filter json
./bench --warmup 5 --repeat 20 --min-time-ms 200
./bench --json out/results.json --csv out/results.csv
```

### Intégrer ses benchmarks

```c
#include "bench/registry.h"

static int my_bench(void* ctx, int64_t iters) {
    (void)ctx;
    for (int64_t i = 0; i < iters; ++i) {
        // work
    }
    return 0; // BENCH_RC_OK
}

void register_suite(void) {
    bench_registry_add("micro/my_bench", 0, my_bench, NULL);
}
```

Si ton projet utilise une convention d’appel différente, le runner peut être adapté via `BENCH_FN_CALL(case_ptr, iters)`.

---

## Toolchain

Le dossier `toolchain/` regroupe l’infrastructure pour :

- matérialiser une configuration **par target** (OS/arch/triple),
- construire des **flags** (C/C++/linker) de manière déterministe,
- générer et utiliser des fichiers réponse `.rsp` (clang/lld),
- standardiser l’environnement (SDK path, sysroot, runtime libs, etc.).

L’objectif est un build reproductible et inspectable :

- mêmes entrées → mêmes flags → mêmes artefacts,
- erreurs lisibles (catégorisées),
- séparation nette entre config (headers/templates) et exécution.

---

## Fuzz

Le dossier `fuzz/` contient les éléments pour exécuter des campagnes en local ou en CI :

- configurations CI (sanitizers, coverage, timeouts),
- corpora (seeds) structurés par composant (lexer/ast/ir/bytecode/toolchain, selon le dépôt),
- conventions de reproduction (inputs minimaux, command-lines de repro, triage).

---

## Contribuer

- Ouvrir une issue avec : OS/arch, compiler/version, commande exacte, logs.
- Préférer des PRs petites : une intention → un patch → un test/bench si pertinent.
- Éviter les changements implicites : garder les APIs C17 stables (headers propres, compat C/C++).

---

## Licence

MIT (voir `LICENSE`).
