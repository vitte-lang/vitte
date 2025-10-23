

# vitte-codegen-rv64

Backend **RISC‑V 64 bits (RV64)** pour le compilateur Vitte.  
`vitte-codegen-rv64` gère la génération de code machine pour l’architecture RISC‑V, à partir de l’IR Vitte, en respectant la spécification RV64G (base + extensions).

---

## Objectifs

- Génération de **code natif RISC‑V** à partir de l’IR intermédiaire.  
- Support des extensions standard : `I` (integer), `M` (mul/div), `A` (atomic), `F` (float), `D` (double).  
- Implémentation des conventions d’appel SysV RISC‑V 64.  
- Émission ELF64 conforme pour Linux et plateformes embarquées.  
- Compatible avec `vitte-codegen` pour unifier les backends ISA.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `isa`         | Définition de l’ISA RISC‑V : registres, instructions, ABI |
| `emit`        | Génération du code binaire RV64 (ELF) |
| `lower`       | Transformation IR → instructions RISC‑V |
| `regalloc`    | Allocation de registres et gestion des spill slots |
| `frame`       | Gestion de la pile et du frame pointer |
| `abi`         | Conventions d’appel et passage d’arguments |
| `inst`        | Modélisation typée des instructions RISC‑V |
| `reloc`       | Résolution et application des relocations |
| `tests`       | Vérification et désassemblage de code RV64 |

---

## Exemple d’utilisation

```rust
use vitte_codegen_rv64::Rv64Backend;
use vitte_codegen::{Context, Target};

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new(Target::Rv64);
    let backend = Rv64Backend::new();

    let func = ctx.create_function("mul");
    let code = backend.compile(&ctx, &func)?;

    std::fs::write("mul.o", code)?;
    Ok(())
}
```

---

## Intégration

- `vitte-codegen` : infrastructure IR commune.  
- `vitte-compiler` : sélection du backend via la cible.  
- `vitte-build` : gestion du cross‑compiling et linking ELF.  
- `vitte-runtime` : exécution native RV64 ou via émulation QEMU.  
- `vitte-analyzer` : inspection et validation CFG IR/RV64.

---

## Dépendances

```toml
[dependencies]
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

cranelift-codegen = "0.110"
cranelift-frontend = "0.110"
cranelift-module = "0.110"
object = "0.36"
anyhow = "1"
thiserror = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
```

---

## Tests

```bash
cargo test -p vitte-codegen-rv64
```

- Tests de génération IR → RV64.  
- Validation du respect des conventions SysV.  
- Tests d’intégration sur ELF exécutables.  
- Vérification de la compatibilité QEMU et Spike.

---

## Roadmap

- [ ] Support des extensions vectorielles `V` et bit‑manipulation `B`.  
- [ ] Optimisations de pipeline et réduction des dépendances RAW.  
- [ ] Support RISC‑V embedded (RV32).  
- [ ] Intégration avec `vitte-runtime` JIT RV64.  
- [ ] Profilage et comptage des cycles.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau