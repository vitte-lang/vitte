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
