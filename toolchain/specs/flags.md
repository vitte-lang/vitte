

# Vitte Toolchain Flags (spécification)

Ce document décrit les **flags / options** attendus pour l’outillage Vitte (compiler `vittec`, driver `vitte`, formatter `vittefmt`, LSP `vittelsp`, etc.).

Objectif : une surface stable, lisible et scriptable (CI, build systems, Muffin `.muf`).

---

## Conventions

- Les flags longs sont au format `--kebab-case`.
- Les flags courts (quand utiles) sont au format `-k`.
- Les valeurs sont soit `--flag=value`, soit `--flag value`.
- Les booléens :
  - `--foo` active
  - `--no-foo` désactive
  - `--foo=true|false` accepté si besoin.
- Les chemins : UTF-8, relatifs ou absolus.
- Les fichiers sources : `*.vitte` (et éventuellement `*.v` si tu gardes la compat).

---

## Commandes (driver)

Le driver `vitte` orchestre les outils. La grammaire standard :

```text
vitte <command> [options] [args]
```

Commandes :
- `build` : build complet (résolution deps + compilation + link).
- `check` : analyse/typing sans produire de binaire final.
- `run` : build + exécution.
- `test` : build + exécution des tests.
- `fmt` : formattage.
- `lsp` : serveur LSP.
- `clean` : purge cache/artifacts.

Chaque commande accepte les flags communs et des flags spécifiques.

---

## Flags communs (toutes commandes)

### Entrées / projet

- `--manifest <path>`
  - Chemin du manifest Muffin (par défaut : `./muffin.muf`).
- `--workspace <path>`
  - Répertoire racine du workspace.
- `--package <name>`
  - Cible un package dans le workspace.
- `--bin <name>`
  - Cible un binaire particulier défini par le manifest.
- `--example <name>`
  - Build/run un exemple.

### Sorties / répertoires

- `--out-dir <path>`
  - Répertoire de sortie (par défaut : `./target`).
- `--cache-dir <path>`
  - Cache incrémental (par défaut : `<out-dir>/cache`).
- `--emit-dir <path>`
  - Où écrire les artefacts `--emit=*` (si non défini : `<out-dir>/emit`).

### Cible / toolchain

- `--target <triple>`
  - Triple cible (ex : `x86_64-unknown-linux-gnu`, `aarch64-apple-darwin`).
- `--host <triple>`
  - Triple hôte (utile pour cross).
- `--toolchain <name>`
  - Sélection explicite d’une toolchain (ex : `system`, `bundled`, `llvm-18`).
- `--sysroot <path>`
  - Sysroot pour headers/libs C/OS.
- `--cc <path>` / `--cxx <path>`
  - Override compilateur C/C++ (si backend C/LLVM utilise un tool externe).
- `--ar <path>` / `--ranlib <path>`
  - Override outils d’archivage.

### Logs

- `-v`, `--verbose`
  - Log détaillé.
- `-q`, `--quiet`
  - Log minimal.
- `--log <level>`
  - `trace|debug|info|warn|error`.
- `--color <mode>`
  - `auto|always|never`.
- `--json`
  - Sortie machine-readable (diagnostics + étapes).

---

## Flags de compilation (build/check/run/test)

### Mode build

- `--profile <name>`
  - `dev|release|bench|test`.
- `--opt-level <n>`
  - `0|1|2|3|s|z`.
- `--debug`
  - Equivalent `--debuginfo=2` + assertions.
- `--debuginfo <n>`
  - `0|1|2`.
- `--strip <mode>`
  - `none|debuginfo|symbols|all`.
- `--lto <mode>`
  - `off|thin|fat`.
- `--codegen-units <n>`
  - Parallélisation codegen.

### Warnings / erreurs

- `-W<name>`
  - Active un warning (ex : `-Wunused`).
- `-Wno-<name>`
  - Désactive.
- `-Werror` ou `--warnings-as-errors`
  - Transforme warnings en erreurs.
- `--deny <name>` / `--warn <name>` / `--allow <name>`
  - Contrôle fin par catégorie.

### Features / cfg

- `--cfg <key>` ou `--cfg <key>=<value>`
  - Injecte une condition de compilation.
- `--feature <name>`
  - Active une feature (répétable).
- `--all-features`
- `--no-default-features`

### Dépendances / résolution

- `--locked`
  - Respect strict de `muffin.lock`.
- `--frozen`
  - Comme `--locked` + pas de réseau.
