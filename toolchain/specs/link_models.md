

# Vitte Link Models (spécification)

Ce document décrit les **modèles de linkage** supportés (ou visés) par la toolchain Vitte.

Objectifs :
- Sorties **prévisibles** (noms, extensions, layout `target/`).
- Comportement **cross-platform** (Linux/macOS/Windows) + cross-compilation.
- Intégration propre avec Muffin (`muffin.muf`) et le driver `vitte`.

---

## Glossaire

- **Link** : étape qui assemble des objets (`.o/.obj`) et des libs en un artefact final.
- **Static link** : toutes les dépendances (ou une partie) sont fusionnées dans l’exécutable.
- **Dynamic link** : l’artefact final dépend de libs chargées au runtime.
- **rpath** : chemins embarqués (Linux/macOS) pour trouver les libs dynamiques.
- **import library** : sur Windows, `.lib` associé à une `.dll`.

---

## Artefacts et types de sortie

Vitte doit pouvoir produire (selon `--backend` et `--crate-type`) :

### 1) Exécutable
- **Linux** : `app` (ELF)
- **macOS** : `app` (Mach-O)
- **Windows** : `app.exe`

### 2) Bibliothèque statique
- **Linux/macOS** : `lib<name>.a`
- **Windows** : `<name>.lib`

### 3) Bibliothèque dynamique (shared)
- **Linux** : `lib<name>.so` (+ SONAME)
- **macOS** : `lib<name>.dylib`
- **Windows** : `<name>.dll` (+ `<name>.lib` import)

### 4) Bibliothèque FFI “C ABI” (cdylib)
Même format que shared, mais contrat :
- ABI C stable (naming, layout, calling convention)
- symboles exportés explicitement

### 5) Artefacts intermédiaires (pipeline)
Selon backend :
- `tokens`, `ast`, `ir`, `mir`
- `llvm` (IR), `bc` (bitcode)
- `asm`, `obj`
- `c` si backend C

Ces artefacts sont contrôlés par `--emit` / `--emit-dir` (voir `flags.md`).

---

## Layout target/ (convention)

Par défaut :

```text
target/
  <triple>/
    <profile>/
      deps/            # libs produites pour les dépendances
      build/           # builds temporaires (C/LLVM)
      emit/            # dumps (tokens/ast/ir/asm/etc.)
      obj/             # objets
      bin/             # exécutables
      lib/             # libs finales
      logs/
```

Règles :
- Les chemins doivent être **déterministes** (utile CI + cache).
- Le nom final est issu de Muffin (`package.name`, `targets.bin[]`, etc.).

---

## Modèles de linkage supportés

### A) `static`
But : portable, déploiement simple.
- Linux/macOS : link statique total si possible.
- Windows : link statique du runtime quand faisable.

Contraintes :
- Certains composants restent dynamiques (libc, frameworks, etc.) selon OS.
- `--static` active ce mode.

### B) `dynamic`
But : taille réduite, mises à jour de libs.
- Produit un exécutable + dépendances dynamiques.
- Gère `rpath` / install_name (macOS).

### C) `hybrid`
But : compromis.
- Dépendances “internes” statiques, dépendances “système” dynamiques.

### D) `plugin`
But : architecture extensible.
- Produit une lib dynamique chargée à la demande (ex: VM, runtime, host app).
- Contrat : point d’entrée stable (`vitte_plugin_init` / versioning).

---

## Résolution des dépendances (ordre de link)

### Règles génériques
- L’ordre importe sur certains linkers :
  - **obj** d’abord, puis **libs**.
  - libs statiques en dernier (ou via groupes `--start-group/--end-group`).
- Déduplication : une lib ne doit pas être linkée 2 fois inutilement.

### Catégories
- **project** : objets du package courant
- **workspace deps** : libs construites depuis le workspace
- **registry deps** : libs externes (si ton écosystème)
- **system** : `-lm`, `-ldl`, frameworks macOS, etc.

---

## rpath / runtime lookup

### Linux (ELF)
- `RPATH`/`RUNPATH` support.
- Convention recommandée :
  - exécutable : `RUNPATH=$ORIGIN/../lib` ou `$ORIGIN` selon layout.

### macOS (Mach-O)
- `install_name` + `@rpath`.
- Convention recommandée :
  - dylib : `@rpath/libname.dylib`
  - exe : `@executable_path/../lib`

### Windows
- Recherche standard : répertoire du `.exe`, puis PATH.
- Convention recommandée :
  - copier les `.dll` dans `bin/` à côté de l’exe (ou `bin/../lib` + script wrapper).

---

## ABI / FFI (contrat)

Pour `cdylib` (FFI C) :
- Export explicite :
  - Linux/macOS : `__attribute__((visibility("default")))`
  - Windows : `__declspec(dllexport)` / `.def`
- Stable layout : types `repr(C)` côté Rust ou équivalent Vitte.
- Versioning :
  - symbole `vitte_abi_version()`
  - ou structure `vitte_abi_info { major, minor, patch }`.

---

## Backend et implications

### Backend `vm`
- Le “link” est conceptuel :
  - regroupe bytecode + metadata + table des symboles.
- Sortie typique : `app.vbc` + runtime `vittevm`.

### Backend `c`
- Génère `.c` puis compile via `cc` en `.o`, puis link.
- Flags transmis au C toolchain (mappés depuis `vitte` / Muffin).

### Backend `llvm`
- Génère IR/obj via LLVM.
- Link via `ld.lld` ou linker système.

---

## Mapping flags (driver → linker)

Le driver `vitte` agrège :
- `-L <path>` → ajoute search path
- `-l <name>` → link lib
- `--linker <path>` → override
- `--static` → `-static` (quand supporté)
- `--pie/--no-pie`
- `--rdynamic`
- `--strip` → `strip` / `-s` selon OS

Le driver doit normaliser les différences OS:
- Windows : `/LIBPATH:` et `.lib`
- macOS : `-framework` pour frameworks

---

## Muffin `.muf` : clés recommandées

Exemple de schéma logique (indicatif) :

```text
[package]
name = "my_app"
version = "0.1.0"

[targets]
# binaire
bin = [{ name = "my_app", path = "src/main.vitte" }]
# lib
lib = [{ name = "my_lib", crate_type = "staticlib" }]

[link]
model = "hybrid"           # static|dynamic|hybrid|plugin
rpath = ["$ORIGIN/../lib"]  # selon OS, normalisé
libs = ["m", "dl"]
search = ["/usr/local/lib"]
frameworks = ["Foundation"] # macOS

[ffi]
exports = ["vitte_api_init", "vitte_api_do"]
abi = "c"
```

Règles :
- Les valeurs `rpath` sont normalisées par OS.
- `frameworks` ignoré hors macOS.

---

## Stratégie “bundle” (distribution)

### Linux/macOS
- Option : `--bundle`
  - copie les `.so/.dylib` requis dans `target/.../lib/`
  - fixe `rpath` de l’exe

### Windows
- `--bundle`
  - copie les `.dll` dans `bin/`.

---

## Déterminisme / reproductibilité

Le link doit être reproductible :
- tri stable des entrées
- chemins normalisés (ou map debug paths)
- timestamps neutralisés si possible

---

## Exemples

### 1) Exécutable statique (best-effort)

```sh
vitte build --profile release --static
```

### 2) Lib dynamique + rpath (Linux)

```sh
vitte build --profile release --no-static -L ./third_party/lib -l foo --emit asm
```

### 3) cdylib FFI

```sh
vitte build --profile release --package vitte_rust_api --emit obj
```

---

## Notes d’implémentation (driver)

- Construire un “link plan” :
  1) inputs (obj) + libs (static/shared)
  2) options (rpath/pie/strip)
  3) outputs (bin/lib)
- Le plan doit être sérialisable en JSON (`--json`) pour debug.
