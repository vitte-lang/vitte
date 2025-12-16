# rust workspace (vitte)

- `vitte_rust_api` is the only crate exposing `extern "C"` symbols.
- Everything else is pure Rust library/tooling.

## Build
```sh
cargo build -p vitte_rust_api --release
```

## Generate header (optional)
Requires `cbindgen`:
```sh
cbindgen crates/vitte_rust_api -c cbindgen.toml -o include-gen/vitte_rust_api.h
```

# Rust workspace — vitte (max)

Ce dossier contient le **workspace Rust** de Vitte.

Règles d’architecture :
- `crates/vitte_rust_api` est la **seule** crate autorisée à exposer des symboles `extern "C"` (surface FFI stable).
- Toutes les autres crates sont des **libs/outils pure Rust** (pas d’ABI C publique).

---

## 1) Arborescence

- `Cargo.toml` : workspace
- `crates/`
  - `vitte_rust_api/` : FFI (staticlib/cdylib) — surface publique C
  - `vitte_muf/` : parser MUF (lib)
  - `vitte_fmt/` : formatter (tool)
  - `vitte_lsp/` : LSP (tool)
  - `vitte_regex/` : wrapper regex (lib)
  - `vitte_unicode/` : unicode/utf8/normalisation (lib)
- `include-gen/` : headers générés (optionnel)
- `scripts/` : scripts utilitaires (ex: génération headers)
- `cbindgen.toml` : config cbindgen

---

## 2) Pré-requis

- Rust stable (recommandé)
- Cargo
- (optionnel) `cbindgen` pour générer le header C

Installation `cbindgen` :

```sh
cargo install cbindgen
```

---

## 3) Build (crate FFI)

Build release :

```sh
cargo build -p vitte_rust_api --release
```

Build debug :

```sh
cargo build -p vitte_rust_api
```

Artefacts typiques :
- macOS/Linux : `target/<profile>/libvitte_rust_api.a` (static) et/ou `libvitte_rust_api.{dylib,so}` (si activé)
- Windows : `vitte_rust_api.lib` / `vitte_rust_api.dll` selon le type

Le type exact (staticlib/cdylib) dépend de la config de `crates/vitte_rust_api/Cargo.toml`.

---

## 4) Génération du header C (optionnel)

Le header généré est une **vue** de la surface `extern "C"` de `vitte_rust_api`.

Commande :

```sh
cbindgen crates/vitte_rust_api -c cbindgen.toml -o include-gen/vitte_rust_api.h
```

Bonnes pratiques :
- ne jamais éditer `include-gen/vitte_rust_api.h` à la main
- versionner `cbindgen.toml`
- garder les structs `#[repr(C)]` et éviter `bool` Rust dans l’ABI (préférer `u8`) si cette surface est exposée

---

## 5) Build workspace complet

Build de tout le workspace :

```sh
cargo build
```

Build release workspace :

```sh
cargo build --release
```

---

## 6) Tests

Tous les tests :

```sh
cargo test
```

Tests ciblés (ex: MUF parser) :

```sh
cargo test -p vitte_muf
```

---

## 7) Qualité (fmt / clippy)

Formatter :

```sh
cargo fmt
```

Clippy (avec warnings stricts) :

```sh
cargo clippy --all-targets --all-features -- -D warnings
```

---

## 8) Profiling / debug symbols

Recommandations :
- pour profiler en release : activer les symboles (profile release debug) côté Cargo si nécessaire
- éviter les `panic=abort` si vous voulez des backtraces complètes en dev

---

## 9) Cross-compilation (notes)

- Linux musl : ajouter la target Rust correspondante, puis build avec `--target`.
- Windows : privilégier `x86_64-pc-windows-msvc` pour intégration native.

Exemple :

```sh
rustup target add x86_64-unknown-linux-musl
cargo build -p vitte_rust_api --release --target x86_64-unknown-linux-musl
```

---

## 10) Contrat ABI / FFI (rappel)

- Toute surface stable C doit être concentrée dans `vitte_rust_api`.
- Toute évolution de layout ABI doit respecter :
  - `#[repr(C)]`
  - types à tailles fixes
  - append-only sur vtables/structs publiques
  - versioning (SemVer) côté ABI

Les specs associées sont dans `spec/` (runtime ABI, PAL, etc.).

---

## 11) Dépannage

- Header incomplet/incorrect : vérifier `cbindgen.toml` + `#[repr(C)]` + `pub extern "C"`.
- Symbols manquants au link : vérifier le type de crate (`staticlib`/`cdylib`) et la visibilité.
- Windows : attention aux conventions d’appel (documenter `__cdecl` si nécessaire) et aux exports.