- `--offline`
  - Pas de réseau.
- `--vendor-dir <path>`
  - Dépendances vendorizées.

### Emission d’artefacts (utile pour debug pipeline)

- `--emit <what>` (répétable)
  - `tokens|ast|ir|mir|llvm|asm|obj|bc|c|json`.
  - Ex : `--emit ast --emit ir`.
- `--emit-format <fmt>`
  - `text|json|bin` selon `--emit`.
- `--emit-stdout`
  - Force l’output sur stdout (un seul `--emit`).

### Backend / linking

- `--backend <name>`
  - `vm|c|llvm` (selon ce que tu gardes).
- `--linker <path>`
  - Override linker.
- `-L <path>`
  - Ajoute un chemin de libs.
- `-l <name>`
  - Link une lib (ex : `-lm`).
- `--static`
  - Link statique si possible.
- `--pie` / `--no-pie`
  - Exécutable PIE.
- `--rdynamic`
  - Exporte les symboles.

### Santé code / sanitizers

- `--sanitize <list>`
  - `address,undefined,thread,leak`.
- `--coverage`
  - Active instrumentation couverture.

### Exécution (run/test)

- `--args "..."`
  - Arguments passés au programme.
- `--`
  - Séparateur classique : tout après est passé au binaire.

---

## Flags du compilateur bas niveau (`vittec`)

Format classique :

```text
vittec [options] <input.vitte>
```

### Entrée

- `--stdin`
  - Lit la source depuis stdin.
- `--path <virtual-path>`
  - Nom logique pour diagnostics (si stdin).

### Diagnostics

- `--error-format <fmt>`
  - `human|short|json`.
- `--max-errors <n>`
  - Arrête après `n` erreurs.
- `--no-suggestions`
  - Désactive hints.

### Pipeline

- `--stage <name>`
  - `lex|parse|resolve|type|ir|codegen|link`.
  - Stop au stage demandé.
- `--dump <what>`
  - Alias rapide pour `--emit`.

### Strictness

- `--edition <year>`
  - Ex : `2025`.
- `--strict`
  - Mode strict (par défaut pour CI).
- `--no-nested-block-comments`
  - Si tu veux matcher un comportement C-like.

---

## Flags Muffin (résolution + build)

Ces flags influencent directement la lecture de `muffin.muf`.

- `--muf-trace`
  - Log détaillé du parsing/résolution Muffin.
- `--muf-validate`
  - Valide le manifest et s’arrête.
- `--lockfile <path>`
  - Override `muffin.lock`.

---

## Flags formatter (`vittefmt`)

- `--check`
  - Ne modifie pas, retourne un code non-zéro si diff.
- `--write`
  - Écrit in-place (default).
- `--stdin` / `--stdout`
  - Pipeline.
- `--config <path>`
  - Config style.
- `--line-width <n>`
  - Largeur cible.

---

## Flags LSP (`vittelsp`)

- `--stdio`
  - LSP sur stdin/stdout.
- `--socket <port>`
  - LSP via TCP.
- `--log-file <path>`
  - Log persistant.
- `--trace`
  - Trace protocole.

---

## Variables d’environnement

- `VITTE_HOME`
  - Home Vitte (toolchains, cache global, etc.).
- `VITTE_TARGET_DIR`
  - Override `--out-dir`.
- `VITTE_LOG`
  - Override `--log`.
- `VITTE_COLOR`
  - Override `--color`.
- `CC`, `CXX`, `AR`, `RANLIB`
  - Overrides toolchain externe.

---

## Exemples

### Build release cross

```sh
vitte build --profile release --target aarch64-apple-darwin --opt-level 3 --lto thin
```

### Check CI strict

```sh
vitte check --locked --warnings-as-errors --error-format short
```

### Dump AST

```sh
vittec --emit ast --emit-format json --emit-dir ./target/emit main.vitte
```

### Run avec args

```sh
vitte run -- --port 8080 --mode fast
```

---

## Statuts / codes retour

- `0` : succès.
- `1` : erreur de compilation/validation.
- `2` : erreur d’usage (flags invalides).
- `3` : erreur interne (panic / bug).

---

## Notes d’implémentation

- Les flags doivent être accessibles via une API stable (ex : `toolchain/cli/args.c`).
- Le rendu JSON (`--json`) doit inclure : étapes, timings, diagnostics, paths d’artefacts.
- `--emit` doit être **déterministe** et stable (utile pour tests snapshot).