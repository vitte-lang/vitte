# Targets (vittec)

Ce document définit le **modèle de cible** (“target”) utilisé par `vittec`.

La source de vérité machine-readable est dans :
- `share/targets/*.json`

`docs/targets.md` décrit :
- la **convention de nommage** (triples)
- le **contrat JSON** (schéma + champs)
- les **flags toolchain** (C compiler/linker)
- la **matrice** OS/arch

---

## 1) Pourquoi des targets

`vittec` génère du C (phase bootstrap) puis délègue la génération binaire à un toolchain C.

Un “target” doit répondre à :
- quel OS / arch / ABI je vise ?
- quels flags compiler/linker appliquer ?
- quel runtime/defines activer côté C ?
- comment produire un exe / dylib / so / staticlib ?

---

## 2) Convention de triple

On suit une convention proche LLVM/Rust :

```
<arch>-<vendor>-<sys>-<abi>
```

Champs recommandés :
- `arch` : `x86_64`, `aarch64`, `armv7`, `riscv64`, …
- `vendor` : `apple`, `pc`, `unknown`
- `sys` : `darwin`, `linux`, `windows`
- `abi` : `gnu`, `musl`, `msvc`, `android`, …

Exemples standards :
- `x86_64-apple-darwin`
- `aarch64-apple-darwin`
- `x86_64-unknown-linux-gnu`
- `x86_64-unknown-linux-musl`
- `x86_64-pc-windows-msvc`
- `x86_64-pc-windows-gnu`

Règle : le nom du fichier JSON correspond au triple.

Exemple :
- `share/targets/x86_64-apple-darwin.json`

---

## 3) Chargement et résolution

### Entrée utilisateur
- `--target <triple>` : force la cible
- sinon : auto-détection via la plateforme hôte (host target)

### Résolution
`vittec` :
1) cherche un fichier exact `share/targets/<triple>.json`
2) si absent, émet `E01xx` (target unknown) + liste proches (distance simple)

---

## 4) Contrat JSON (schéma)

Chaque fichier `share/targets/<triple>.json` doit respecter ce contrat.

### Champs racine
- `triple` (string, obligatoire) : triple canonical
- `display_name` (string) : nom humain
- `os` (string, obligatoire) : `darwin|linux|windows|...`
- `arch` (string, obligatoire) : `x86_64|aarch64|...`
- `abi` (string, optionnel) : `gnu|musl|msvc|...`
- `endian` (string, optionnel) : `little|big` (défaut: `little`)

### Toolchain
- `cc` : configuration compilation C
- `link` : configuration édition de liens

### Runtime
- `defines[]` : macros globales passées au C (`-DNAME=VALUE`)
- `features{}` : flags internes (ex: `posix`, `win32`, `use_pthreads`, `has_mmap`)

---

## 5) Schéma détaillé (référence)

```json
{
  "triple": "x86_64-unknown-linux-gnu",
  "display_name": "Linux x86_64 (glibc)",
  "os": "linux",
  "arch": "x86_64",
  "abi": "gnu",
  "endian": "little",

  "cc": {
    "driver": "cc",
    "c_standard": "c11",
    "args": ["-O2", "-fno-omit-frame-pointer"],
    "args_debug": ["-O0", "-g"],
    "warnings": ["-Wall", "-Wextra"],
    "include_dirs": ["<sysroot>/include"],
    "defines": ["VITTE_TARGET_LINUX=1"]
  },

  "link": {
    "driver": "cc",
    "args_exe": ["-Wl,-z,relro"],
    "args_shared": ["-shared"],
    "args_static": [],
    "libs": ["m"],
    "rpath": [],
    "lib_dirs": []
  },

  "runtime": {
    "defines": ["VITTE_USE_POSIX=1"],
    "features": {
      "posix": true,
      "win32": false,
      "use_pthreads": true
    }
  }
}
```

### Conventions
- `driver` : commande `cc/clang/gcc/cl` (ou chemin absolu)
- `args` : flags communs
- `args_debug` : flags debug
- `warnings` : flags warnings
- `include_dirs` : répertoires include (optionnel)
- `defines` : macros (optionnel)

`link.args_*` :
- `args_exe` : pour exécutable
- `args_shared` : pour `.so/.dylib/.dll`
- `args_static` : pour `.a/.lib` (souvent vide si on utilise `ar` séparément)

---

## 6) Variables / placeholders

Pour éviter d’encoder des chemins absolus dans JSON, on autorise des placeholders :
- `<sysroot>` : sysroot effectif (si fourni)
- `<workspace>` : racine repo
- `<out>` : répertoire de build (ex: `build/`)

La substitution est faite par `vittec` au chargement du target.

---

## 7) Exemples de targets (pratiques)

### macOS arm64
Fichier : `share/targets/aarch64-apple-darwin.json`
- `cc.driver`: `clang`
- `cc.args`: `-O2 -fno-omit-frame-pointer`
- `link.args_exe`: `-Wl,-dead_strip`
- `runtime.features.posix=true`

### Linux x86_64 glibc
Fichier : `share/targets/x86_64-unknown-linux-gnu.json`
- `cc.driver`: `cc`
- `link.libs`: `m` (math), parfois `dl` selon besoins

### Windows MSVC
Fichier : `share/targets/x86_64-pc-windows-msvc.json`
- `cc.driver`: `cl`
- flags typiques : `/O2`, `/Zi`, `/EHsc` (si C++)
- link : `link.exe` ou `cl` en mode link
- runtime.features.win32=true

---

## 8) Matrice supportée

Cibles “core” (recommandées) :
- macOS : `aarch64-apple-darwin`, `x86_64-apple-darwin`
- Linux : `x86_64-unknown-linux-gnu`, `aarch64-unknown-linux-gnu`
- Windows : `x86_64-pc-windows-msvc`

Cibles “optionnelles” :
- Linux musl : `x86_64-unknown-linux-musl`
- Windows gnu : `x86_64-pc-windows-gnu`

---

## 9) Erreurs typiques

- Target absent : `E01xx` “unknown target”
  - help : “add a JSON file in share/targets/ or choose one from --list-targets”

- Toolchain introuvable : `E01xy` “C compiler not found”
  - note : affiche la commande tentée

- Flags invalides : `E01xz` “toolchain invocation failed”
  - note : stdout/stderr capturés

---

## 10) CLI associée (suggestion)

- `vittec --list-targets` : liste `share/targets/*.json`
- `vittec --target <triple>` : force la cible
- `vittec --print-target` : dump JSON final (après substitution placeholders)

---

## 11) Checklist pour ajouter une cible

1) créer `share/targets/<triple>.json`
2) remplir `cc` + `link` + `runtime`
3) vérifier `vittec --target <triple> --print-target`
4) compiler un `hello.vitte` minimal

---

## Référence

- fichiers : `share/targets/*.json`
- pipeline : `docs/pipeline.md`
- diagnostics : `docs/diagnostics.md`
