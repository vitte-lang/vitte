

# vitte-ecs

Système **Entity-Component-System** haute performance pour l’écosystème Vitte.

`vitte-ecs` fournit un moteur ECS moderne, inspiré de Bevy et Flecs, entièrement intégré à l’architecture Vitte (vitte-core, vitte-runtime, vitte-async). Il permet la gestion structurée d’entités, de composants et de systèmes parallèles, tout en restant compatible avec les outils de compilation et d’analyse (`vitte-compiler`, `vitte-analyzer`, `vitte-doc`).

---

## Objectifs

- ECS générique, extensible et type-safe.
- Traitement parallèle des systèmes via `vitte-async`.
- Intégration directe avec le runtime Vitte et les modules std.
- Optimisation du cache mémoire et itérations incrémentales.
- API unifiée pour les moteurs graphiques, physiques ou logiques.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `entity`       | Identifiants uniques, allocations et pools d’entités |
| `component`    | Définition, stockage et typage des composants |
| `system`       | Systèmes déclaratifs, parallélisation, ordonnancement |
| `scheduler`    | Gestion des dépendances et du graphe d’exécution |
| `world`        | Contexte global, accès aux composants et systèmes |
| `query`        | Sélections dynamiques de composants (archetypes) |
| `event`        | Système d’événements, canaux et broadcast |
| `serde`        | (optionnel) Sérialisation et désérialisation |
| `tests`        | Tests unitaires et intégration multi-thread |

---

## Exemple d’utilisation

```vitte
use vitte::ecs::{World, System};

fn move_system(mut pos: CompMut<Position>, vel: CompRef<Velocity>) {
    pos.x += vel.x;
    pos.y += vel.y;
}

fn main() {
    let mut world = World::new();
    world.register::<Position>();
    world.register::<Velocity>();
    world.spawn((Position { x: 0, y: 0 }, Velocity { x: 1, y: 1 }));
    world.run_system(move_system);
}
```

---

## Intégration

- `vitte-runtime` : exécution parallèle et planification des systèmes.
- `vitte-async` : futures et tâches pour systèmes asynchrones.
- `vitte-serialize` : sauvegarde/restauration d’état ECS.
- `vitte-analyzer` : introspection et profiling de graphe ECS.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-async = { path = "../vitte-async", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-macros = { path = "../vitte-macros", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
parking_lot = "0.12"
rayon = "1.10"
hashbrown = "0.14"
anyhow = "1"
```

---

## CLI (pour tests et debug)

```
vitte ecs info
vitte ecs dump --world <file>
vitte ecs graph --format dot
```

---

## Tests

```bash
cargo test -p vitte-ecs
```

- Vérification du graphe d’exécution.
- Tests de parallélisme et contention.
- Benchmarks d’itération et de requêtes.

---

## Roadmap

- [ ] Systèmes distribués multi-monde.
- [ ] Support GPU et compute shaders (via `vitte-gfx`).
- [ ] ECS hybride (statique/dynamique).
- [ ] Éditeur de scène intégré.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau