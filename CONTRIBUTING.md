# Contribuer à Vitte

Bienvenue, artisan·e du compilateur. Ici on aime le code **net**, **prévisible**, **rapide** — et la poésie d’une build verte. ⚔️✨

---

## Sommaire
- [Philosophie](#philosophie)
- [Pré-requis](#pré-requis)
- [Premier clone & build](#premier-clone--build)
- [Branches, commits, PR](#branches-commits-pr)
- [Qualité: checks obligatoires](#qualité-checks-obligatoires)
- [Style de code](#style-de-code)
- [Organisation du workspace](#organisation-du-workspace)
- [Tests](#tests)
- [Perf & Benchmarks](#perf--benchmarks)
- [Sécurité & licences](#sécurité--licences)
- [Cibles & cross-compile](#cibles--cross-compile)
- [Ajouter un nouveau crate](#ajouter-un-nouveau-crate)
- [Docs](#docs)
- [Signalement de vulnérabilités](#signalement-de-vulnérabilités)
- [Code of Conduct](#code-of-conduct)
- [Licence](#licence)

---

## Philosophie
- **Zéro surprise** : toolchain épinglée, lints stricts, sorties reproductibles.
- **Pas d’`unsafe`** dans les libs (sauf justification *très* argumentée).
- **Features explicites** : `std`, `alloc-only`, `serde`, `tracing`, etc.
- **Petites PRs** > grosses refontes. Chaque pas compte.

---

## Pré-requis
- **Rust** : épinglé via `rust-toolchain.toml` → `1.82.0`.
- Composants : `rustfmt`, `clippy`, `rust-src`, `llvm-tools-preview`.
- Optionnel : `wasm-pack` (pour `vitte-wasm`), `npm` (scripts de confort).

```bash
rustup show active-toolchain
```

---

## Premier clone & build
```bash
git clone https://github.com/vitte-lang/vitte.git
cd vitte

# Build complet
cargo build --workspace --all-features

# Tests
cargo test --workspace --all-features
```

> Tu préfères des alias ? À la racine, `package.json` expose des scripts (npm facultatif) :
> `npm run build | test | fmt | clippy | lint | doc | tree | wasm:*`.

---

## Branches, commits, PR
- **Workflow** : `main` protégé → branches `feature/<topic>` / `fix/<bug>`.
- **Commits** : **Conventional Commits** (ex. `feat(parser): Pratt unary minus`).
- **PR** : petite, contextualisée, avec checklist “checks OK”.

**Checklist PR** (copie-colle dans la description) :
- [ ] `cargo fmt` OK
- [ ] `cargo clippy -- -D warnings` OK
- [ ] `cargo test` OK
- [ ] `cargo deny check` OK
- [ ] Docs/rustdoc à jour
- [ ] Changelog (si breaking/feature user-facing)

---

## Qualité: checks obligatoires
Tout doit passer **en local** avant PR.

```bash
# Formatage
cargo fmt --all -- --check

# Lints
cargo clippy --workspace --all-features -- -D warnings

# Sécurité / licences / sources
cargo deny check advisories bans licenses sources

# Build + tests
cargo build --workspace --all-features
cargo test  --workspace --all-features
```

> Lints de base déjà activés dans chaque crate via `[lints.rust]` / `[lints.clippy]`.
> Règles globales : voir `.clippy.toml`.

---

## Style de code
- **rustfmt** : configuré via `rustfmt.toml` (width 100).
- **Clippy** : `.clippy.toml` (complexité, docs, noms bannis, `disallowed_*`).
- **Erreurs** : `thiserror` pour libs, `anyhow` pour bins.
- **Logs** : `tracing` (`tracing-subscriber` côté binaire). Jamais de `println!` en prod.
- **Unsafe** : interdit par défaut (`unsafe_code = "forbid"`).

---

## Organisation du workspace
Crates notables :
- `vitte-core` : types de base, IDs, utilitaires.
- `vitte-lexer`, `vitte-parser`, `vitte-ast`, `vitte-ir` : front-end.
- `vitte-vitbc` : conteneur bytecode (FILE_VERSION=2, CRC32, compression).
- `vitte-vm` : interpréteur pur.
- `vitte-runtime` : runtime/loader/CLI.
- `vitte-tools` : suite CLI (fmt/check/pack/dump/graph/run).
- `vitte-wasm` : bindings WebAssembly.

Conventions features (exemples): `std`, `alloc-only`, `serde`, `tracing`, `small`, `arena`.

---

## Tests
```bash
# Unit + integ
cargo test --workspace --all-features

# Property tests (proptest)
cargo test -p vitte-parser -- --ignored --nocapture

# Snapshots (insta)
cargo insta test -p vitte-parser
```

> **Fuzzing** : non requis par défaut. Si tu veux fuzzer en local, libre à toi
> d’utiliser `cargo-fuzz` sur `vitte-parser`/`vitte-vitbc` (hors repo).

---

## Perf & Benchmarks
- Bench via **criterion** dans les crates qui l’exposent.
```bash
cargo bench --workspace
```
- Mesure ciblée : préférer des micro-benchs stables, inputs fixés.

---

## Sécurité & licences
- **RustSec** : on **refuse** vulnérabilités connues/yanked (voir `deny.toml`).
- **Licences** autorisées : MIT/Apache-2.0/BSD*/ISC/Zlib/BSL/MPL-2.0…
- **Sources** : registries approuvés uniquement ; git pins déterministes.

---

## Cibles & cross-compile
Cibles principales couvertes par `deny.toml` et `rust-toolchain.toml` (réduis localement si besoin) :
- Linux gnu/musl (x86_64, aarch64, armv7, riscv64, etc.)
- macOS (x86_64, arm64)
- Windows (MSVC x64/ARM64)
- Android (armv7, aarch64, x86_64)
- WebAssembly (wasm32-unknown-unknown, WASI)
- BSD/Illumos/Haiku/Redox (selon crates)
- Embedded (thumbv6/7/8, riscv32/64, bare-metal)

---

## Ajouter un nouveau crate
1. `cargo new crates/<nom> --lib`
2. Ajoute-le dans `[workspace].members` (racine).
3. Calque un `Cargo.toml` existant (lints, features `std/alloc-only/serde/tracing`).
4. `cargo check -p <nom>` puis `cargo test -p <nom>`.
5. Ajoute la doc de module + exemples.

---

## Docs
- **Doc de code** (`///`) + exemples testables (doctests).
- Générer :
```bash
cargo doc --workspace --all-features --no-deps
```
- Docs.rs : garde les features docs propres dans `[package.metadata.docs.rs]`.

---

## Signalement de vulnérabilités
Merci d’ouvrir un **security advisory privé** (ou contacter les mainteneurs).
Pas de PoC public avant correctif et CVE éventuel. On répond vite et bien.

---

## Code of Conduct
Nous suivons le **Contributor Covenant (v2.1)**.
Bienveillance, respect, et zéro tolérance pour les comportements toxiques.

---

## Licence
Contributions sous la licence du projet (MIT, sauf mention contraire).
En soumettant une PR, tu confirmes avoir le droit de publier ce code sous cette licence.

---

*“Du code clair, des invariants solides, et la joie tranquille des builds vertes.”* 🟢
