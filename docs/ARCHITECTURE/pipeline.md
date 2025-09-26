# Vitte — Architecture Overview

## Vision
Vitte est un langage et un écosystème construit pour être minimaliste, portable et puissant.  
Son architecture repose sur une séparation claire des responsabilités : parsing, compilation, génération de bytecode, exécution par une VM dédiée, et intégration avec des outils modernes (LSP, CLI, extensions IDE).

---

## Couches Principales

### 1. **Source Layer**
- **But :** Fournir le code source écrit par l’utilisateur.
- **Formats :** `.vit` (fichiers Vitte standards), `.vl` (VitteLight).
- **Outils associés :** éditeurs, LSP, syntax highlighting.

---

### 2. **Frontend (Parsing & AST)**
- **Lexer :** Découpe le texte en tokens.
- **Parser :** Génère un AST (Abstract Syntax Tree).
- **Vérifications syntaxiques** : erreurs lexicales, erreurs de grammaire.
- **Module :** `vitte-ast`, `vitte-parser`.

---

### 3. **IR Layer (Intermediate Representation)**
- **RawProgram / RawOp** : représentation intermédiaire proche de l’assembleur.
- **Transformations :** simplification, désucreur syntaxique.
- **Optimisations de base :** propagation de constantes, élimination de code mort.
- **Module :** `vitte-core`.

---

### 4. **Compiler & Bytecode**
- **Format :** VITBC (Vitte ByteCode).
- **Sections :** `INT`, `FLOAT`, `STR`, `DATA`, `CODE`.
- **Trailer :** CRC32 pour validation.
- **Option :** Compression `zstd`.
- **Module :** `vitte-compiler`.

---

### 5. **Virtual Machine**
- **Rôle :** Exécuter le bytecode VITBC.
- **Composants :**
  - Loader (lecture, vérification CRC, décompression).
  - Memory Manager (pile, tas, registres).
  - Interpreter Loop.
- **Module :** `vitte-vm`.

---

### 6. **Tooling & Ecosystem**
- **CLI :** `vitte-cli` (exécution, compilation, REPL).
- **LSP :** `vitte-lsp` (diagnostics, autocomplétion, formatage).
- **IDE :** Extension VS Code (`syntax`, `snippets`, `icons`).
- **Testing :** `vitte-fuzz` (fuzzing et robustesse).
- **Future :** `vitte-steel` (compilateur optimisé en Rust natif).

---

## Cross-Platform Support
- **Cibles :**
  - Windows (MSVC, MinGW)
  - Linux (glibc, musl)
  - macOS (Intel, Apple Silicon)
  - BSD (FreeBSD, OpenBSD, NetBSD)
  - RISC-V
  - WebAssembly/WASI
- **Packaging :**
  - Homebrew, MacPorts
  - Linux distros (deb, rpm)
  - Releases GitHub

---

## Diagramme Simplifié

```mermaid
flowchart TD
    A[Source Code .vit] --> B[Lexer]
    B --> C[Parser -> AST]
    C --> D[RawProgram / RawOp IR]
    D --> E[VITBC Compiler]
    E --> F[.vitbc Bytecode]
    F --> G[Virtual Machine]
    G --> H[Execution Results]
