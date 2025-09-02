# Vitte — Guide du code source

> *Un dépôt propre, c’est du temps de cerveau libéré pour l’essentiel.* ✨
> Cette page te donne le **plan du château**, les **règles du jeu** et les **astuces anti‑galère** pour travailler confort sur le code Vitte.

---

## 1) Panorama du dépôt

```
.
├─ crates/            # cœur du compilateur & outils
│  ├─ vitte-ast/      # AST, spans, nœuds
│  ├─ vitte-lexer/    # lexeur
│  ├─ vitte-parser/   # parseur
│  ├─ vitte-ir/       # IR & passes
│  ├─ vitte-compiler/ # orchestration front→IR→BC
│  ├─ vitte-vitbc/    # bytecode & formats
│  ├─ vitte-vm/       # machine virtuelle (VM)
│  ├─ vitte-cli/      # binaire `vitte`
│  ├─ vitte-lsp/      # serveur LSP
│  ├─ vitte-debugger/ # débogueur / DAP
│  ├─ vitte-runtime/  # runtime commun
│  └─ ...
├─ modules/           # sous‑systèmes optionnels (fmt, typer, resolver, hal, sys, mm, opt, shed, gfx2d…)
│  ├─ vitte-typer/
│  ├─ vitte-resolver/
│  └─ ...
├─ std/               # bibliothèque standard (crate)
├─ benchmarks/        # micro/macro‑bench (Criterion)
├─ tests/             # tests d’intégration cross‑crates
├─ rfcs/              # propositions de conception
└─ docs/              # documentation du dépôt (ici)
```

* **Workspace** : `vitte/Cargo.toml` déclare tous les membres (wildcards `crates/*`, `modules/*`).
* **Default‑members** : orientés dev quotidien (compilation rapide).
* **Lints** : `unsafe_code = forbid`, `clippy::{all,pedantic,nursery}=warn`.

---

## 2) Pré‑requis & installation

### 2.1 Outils (Windows MSVC, recommandés)

```powershell
# Toolchain Rust stable (MSVC)
rustup toolchain install stable-x86_64-pc-windows-msvc

# Composants indispensables
rustup component add rust-src rustfmt clippy

# Optionnel: nightly pour tests expérimentaux
rustup toolchain install nightly
```

### 2.2 Éditeur (VS Code + rust-analyzer)

* Extension **rust-analyzer**.
* Quelques réglages utiles (à mettre dans `.vscode/settings.json`) :

```jsonc
{
  "rust-analyzer.cargo.features": ["std"],
  "rust-analyzer.checkOnSave.command": "clippy",
  "rust-analyzer.checkOnSave.extraArgs": ["--all-targets", "--all-features", "--workspace"],
  "editor.formatOnSave": true
}
```

> Si tu vois `can't load standard library, try installing rust-src`, exécute: `rustup component add rust-src`.

---

## 3) Construire, tester, linter

### 3.1 Commandes de base

```powershell
# Vue d’ensemble
cargo metadata --no-deps

# Vérif rapide\ ncargo check --workspace --all-targets

# Build debug / release
cargo build --workspace
cargo build --workspace --release

# Lints
cargo clippy --workspace --all-targets -- -Dwarnings

# Formatage
cargo fmt --all

# Tests
cargo test --workspace --all-features
```

### 3.2 Sélection par features

```powershell
# Compiler un module optionnel
cargo build -p vitte-modules --features "resolver,typer,serde"
```

### 3.3 Benchmarks (Criterion)

```powershell
cargo bench -p benchmarks
```

---

## 4) Conventions de code (condensé)

* **Nommage** : `snake_case` (fonctions/variables), `PascalCase` (types), `SCREAMING_SNAKE_CASE` (constantes).
* **Imports** : `use module::sub::{A, B}`; pas de glob `*` (sauf tests).
* **Ergonomie** : dernière expression **sans** `;` = valeur du bloc.
* **Erreurs** : `Result<T,E>` + `?`; `panic` réservé aux invariants (voir RFC 0004).
* **Safety** : `unsafe` = périmètre minimal, justifié en commentaire.
* **Docs** : `///` pour items; `//!` pour docs de module.

> Le style détaillé vit dans **RFC 0018**. Le formatter `vitte-fmt` vise une sortie stable et lisible.

---

## 5) Invariants d’architecture

* **Resolver → Typer** : les diagnostics se basent sur `NodeId` stables.
* **IR** : SSA‑friendly; chaque passe doit conserver les invariants (dominateurs, CFG cohérent).
* **Bytecode (VITBC)** : formats versionnés; pas de breaking sans bump.
* **VM** : respecte le contrat d’aliasing (RFC 0003), `panic` (abort/unwind) dépend du build.

---

## 6) Débogage & profilage

* **Logs** : `RUST_LOG=info vitte ...` (CLI & outils).
* **Trace** : `tracing + tracing-subscriber` (`env-filter`).
* **Perf** : `perf`/`ETW`/`VTune` selon OS; micro‑bench via Criterion; flamegraph conseillé.

---

## 7) Erreurs courantes & remèdes (FAQ)

* ❌ `can't load standard library, try installing rust-src`
  ✅ `rustup component add rust-src` (vérifie aussi ta toolchain sélectionnée par `rustup default`).

* ❌ `failed fetching cargo workspace root` / `file not found: .../Cargo.toml`
  ✅ Ouvre **la racine** du repo (le dossier qui contient `vitte/Cargo.toml`). Évite d’ouvrir un sous‑dossier isolé.

* ❌ `duplicate key log in workspace.dependencies`
  ✅ La table `[workspace.dependencies]` ne doit déclarer **chaque clé qu’une seule fois**. Regroupe `log = "0.4"` au même endroit et **réfère** via `workspace = true` dans les crates membres.

* ❌ `error inheriting env_logger from workspace... not found`
  ✅ Ajoute `env_logger = "0.11"` **dans** `[workspace.dependencies]` (puis `env_logger = { workspace = true }` côté crate) — pas de double déclaration.

---

## 8) Branches, commits, PR

* **Branches** : `feat/*`, `fix/*`, `docs/*`, `refactor/*`.
* **Commits** : style Conventional Commits (ex. `feat(typer): generalize let rec`).
* **PR** : check‑list CI verte, docs/notes RFC à jour si impact public.

---

## 9) Licence & crédits

* Code sous **Apache‑2.0**.
* Contributeurs listés via l’historique git; RFCS signent la conception.

---

## 10) Checklist “PR prête”

* [ ] `cargo check` + `clippy -Dwarnings`
* [ ] `cargo test` (unitaires & intégration)
* [ ] `cargo fmt` (diff propre)
* [ ] Docs (`///`) & RFC lié mis à jour
* [ ] Changelog / notes de version si nécessaire

---

## 11) Commandes utiles (mémo)

```powershell
# Forcer rust-analyzer à relire le workspace
cargo metadata --format-version=1 > $null

# Nettoyer dur
cargo clean -p vitte-cli; cargo clean -p vitte-vm

# Lancer le binaire\ ncargo run -p vitte-cli -- --help
```

---

> *“On vise clair, droit, efficace. Le reste n’est que littérature.”* 🏹
