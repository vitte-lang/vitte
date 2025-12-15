
# Vitte SDK 

Le **Vitte SDK** fournit le **sysroot** officiel utilisé par :
- le compilateur `vittec` (toolchain C/VM)
- les binders/FFI (ex: `rust/crates/vitte-sys`)
- les outils (fmt/lsp/doc) pour connaître ABI/targets/specs/templates

Il est conçu pour être :
- **portable** (macOS/Linux/Windows/*BSD)
- **reproductible** (versionné, checksums)
- **stable** côté ABI (headers publics)

---

## 1) Layout (contract)

Le layout suivant est **contractuel** et ne doit pas changer sans bump MAJOR du SDK.

- `sysroot/include/vitte/**/*.h` : headers ABI stables (runtime + plugins + PAL si exposée)
- `sysroot/lib/vitte/` : libs runtime + stubs (static/shared)
- `sysroot/lib/pkgconfig/*.pc` : (optionnel) pkg-config
- `sysroot/share/vitte/targets/*.json` : cibles supportées (triples/flags/toolchains)
- `sysroot/share/vitte/specs/` : copies (ou pointeurs) vers la spec officielle
- `sysroot/share/vitte/templates/` : templates (projets, modules, plugins)
- `sysroot/share/vitte/licenses/` : licences/NOTICE tierces (si besoin)
- `sysroot/share/vitte/checksums/` : hashes des artefacts du SDK (optionnel)

---

## 2) Variables d’environnement

- `VITTE_SDK=/path/to/vitte/sdk` (répertoire qui contient `sysroot/`)

Recommandé :
- `VITTE_TARGET=<target>` (nom MUF target, ex: `macos_arm64`)
- `VITTE_PROFILE=<profile>` (ex: `debug`, `release`)

---

## 3) Résolution du sysroot

Ordre recommandé (outils) :

1) `VITTE_SDK` si défini
2) `./sdk` relatif au workspace
3) sysroot packagé avec l’install système (ex: `/usr/local/share/vitte/sysroot`)

Un sysroot est valide si :
- `sysroot/include/vitte/` existe
- un fichier version est présent (voir §4)

---

## 4) Versioning

Le SDK a 3 versions pertinentes :

- **SDK version** : version globale du bundle
- **Runtime ABI** : `spec/runtime/rt_abi.md`
- **PAL contract** : `spec/runtime/pal_contract.md`

Recommandation de fichiers :

- `sysroot/share/vitte/VERSION` : version du SDK (SemVer)
- `sysroot/share/vitte/ABI_RUNTIME_VERSION` : SemVer ABI runtime
- `sysroot/share/vitte/PAL_VERSION` : SemVer PAL

Règle :
- si `ABI_RUNTIME_VERSION.major` change → recompiler plugins et consumers.

---

## 5) Contenu des headers ABI

Les headers publics doivent correspondre à la spec :

- `spec/vitte/abi_ffi.md`
- `spec/runtime/rt_abi.md`
- `spec/runtime/pal_contract.md`

Le principe est **append-only** :
- ne jamais réordonner les structs ABI
- n’ajouter que des champs en fin

---

## 6) Cibles (targets) et triples

Les targets sont décrites par des JSON dans :

- `sysroot/share/vitte/targets/*.json`

Chaque JSON doit inclure au minimum :
- `name` (ex: `macos_arm64`)
- `triple` (ex: `aarch64-apple-darwin`)
- `toolchain` (ex: `system`, `llvm`, `msvc`)
- `cflags` / `ldflags` (listes)

Références :
- `spec/targets/triples.md`
- `spec/targets/flags.md`

---

## 7) Specs (copie/pointeur)

Le SDK embarque une copie (ou un lien) des specs pour que :
- `vittec` puisse afficher une aide cohérente
- `vitte-lsp` puisse valider grammaire/semantics
- les templates puissent référencer les mêmes contrats

Chemins recommandés :

- `sysroot/share/vitte/specs/vitte/grammar_core_phrase.ebnf`
- `sysroot/share/vitte/specs/vitte/semantics.md`
- `sysroot/share/vitte/specs/vitte/module_system.md`
- `sysroot/share/vitte/specs/vitte/error_model.md`
- `sysroot/share/vitte/specs/vitte/abi_ffi.md`
- `sysroot/share/vitte/specs/muf/muf_grammar.ebnf`
- `sysroot/share/vitte/specs/muf/muf_semantics.md`
- `sysroot/share/vitte/specs/muf/muf_lockfile.md`
- `sysroot/share/vitte/specs/runtime/pal_contract.md`
- `sysroot/share/vitte/specs/runtime/rt_abi.md`

---

## 8) Templates

`sysroot/share/vitte/templates/` contient des squelettes “prêts à build” :
- projet minimal (hello)
- module std-like
- plugin (dylib) avec `vitte_plugin_entry`
- tool CLI

Chaque template doit :
- inclure un `muffin.muf` minimal
- coller aux conventions `.end`
- inclure une cible/targets par défaut

---

## 9) Exemples d’usage

### 9.1 Détection

- `vittec --sdk-path`
- `vittec --print-sysroot`

### 9.2 Build d’un projet MUF

- `muffin build --target macos_arm64 --profile release`

### 9.3 Génération lock

- `muffin lock`

### 9.4 Plugin (dylib)

- Compiler la dylib avec `-fvisibility=hidden` et exporter `vitte_plugin_entry`
- Vérifier `ABI_RUNTIME_VERSION.major`

---

## 10) Intégrité (checksums)

Recommandation :
- publier un checksum global du SDK (sha256)
- publier un fichier `sysroot/share/vitte/checksums/SHA256SUMS`

Objectif :
- installations reproductibles
- détection de corruption

---

## 11) Politique de compatibilité

- Le SDK est compatible avec un `vittec` qui partage le même `SDK major`.
- Les plugins doivent matcher `ABI_RUNTIME_VERSION.major`.
- Les outils (fmt/lsp) doivent matcher `grammar_core_phrase.ebnf` (même version vendue).

---

## 12) Notes

- Le SDK est un **artefact produit** par le build/release.
- Ne pas éditer manuellement les headers ABI générés si un générateur existe.