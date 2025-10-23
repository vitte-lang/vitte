

# vitte-codegen-x86_64

Backend **x86_64** pour le compilateur Vitte.  
`vitte-codegen-x86_64` gère la génération de code machine pour les architectures x86-64 (Intel/AMD), à partir de l’IR Vitte.

---

## Objectifs

- Génération efficace de **code natif x86-64** depuis l’IR intermédiaire.  
- Implémentation complète des conventions d’appel System V AMD64.  
- Gestion des optimisations d’instructions SSE/AVX.  
- Support des modes **AOT** et **JIT** via Cranelift ou LLVM.  
- Compatibilité multi-plateforme (macOS, Linux, Windows).  
- Intégration transparente avec `vitte-compiler` et `vitte-runtime`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `isa`         | Définition de l’ISA x86_64 : registres, ABI, instructions |
| `emit`        | Émission binaire et encodage machine |
| `lower`       | Transformation de l’IR générique vers instructions x86 |
| `regalloc`    | Allocation et gestion des registres (spill/reload) |
| `frame`       | Gestion des frames de pile, prologue/épilogue |
| `abi`         | Conventions d’appel SysV / Microsoft x64 |
| `inst`        | Représentation typée des instructions machine |
| `reloc`       | Gestion des relocations, patchs, symboles |
| `tests`       | Vérification et désassemblage du code généré |

---

## Exemple d’utilisation

```rust
use vitte_codegen_x86_64::X86_64Backend;
use vitte_codegen::{Context, Target};

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new(Target::X86_64);
    let backend = X86_64Backend::new();

    let func = ctx.create_function("add");
    let code = backend.compile(&ctx, &func)?;

    std::fs::write("add.o", code)?;
    Ok(())
}
```

---

## Intégration

- `vitte-codegen` : backend générique, IR et passes communes.  
- `vitte-compiler` : sélection dynamique de la cible et du pipeline.  
- `vitte-build` : linking et production ELF/Mach-O/PE.  
- `vitte-runtime` : chargement JIT et exécution native.  
- `vitte-analyzer` : visualisation et validation CFG x86_64.

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
cargo test -p vitte-codegen-x86_64
```

- Tests d’abaissement IR → x86_64.  
- Vérification des conventions SysV et Microsoft x64.  
- Tests de génération ELF/Mach-O/PE.  
- Benchmarks sur séquences critiques (arithmétique, float, mémoire).

---

## Roadmap

- [ ] Support complet AVX2/AVX512.  
- [ ] Optimisations de pipeline et réduction des stalls.  
- [ ] JIT multi-plateforme intégré à `vitte-runtime`.  
- [ ] Génération DWARF et symboles debug.  
- [ ] Profilage via `vitte-analyzer`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau