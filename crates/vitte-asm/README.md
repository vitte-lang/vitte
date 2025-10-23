# vitte-asm

Assembler et désassembleur du langage Vitte.  
Crate interne pour la lecture, l’écriture et la validation d’un format assembleur lisible et d’un bytecode bas niveau.

---

## Objectifs

- Définir une grammaire assembleur stable pour Vitte
- Assembler vers bytecode `.vitbc`
- Désassembler depuis `.vitbc` vers `.vitasm`
- Résolution de labels, sections et symboles
- Vérifications structurelles et diagnostics précis
- Passes optionnelles d’optimisation au niveau asm

---

## Architecture

| Module        | Rôle                                                        |
|---------------|-------------------------------------------------------------|
| `lexer`       | Tokenisation de l’assembleur                               |
| `parser`      | Grammaire et AST assembleur                                |
| `encoder`     | Encodage vers bytecode `.vitbc`                            |
| `decoder`     | Décodage et désassemblage                                   |
| `link`        | Résolution de labels, imports/exports, relocations          |
| `validate`    | Contrôles de forme, tailles, alignements                    |
| `passes`      | Petites optimisations (const-fold, nop-strip, peephole)     |
| `diagnostics` | Erreurs, avertissements, notes                              |
| `fmt`         | Impression lisible et normalisation de code asm             |

---

## Dépendances

```toml
[dependencies]
vitte-core        = { path = "../vitte-core" }
vitte-diagnostics = { path = "../vitte-diagnostics" }
vitte-utils       = { path = "../vitte-utils" }
# vitte-asm

Assembler et désassembleur **officiel** du langage Vitte.  
`vitte-asm` constitue la couche bas-niveau du compilateur, responsable de la conversion entre code assembleur humainement lisible (`.vitasm`) et bytecode exécutable (`.vitbc`), tout en assurant la validation structurelle et la compatibilité entre architectures.

---

## Objectifs

- Définir une **grammaire assembleur stable et pérenne** pour Vitte.
- Assembler du code `.vitasm` vers bytecode binaire `.vitbc`.
- Désassembler le bytecode `.vitbc` en texte lisible.
- Résoudre les labels, symboles, imports et sections.
- Garantir la validité, les alignements et la sécurité du code.
- Appliquer des passes d’optimisation locales (peephole, const-folding, etc.).
- Servir de base à l’inspecteur (`vitte inspect`) et aux backends (`vitte-codegen-*`).

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `lexer`        | Découpage lexical et classification des tokens assembleur |
| `parser`       | Construction d’un AST assembleur conforme à la grammaire officielle |
| `encoder`      | Conversion de l’AST en bytecode `.vitbc` |
| `decoder`      | Désassemblage et reconstruction texte depuis bytecode |
| `link`         | Résolution des labels, relocations et sections globales |
| `validate`     | Vérifications structurelles, alignements et tailles |
| `passes`       | Optimisations asm légères (const-fold, nop-strip, peephole) |
| `diagnostics`  | Emission d’erreurs, avertissements et messages contextuels |
| `fmt`          | Impression lisible, normalisation et beautify de code asm |
| `tests`        | Snapshots et vérification de roundtrip (asm → bc → asm) |

---

## Exemple d’utilisation

Assembler :
```bash
vitte asm build main.vitasm -o main.vitbc
```

Désassembler :
```bash
vitte asm disassemble main.vitbc -o main.vitasm
```

Depuis Rust :
```rust
use vitte_asm::{assemble, disassemble};

fn main() -> anyhow::Result<()> {
    let bytecode = assemble("fn main() { return 42 }")?;
    let text = disassemble(&bytecode)?;
    println!("{}", text);
    Ok(())
}
```

---

## Formats pris en charge

| Extension | Description | Type |
|------------|--------------|------|
| `.vitasm`  | Source assembleur texte | UTF-8 |
| `.vitbc`   | Bytecode binaire compilé | Little Endian |
| `.vitobj`  | Objet partiel avec tables de relocation | Intermédiaire |
| `.vitmod`  | Module lié multi-section | Final |

---

## Dépendances

```toml
[dependencies]
vitte-core        = { path = "../vitte-core", version = "0.1.0" }
vitte-diagnostics = { path = "../vitte-diagnostics", version = "0.1.0" }
vitte-utils       = { path = "../vitte-utils", version = "0.1.0" }
vitte-errors      = { path = "../vitte-errors", version = "0.1.0" }
anyhow            = "1"
colored           = "2"
serde             = { version = "1.0", features = ["derive"] }
```

---

## Tests

```bash
cargo test -p vitte-asm
```

- Vérification du roundtrip (`asm -> bc -> asm`).
- Tests de compatibilité sur bytecode généré.
- Snapshots d’instructions et alignements mémoire.
- Validation d’erreurs de parsing et diagnostics.

---

## Roadmap

- [ ] Support des sections personnalisées.
- [ ] Compression et décompression de bytecode.
- [ ] Désassemblage symbolique enrichi (métadonnées, noms).
- [ ] Outils d’analyse du flux d’exécution asm.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau