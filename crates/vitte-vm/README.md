

# vitte-vm

Machine virtuelle **officielle du langage Vitte**, responsable de l’exécution du bytecode `.vitbc`.  
`vitte-vm` gère la pile, les registres, la mémoire et les appels systèmes, et constitue le cœur du moteur d’exécution du langage.

---

## Objectifs

- Offrir une **exécution rapide, déterministe et portable**.  
- Supporter le format binaire standard `.vitbc` généré par `vitte-compiler`.  
- Intégration directe avec `vitte-runtime`, `vitte-compiler`, `vitte-analyzer` et `vitte-tools`.  
- Gérer les threads, exceptions, appels natifs et introspection mémoire.  
- Permettre la sandboxing et le profiling avancé.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `core`        | Boucle d’exécution principale (fetch, decode, execute) |
| `bytecode`    | Gestion du format `.vitbc`, lecture et décodage des instructions |
| `stack`       | Gestion de la pile d’exécution et frames d’appels |
| `memory`      | Allocation dynamique, GC, heap et régions statiques |
| `syscall`     | Appels systèmes et liaisons externes (FFI) |
| `jit`         | Compilation dynamique à la volée (JIT, optionnelle) |
| `trace`       | Profilage et instrumentation d’exécution |
| `tests`       | Validation de la conformité et benchmarks |

---

## Exemple d’utilisation

### Exécution d’un fichier compilé

```bash
vitte run target/main.vitbc
```

### Exemple Rust

```rust
use vitte_vm::Vm;

fn main() -> anyhow::Result<()> {
    let mut vm = Vm::new();
    vm.load_file("target/main.vitbc")?;
    vm.run()?;
    Ok(())
}
```

### Exemple Vitte équivalent

```vitte
fn main() {
    print("Hello from Vitte VM!");
}
```

---

## Intégration

- `vitte-runtime` : gestion des threads, heap et appels internes.  
- `vitte-compiler` : génération de bytecode `.vitbc`.  
- `vitte-tools` : inspection, débogage et instrumentation.  
- `vitte-analyzer` : validation sémantique et exécution symbolique.  
- `vitte-studio` : visualisation du cycle d’exécution et des frames.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-vitbc = { path = "../vitte-vitbc", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

byteorder = "1.5"
parking_lot = "0.12"
rayon = "1.10"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `cranelift` pour backend JIT, `mimalloc` pour allocateur haute performance, `profiling` pour instrumentation temps réel.

---

## Tests

```bash
cargo test -p vitte-vm
```

- Tests d’exécution bas niveau (pile, mémoire, bytecode).  
- Tests de compatibilité inter-version `.vitbc`.  
- Tests de performance et stabilité sous charge.  
- Tests d’intégration avec `vitte-runtime` et `vitte-compiler`.

---

## Roadmap

- [ ] JIT multi-architecture (x86_64, AArch64, RV64).  
- [ ] Support des exceptions et frames dynamiques.  
- [ ] Mode sandbox sécurisé (isolation mémoire et IO).  
- [ ] Visualisation des threads dans `vitte-studio`.  
- [ ] Optimisations dynamiques et trace-based compilation.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau