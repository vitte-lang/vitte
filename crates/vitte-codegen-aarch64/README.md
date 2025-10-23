

# vitte-codegen-aarch64

Backend **AArch64** pour le compilateur Vitte.  
`vitte-codegen-aarch64` implémente la génération de code machine ARM 64 bits à partir de l’IR Vitte, en s’appuyant sur `vitte-codegen` et les spécifications de l’architecture ARMv8-A.

---

## Objectifs

- Générer du **code machine optimisé** pour les processeurs ARM 64 bits.  
- Implémenter les conventions d’appel AAPCS64 et l’allocation de registres.  
- Gérer la vectorisation (NEON/SIMD) et les extensions FP.  
- Fournir une couche d’abstraction compatible avec le backend générique (`vitte-codegen`).  
- Assurer la **portabilité** sur macOS (Apple Silicon) et Linux ARM.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `isa`         | Description de l’ISA ARMv8-A : registres, instructions, ABI |
| `emit`        | Émission binaire des instructions ARM64 |
| `lower`       | Abaissement de l’IR générique vers ARM64 |
| `regalloc`    | Allocation de registres et spill/reload |
| `frame`       | Gestion de la pile, prologue/épilogue |
| `abi`         | Conventions d’appel et passage de paramètres |
| `inst`        | Définition typée des instructions ARM64 |
| `reloc`       | Gestion des relocations et patchs de symboles |
| `tests`       | Vérification des séquences d’instructions générées |

---

## Exemple d’utilisation

```rust
use vitte_codegen_aarch64::AArch64Backend;
use vitte_codegen::{Context, Target};

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new(Target::AArch64);
    let backend = AArch64Backend::new();

    let func = ctx.create_function("sum");
    let code = backend.compile(&ctx, &func)?;

    std::fs::write("sum.o", code)?;
    Ok(())
}
```

---

## Intégration

- `vitte-codegen` : fournit l’infrastructure IR et les passes communes.  
- `vitte-compiler` : pilote la génération multi-ISA.  
- `vitte-build` : gère le linking et la production des binaires ARM64.  
- `vitte-runtime` : prend en charge l’exécution native sur macOS ARM64 et Linux ARM.

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
cargo test -p vitte-codegen-aarch64
```

- Tests d’abaissement IR → ARM64.  
- Vérification du respect AAPCS64.  
- Tests d’émission binaire et désassemblage inverse.  
- Tests de compatibilité macOS/Linux ARM.

---

## Roadmap

- [ ] Support NEON complet et instructions vectorielles.  
- [ ] Support des extensions SVE (Scalable Vector Extension).  
- [ ] Optimisations spécifiques Apple M-series.  
- [ ] Émission directe de Mach-O et ELF ARM64.  
- [ ] Intégration JIT avec `vitte-runtime`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau