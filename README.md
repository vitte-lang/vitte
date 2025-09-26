# Vitte Language — Modern Systems Programming Language

<p align="center">
  <img src="assets/logo-vitte.svg" alt="Vitte Logo" width="220">
</p>

<p align="center">
  <i>Rapide comme C++, sûr comme Rust, simple comme Go — et prêt pour le futur.</i>
</p>

<p align="center">
  <a href="https://github.com/vitte-lang/vitte/actions">
    <img alt="Build" src="https://img.shields.io/github/actions/workflow/status/vitte-lang/vitte/ci.yml?branch=main&label=build&style=flat-square">
  </a>
  <a href="https://github.com/vitte-lang/vitte/releases">
    <img alt="Release" src="https://img.shields.io/github/v/release/vitte-lang/vitte?style=flat-square">
  </a>
  <img alt="Downloads" src="https://img.shields.io/github/downloads/vitte-lang/vitte/total?style=flat-square">
  <img alt="Coverage" src="https://img.shields.io/codecov/c/github/vitte-lang/vitte?style=flat-square">
  <img alt="License" src="https://img.shields.io/github/license/vitte-lang/vitte?style=flat-square">
  <img alt="Top language" src="https://img.shields.io/github/languages/top/vitte-lang/vitte?style=flat-square">
</p>

---

## 📖 Sommaire

1. [Présentation](#-présentation)
2. [Pourquoi Vitte ?](#-pourquoi-vitte-)
3. [État du projet](#-état-du-projet)
4. [Architecture](#-architecture)
5. [Compatibilité & cibles](#-compatibilité--cibles)
6. [Installation](#-installation)
7. [Quickstart](#-quickstart)
8. [CLI & outils](#-cli--outils)
9. [Éditeur & LSP](#-éditeur--lsp)
10. [Exemples](#-exemples)
11. [Crates du monorepo](#-crates-du-monorepo)
12. [Roadmap](#-roadmap)
13. [Contribuer](#-contribuer)
14. [Licence](#-licence)

---

## 🌟 Présentation

**Vitte** est un langage **systèmes & applicatif** pensé pour l’ère post-2025 : performance, sûreté mémoire et expressivité sans chichis.

> _« Un langage doit te laisser coder vite, bien, et longtemps. »_

- **Sûreté** : pas de `null` implicite, emprunts vérifiés statiquement.
- **Perfs** : exécution native (LLVM), JIT (Cranelift) ou VM bytecode (VITBC).
- **Interop** : C/C++/Rust/Zig via FFI, et WebAssembly.
- **DX** : LSP complet (diagnostics, complétion, go-to), CLI outillée.

---

## 🧭 État du projet

- **MSRV** : Rust `1.82.0` (épinglé via `rust-toolchain.toml`).
- **Qualité** : lints stricts (`.clippy.toml`), `cargo deny`, tests & snapshots.
- **Stabilité** : grammaire et IR évoluent encore ; l’ABI VITBC est **versionnée**.

> ⚠️ Jusqu’à la v1, des changements **break** peuvent survenir entre releases.

---

## 🛠 Architecture

```
          ┌──────────────┐
          │  Source .vit │
          └──────┬───────┘
                 ▼
       ┌─────────────────┐
       │ Front-end        │
       │  lexer, parser,  │
       │  diagnostics     │
       └────────┬────────┘
                ▼
       ┌─────────────────┐
       │   IR (CFG/DFG)   │
       │  passes & checks │
       └────────┬────────┘
                ▼
   ┌────────────────────────┐
   │ Backends               │
   │  • LLVM (opt)          │
   │  • Cranelift (JIT)     │
   │  • VM Vitte (VITBC)    │
   └──────────┬─────────────┘
              ▼
         Exécutable / Bytecode
```

---

## 🖥 Compatibilité & cibles

Support de dev quotidien : **Linux x86_64/ARM64**, **macOS (Intel/Apple Silicon)**, **Windows x64/ARM64**, **WASM**.
Cibles élargies (Android, BSD, RISC-V, embedded) sont **expérimentales** et suivies via `deny.toml`.

- OS/arch suivies : voir `rust-toolchain.toml` (`targets`) et `deny.toml` (`[graph].targets`).

---

## ⬇ Installation

### Depuis les sources (recommandé)

```bash
# Prérequis : Rust 1.82.0 (toolchain épinglée)
git clone https://github.com/vitte-lang/vitte.git
cd vitte

# Build complet (workspace)
cargo build --workspace --all-features

# Outils CLI (binaire "vitte")
cargo build -p vitte-tools --features cli --release
./target/release/vitte --help
```

### Auto-complétions (bash/zsh/fish/pwsh/elvish/nu)

Après build, un message s’affiche. Installation automatique :

```bash
./target/release/vitte completions --install
```

Manuelle (ex.) :

```bash
./target/release/vitte completions --shell zsh --dir "$XDG_DATA_HOME/zsh/site-functions"
```

---

## ⚡ Quickstart

Fichier `hello.vit` :

```vitte
do main() {
    print("Hello, Vitte!")
}
```

Compile & exécute :

```bash
# Pack en VITBC puis exécute via le runtime
./target/release/vitte pack hello.vit -o hello.vitbc
./target/release/vitte run hello.vitbc
```

> Tu as aussi `vitte fmt`, `vitte check`, `vitte dump`, `vitte graph` (voir plus bas).

---

## 🧰 CLI & outils

Le binaire principal **`vitte`** regroupe les sous-commandes :

```text
vitte
├─ fmt     # formatage des sources
├─ check   # diagnostics statiques, lints
├─ pack    # compile → VITBC (bytecode)
├─ dump    # inspecte un .vitbc (sections/CRC/compression)
├─ graph   # exports CFG/DFG (DOT)
└─ run     # exécute VITBC via le runtime/VM
```

Exemples :

```bash
vitte fmt src/ --write
vitte check .
vitte pack examples/fib.vit -O2 -o build/fib.vitbc
vitte dump build/fib.vitbc
vitte graph build/fib.vitbc --dot out/cfg.dot
vitte run build/fib.vitbc

### Autres outils du monorepo

Outre la commande unique `vitte`, le crate `vitte-tools` expose plusieurs binaires spécialisés :

| Binaire        | Description rapide                               | Build |
|----------------|---------------------------------------------------|-------|
| `vitte-asm`    | Assembleur `.vit.s → .vitbc`                      | `cargo build -p vitte-tools --bin vitte-asm` |
| `vitte-disasm` | Désassembleur `.vitbc → texte/JSON`               | `cargo build -p vitte-tools --bin vitte-disasm` |
| `vitte-link`   | Linker multi-chunks (fusion, déduplication, strip)| `cargo build -p vitte-tools --bin vitte-link` |
| `vitte-repl`*  | REPL expérimental (couleurs/historique inclus)    | `cargo build -p vitte-tools --bin vitte-repl --features repl-cli` |

> `*` Le REPL est actuellement livré en mode **stub** : l’interface démarre et gère l’historique/couleurs, mais signale que la compilation à la volée n’est pas encore branchée. Utile pour tester l’intégration CLI/LSP sans bloquer le build.
```

---

## 🧑‍💻 Éditeur & LSP

- **LSP** : `vitte-lsp` (VS Code, Neovim, etc.).
  Build : `cargo build -p vitte-lsp --features stdio --release` → binaire `vitte-lsp`.

- **VS Code** : extension TextMate incluse (`editors/vscode-vitte/`).
  Installe via “Install from VSIX…” ou dev :

```bash
# pack rapide (depuis editors/vscode-vitte/)
npm i && npm run build   # si tu as un package.json ; sinon charge le dossier tel quel
```

- **Coloration GitHub** : `.gitattributes` mappe `.vit` & `.vitte` → Rust pour un highlight correct.

---

## 🔬 Exemples

### Pattern Matching

```vitte
match get_data() {
    Ok(val) => print(val),
    Err(e)  => print("Erreur: " + e),
}
```

### Async

```vitte
async do fetch() {
    await net::get("https://example.org")
}
```

### FFI C

```vitte
extern(c) do printf(fmt: *u8, ...) -> i32

do main() {
    printf("Nombre: %d\\n", 42)
}
```

---

## 🗂 Crates du monorepo

| Crate            | Rôle                                                    |
|------------------|---------------------------------------------------------|
| `vitte-core`     | Types de base, IDs, erreurs communes.                   |
| `vitte-lexer`    | Lexeur.                                                 |
| `vitte-parser`   | Grammaire + AST + diagnostics.                          |
| `vitte-ast`      | Structures AST.                                         |
| `vitte-ir`       | IR, CFG/DFG, passes, export DOT.                        |
| `vitte-vitbc`    | Format VITBC : lecture/écriture, sections, CRC, comp.   |
| `vitte-vm`       | VM/interpréteur pur.                                    |
| `vitte-runtime`  | Loader/VM, snapshots, REPL/CLI optionnels.              |
| `vitte-tools`    | Suite CLI : `vitte` (fmt/check/pack/dump/graph/run).    |
| `vitte-lsp`      | Serveur LSP (stdio/tcp).                                |
| `vitte-wasm`     | Bindings WebAssembly (expérimental).                    |
| `stdlib`         | Bibliothèque standard (pré-lude, I/O, temps…).          |
| `modules/*`      | Modules additionnels (optionnels).                      |
| `tests`          | Tests d’intégration E2E cross-crates.                   |

---

## 🗺 Roadmap

- [x] IR & passes de base (CFG/DFG, DOT)
- [x] VITBC v2 (sections, CRC32, compression)
- [x] VM & runtime (fuel, invariants)
- [x] CLI `vitte` (fmt/check/pack/dump/graph/run)
- [x] LSP initial (diagnostics, hover, completion)
- [ ] Backends LLVM/Cranelift stabilisés
- [ ] Debugger + DAP
- [ ] WASM complet (WASI + std partielle)
- [ ] Stdlib étendue (net/fs/async)

---

## 🤝 Contribuer

- **Guides** : [CONTRIBUTING.md](CONTRIBUTING.md)
- **Qualité** : `cargo fmt` • `cargo clippy -D warnings` • `cargo test` • `cargo deny check`
- **MSRV** : 1.82.0 • **unsafe** interdit par défaut.
- **Sécurité** : signale toute vulnérabilité en privé (voir `SECURITY.md` si présent, sinon issue privée/mainteneurs).

---

## 📜 Licence

Triple licence : **MIT OR Apache-2.0 OR BSD-3-Clause**
Voir `LICENSE-MIT`, `LICENSE-APACHE`, `LICENSE-BSD`.

---

> _Du code clair, des invariants solides, et la joie tranquille des builds vertes._ 🟢
