# vitte

Toolchain du langage **Vitte** : spécification, compilateur(s), runtime, stdlib, outils et benches.

> Statut : **expérimental**. APIs/format de fichiers/sémantique peuvent évoluer sans préavis.

---

## Objectifs

- **Langage** : syntaxe "core + phrase" (blocs fermés par `.end`), modules, types, fonctions/scénarios.
- **Toolchain** : lexer/parser → IR → backend (C ou VM/bytecode selon milestone) + outils (fmt/LSP à terme).
- **Runtime/Stdlib** : primitives (strings/slices), erreurs, utilitaires std.
- **Qualité** : builds reproductibles, tests de non-régression, benchs traçables.

---

## Démarrage rapide

Cloner :

```bash
git clone https://github.com/vitte-lang/vitte.git
cd vitte
```

Build (CMake) — la plupart des projets utilisent un out-of-tree build :

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Exécution : le binaire exact dépend du repo (compiler/runner/tools). Liste rapide des exécutables générés :

```bash
find build -maxdepth 3 -type f -perm -111 | head -n 50
```

---

## Exemple Vitte (syntaxe `.end`)

```vitte
# hello.vitte
fn main() -> i32
  say "hello, vitte"
  ret 0
.end
```

Notes :
- Les exemples et modules générés dans ce projet privilégient **`.end`** (pas d’accolades).
- La surface "phrase" (`say`, `set`, `do`, `when`, `loop`, `ret`) est du sucre de surface au-dessus du noyau.

---

## Benchmarks

Un runner robuste est fourni : `run_benchmarks.sh`.

Exemples :

```bash
# Build + exécute 5 runs avec warmup
chmod +x ./run_benchmarks.sh
./run_benchmarks.sh --build --repeat 5 --warmup 1

# Filtrer (si le runner supporte un flag de filter)
./run_benchmarks.sh --bench-filter "json" --out dist/bench
```

Sorties :
- `dist/bench/<run_id>/summary.json` : métadonnées (git/système), résultats (exit codes), layout artifacts
- `dist/bench/<run_id>/summary.csv`  : index run, code retour, artifact associé
- `dist/bench/<run_id>/logs/`        : logs
- `dist/bench/<run_id>/raw/`         : sorties JSON brutes si le runner l’émet

---

## Arborescence (vue d’ensemble)

Cette arborescence peut varier, mais le repo est typiquement organisé autour de :

- `spec/` : spécifications (grammaire, sémantique, ABI/FFI, modules)
- `grammar/` : grammaire parser (ex: `vitte.pest`)
- `compiler/` ou `src/` : frontend (lexer/parser/AST), IR, backends
- `runtime/` : runtime/VM, alloc, erreurs
- `std/` : stdlib (modules)
- `bench/` : micro-benchs + framework
- `tools/` : scripts CI, packaging, utilitaires
- `muffin.muf` / `muffin.lock` : manifests (si présents) pour l’orchestration build/deps

---

## Pré-requis (recommandés)

- **CMake** (>= 3.20 recommandé)
- Un compilateur C/C++ moderne (**Clang** ou **GCC**)
- **Ninja** (optionnel, mais accélère les builds)
- **Python 3** (si scripts/outils)

Vérification rapide :

```bash
cmake --version
cc --version || clang --version
ninja --version || true
python3 --version || true
```

---

## Dépannage

### Erreur Git : `fatal: bad object refs/remotes/origin/HEAD` / `did not send all necessary objects`

Ça correspond généralement à une ref `origin/HEAD` locale corrompue. Correctif :

```bash
git update-ref -d refs/remotes/origin/HEAD
git remote set-head origin -a
git fetch --prune --tags origin
```

Si ça persiste : vérifie `packed-refs` :

```bash
grep -n "refs/remotes/origin/HEAD" .git/packed-refs || true
```

---

## Contribuer

Principes :
- Modifs petites et atomiques, messages de commit explicites.
- Ajoute/maj les **tests** quand tu touches au lexer/parser/IR.
- Les exemples Vitte doivent respecter la convention **`.end`**.

Workflow typique :

```bash
git checkout -b feat/<sujet>
# edits
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
# tests (si disponibles)
ctest --test-dir build --output-on-failure || true
```

---

## Roadmap (haut niveau)

- Stabiliser frontend (lexer/parser) + golden tests
- Définir IR minimal + passes (name resolution / typing subset)
- Choisir et verrouiller le backend du milestone (C backend minimal **ou** VM minimal)
- Runtime minimal (strings/slices, erreurs/panic/report)
- Stdlib prioritaire (`std/core`, `std/cli`, `std/io`, `std/fs`)
- Bench/perf : schéma JSON stable + comparateur

---

## Licence

Voir `LICENSE`.
