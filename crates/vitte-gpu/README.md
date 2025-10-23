

# vitte-gpu

Moteur **GPU et calcul parallèle** pour l’écosystème Vitte.

`vitte-gpu` fournit une interface unifiée pour le calcul graphique et général (GPGPU) à travers plusieurs backends : Vulkan, Metal, WebGPU et OpenCL. Il permet à Vitte d’exécuter du code sur le GPU de manière portable, optimisée et intégrée au runtime.

---

## Objectifs

- Abstraction GPU multiplateforme.
- Compilation et exécution de kernels Vitte sur GPU.
- Support des shaders de calcul et graphiques.
- Gestion de buffers, textures et pipelines.
- Intégration directe avec `vitte-runtime` et `vitte-async`.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `device`       | Détection, initialisation et gestion des périphériques GPU |
| `buffer`       | Allocation et transfert de mémoire entre CPU et GPU |
| `pipeline`     | Création et exécution des shaders de calcul ou graphiques |
| `shader`       | Compilation Vitte → SPIR-V / MSL / WGSL |
| `scheduler`    | Orchestration des tâches GPU et synchronisation |
| `interop`      | Intégration CPU ↔ GPU avec `vitte-async` |
| `tests`        | Validation de performances et compatibilité matériel |

---

## Exemple d’utilisation

```rust
use vitte_gpu::{GpuContext, ShaderModule};

fn main() -> anyhow::Result<()> {
    let gpu = GpuContext::new_default()?;
    let shader = ShaderModule::from_source("
        kernel add(a: Buffer<f32>, b: Buffer<f32>, out: Buffer<f32>) {
            out[i] = a[i] + b[i];
        }
    ")?;
    gpu.run(&shader)?;
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : exécution GPU planifiée.
- `vitte-async` : support de futures et streams sur GPU.
- `vitte-codegen-wasm64` : compilation vers WebGPU.
- `vitte-analyzer` : profilage GPU/CPU et statistiques.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-async = { path = "../vitte-async", version = "0.1.0" }

wgpu = "0.19"
naga = "0.19"
pollster = "0.3"
anyhow = "1"
bytemuck = "1"
```

---

## Tests

```bash
cargo test -p vitte-gpu
```

- Tests de compatibilité backend (Metal, Vulkan, WebGPU).
- Validation des transferts mémoire.
- Benchmarks de kernels de calcul.

---

## Roadmap

- [ ] Support complet des shaders graphiques (rendu 2D/3D).
- [ ] Intégration Raytracing via Vulkan RT.
- [ ] Compilation dynamique de kernels depuis Vitte.
- [ ] Analyse et profilage GPU temps réel.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau